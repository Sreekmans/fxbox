#!/usr/bin/env python3
"""Integration test: the real UI process (fxbox.main) against the real fxbox-engine.

Needs a running JACK server (e.g. `jackd --no-realtime -d dummy -r 48000 -p 128`)
and a built engine. The framebuffer is replaced by an in-memory buffer, the
physical controls by injected events.
    python3 tests/integration_engine.py ../engine/build/fxbox-engine
"""
import os
import signal
import subprocess
import sys
import tempfile
import threading
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from fxbox import display, main as uimain  # noqa: E402

ENGINE = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "../engine/build/fxbox-engine")
tmp = tempfile.mkdtemp()
sock = os.path.join(tmp, "engine.sock")
FACTORY = os.path.join(os.path.dirname(__file__), "..", "..", "presets", "factory.json")

eng = subprocess.Popen([ENGINE, "--socket", sock, "--no-din", "--no-usb-midi"], stderr=subprocess.PIPE, text=True)
for _ in range(50):
    if os.path.exists(sock):
        break
    time.sleep(0.1)

# In-memory framebuffer.
frames = {"n": 0, "rows": 0}


def fake_open(self):
    self.stride = self.width * 2
    self.mm = bytearray(self.stride * self.height)


orig_show = display.FramebufferDisplay.show


def counting_show(self, surface):
    rows = orig_show(self, surface)
    frames["n"] += 1
    frames["rows"] += rows
    return rows


display.FramebufferDisplay.open = fake_open
display.FramebufferDisplay.show = counting_show

# Inject control events through the queue main() creates.
captured = {}
orig_queue = uimain.queue.Queue


def capture_queue(*a, **k):
    q = orig_queue(*a, **k)
    captured["q"] = q
    return q


uimain.queue.Queue = capture_queue


def driver():
    time.sleep(2.0)
    q = captured["q"]
    t = time.monotonic()
    q.put(("knob", 2, True, t))              # K3 short press: sync on
    q.put(("knob", 2, False, t + 0.05))
    time.sleep(0.5)
    q.put(("switch", 3, True, time.monotonic()))   # next bank
    time.sleep(0.5)
    q.put(("turn", 0, 1, time.monotonic()))        # next division
    time.sleep(2.5)
    os.kill(os.getpid(), signal.SIGTERM)


threading.Thread(target=driver, daemon=True).start()
sys.argv = ["fxbox-ui", "--socket", sock, "--data-dir", tmp, "--factory", FACTORY,
            "--hw-config", "", "--no-inputs"]
rc = uimain.main()
eng.terminate()
eng.wait(5)

state = __import__("json").load(open(os.path.join(tmp, "state.json")))
fails = 0


def check(cond, msg):
    global fails
    print(("  ok:   " if cond else "  FAIL: ") + msg)
    fails += 0 if cond else 1


check(rc == 0, "UI exited cleanly on SIGTERM")
check(frames["n"] > 100, f"{frames['n']} frames rendered")
check(frames["rows"] < frames["n"] * 320, f"only changed rows written ({frames['rows']} rows in {frames['n']} frames)")
check(state["sync"] is True, "sync state persisted")
check(state["bank"] == "F2", f"bank change persisted ({state['bank']})")
check(state["dirty"] is True, "edit after preset load marked as modified")
print("ALL PASSED" if not fails else "FAILED")
sys.exit(1 if fails else 0)
