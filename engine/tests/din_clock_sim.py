#!/usr/bin/env python3
"""Integration test: feeds a simulated DIN MIDI clock into fxbox-engine.

A pseudo-terminal stands in for the UART (/dev/ttyAMA0). Needs a running JACK
server (e.g. `jackd --no-realtime -d dummy -r 48000 -p 128`).

    python3 din_clock_sim.py path/to/fxbox-engine
Exit code 0 when tempo detection, loss detection and hold behave as specified.
"""
import os
import pty
import random
import socket
import subprocess
import sys
import tempfile
import threading
import time

ENGINE = sys.argv[1] if len(sys.argv) > 1 else "./build/fxbox-engine"

master, slave = pty.openpty()
tty = os.ttyname(slave)
sock_path = os.path.join(tempfile.mkdtemp(), "engine.sock")
eng = subprocess.Popen([ENGINE, "--socket", sock_path, "--din", tty, "--no-usb-midi"],
                       stderr=subprocess.DEVNULL)
for _ in range(50):
    if os.path.exists(sock_path):
        break
    time.sleep(0.1)
time.sleep(0.5)
s = socket.socket(socket.AF_UNIX)
s.connect(sock_path)
s.settimeout(0.05)
s.sendall(b"sync 1\nd 0 0 0.75\ntempo 100\n")

T0 = time.time()
log = []
buf = ""


def pump(seconds):
    global buf
    end = time.time() + seconds
    while time.time() < end:
        try:
            buf += s.recv(65536).decode()
        except socket.timeout:
            pass
        while "\n" in buf:
            line, buf = buf.split("\n", 1)
            if line.startswith("t "):
                p = line.split()
                log.append((time.time() - T0, float(p[1]), p[2], int(p[3])))


def clock(bpm, secs, jitter=0.0015):
    period = 60.0 / bpm / 24.0
    t = time.time()
    end = t + secs
    os.write(master, b"\xfa")          # Start
    while time.time() < end:
        t += period
        d = t + random.gauss(0, jitter) - time.time()
        if d > 0:
            time.sleep(d)
        os.write(master, b"\xf8")      # Clock
    os.write(master, b"\xfc")          # Stop


th = threading.Thread(target=clock, args=(174.0, 6.0))
th.start()
pump(8.0)
th.join()
eng.terminate()
eng.wait(5)

fails = 0


def check(cond, msg):
    global fails
    print(("  ok:   " if cond else "  FAIL: ") + msg)
    fails += 0 if cond else 1


during = [x for x in log if 3.0 < x[0] < 6.0]
after = [x for x in log if x[0] > 7.0]
check(bool(during) and all(x[2] == "din" and x[3] == 1 for x in during), "clock received from 'din' while running")
check(bool(during) and all(abs(x[1] - 174.0) <= 0.5 for x in during),
      "tempo %s BPM (expected 174 +/- 0.5)" % sorted({x[1] for x in during}))
check(bool(after) and all(x[3] == 0 for x in after), "clock reported lost after it stopped")
check(bool(after) and all(abs(x[1] - 174.0) <= 0.5 for x in after),
      "tempo held at last value after loss (%s)" % sorted({x[1] for x in after}))
print("ALL PASSED" if not fails else "FAILED")
sys.exit(1 if fails else 0)
