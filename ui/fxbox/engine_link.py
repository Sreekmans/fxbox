"""Connection to the audio engine.

EngineLink talks to fxbox-engine over its UNIX socket. A background thread
reads status lines into a queue; the main loop calls poll() so all state is
only touched from one thread. If the engine restarts, the link reconnects and
the controller re-sends the complete state.

SimEngine offers the same interface with simulated meters and tempo, for the
desktop demo. Everything it reports is marked as simulated in the UI.
"""
from __future__ import annotations

import math
import queue
import random
import socket
import threading
import time
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class Meters:
    vu: tuple[float, float] = (0.0, 0.0)        # linear, reads RMS for a sine
    peak: tuple[float, float] = (0.0, 0.0)      # sample peak since previous report
    out_clips: int = 0
    in_peak: tuple[float, float] = (0.0, 0.0)
    in_clips: int = 0
    mono: bool = False                          # auto input mode detected a mono source
    gr: float = 0.0                             # compressor gain reduction in dB (0 when off)


@dataclass
class TempoInfo:
    bpm: float = 120.0
    source: str = "int"      # "din", "usb:<name>", "tap", "int"
    receiving: bool = False  # clock really arriving from the selected source
    sync: bool = False


@dataclass
class AudioInfo:
    state: str = "unknown"   # ok | error | starting | unknown
    reason: str = "-"
    sample_rate: int = 0
    period: int = 0
    xruns: int = 0
    load: float = 0.0
    faults: int = 0


@dataclass
class LinkState:
    connected: bool = False
    simulated: bool = False
    meters: Meters = field(default_factory=Meters)
    tempo: TempoInfo = field(default_factory=TempoInfo)
    audio: AudioInfo = field(default_factory=AudioInfo)
    sources: list[str] = field(default_factory=list)
    last_contact: float = 0.0
    connected_since: float = 0.0


class EngineLink:
    def __init__(self, path: str = "/run/fxbox/engine.sock"):
        self.path = path
        self.state = LinkState()
        self._q: "queue.Queue[tuple]" = queue.Queue()
        self._sock: Optional[socket.socket] = None
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, name="engine-link", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def send(self, line: str) -> None:
        with self._lock:
            s = self._sock
        if s is None:
            return
        try:
            s.sendall((line + "\n").encode())
        except OSError:
            pass

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(self.path)
            except OSError:
                self._q.put(("disconnected",))
                time.sleep(0.5)
                continue
            with self._lock:
                self._sock = s
            self._q.put(("connected",))
            buf = b""
            s.settimeout(1.0)
            try:
                while not self._stop.is_set():
                    try:
                        chunk = s.recv(65536)
                    except socket.timeout:
                        continue
                    if not chunk:
                        break
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        self._q.put(("line", line.decode(errors="replace")))
            except OSError:
                pass
            with self._lock:
                self._sock = None
            try:
                s.close()
            except OSError:
                pass
            self._q.put(("disconnected",))
            time.sleep(0.3)

    def poll(self) -> list[tuple]:
        """Apply queued status updates; returns events for the controller."""
        events = []
        now = time.monotonic()
        while True:
            try:
                item = self._q.get_nowait()
            except queue.Empty:
                break
            kind = item[0]
            if kind == "connected":
                self.state.connected = True
                self.state.connected_since = now
                self.state.last_contact = now
                events.append(("connected",))
            elif kind == "disconnected":
                if self.state.connected:
                    events.append(("disconnected",))
                self.state.connected = False
            elif kind == "line":
                self.state.last_contact = now
                ev = parse_line(item[1], self.state)
                if ev:
                    events.append(ev)
        return events


def parse_line(line: str, st: LinkState) -> Optional[tuple]:
    p = line.split()
    if not p:
        return None
    try:
        if p[0] == "m" and len(p) >= 9:
            m = st.meters
            m.vu = (float(p[1]), float(p[2]))
            # Peaks are "since last report": keep the max until the UI consumes them.
            m.peak = (max(m.peak[0], float(p[3])), max(m.peak[1], float(p[4])))
            m.out_clips += int(p[5])
            m.in_peak = (max(m.in_peak[0], float(p[6])), max(m.in_peak[1], float(p[7])))
            m.in_clips += int(p[8])
            m.mono = len(p) >= 10 and p[9] == "1"
            m.gr = max(0.0, float(p[10])) if len(p) >= 11 else 0.0
        elif p[0] == "t" and len(p) >= 5:
            st.tempo = TempoInfo(float(p[1]), p[2], p[3] == "1", p[4] == "1")
        elif p[0] == "srcs":
            st.sources = [s for s in (line[5:].split("|") if len(line) > 5 else []) if s]
        elif p[0] == "a" and len(p) >= 8:
            st.audio = AudioInfo(p[1], p[2], int(p[3]), int(p[4]), int(p[5]), float(p[6]), int(p[7]))
        elif p[0] == "tap" and len(p) >= 2:
            return ("tap", float(p[1]))
        elif p[0] == "hello":
            return ("hello", " ".join(p[1:]))
    except (ValueError, IndexError):
        return None
    return None


class SimEngine:
    """Simulated engine for the desktop demo (no audio is processed)."""

    def __init__(self, clock=time.monotonic):
        self.clock = clock
        self.state = LinkState(connected=True, simulated=True)
        self.state.audio = AudioInfo("ok", "-", 48000, 128, 0, 0.0, 0)
        self._events: list[tuple] = [("connected",), ("hello", "simulated")]
        self._t0 = clock()
        self._vu = [0.0, 0.0]
        self._internal_bpm = 120.0
        self._taps: list[float] = []
        self.fx = 0
        self.mix = {i: 0.3 for i in range(8)}
        self.on = {i: i == 0 for i in range(8)}
        self.comp_threshold = -18.0
        self.comp_ratio = 4.0
        self.bypass = False
        self.sync = False
        self.sim_clock = False          # toggled with a key in the demo
        self.sim_clock_bpm = 174.0
        self.clock_source = "auto"
        self.state.sources = ["din", "usb:SIM-CLOCK"]
        self._last = clock()

    def start(self) -> None:
        pass

    def stop(self) -> None:
        pass

    def send(self, line: str) -> None:
        p = line.split()
        if not p:
            return
        try:
            if p[0] == "fx":
                self.fx = int(p[1])
            elif p[0] == "p":
                from .model import EFFECTS
                f, i, v = int(p[1]), int(p[2]), float(p[3])
                key = EFFECTS[f].params[i].key
                if key == "mix":
                    self.mix[f] = v
                elif EFFECTS[f].key == "compressor" and key == "threshold":
                    self.comp_threshold = v
                elif EFFECTS[f].key == "compressor" and key == "ratio":
                    self.comp_ratio = v
            elif p[0] == "on":
                self.on[int(p[1])] = p[2] == "1"
            elif p[0] == "bypass":
                self.bypass = p[1] == "1"
            elif p[0] == "sync":
                self.sync = p[1] == "1"
            elif p[0] == "tempo":
                self._internal_bpm = float(p[1])
                self._src = "int"
            elif p[0] == "clksrc":
                self.clock_source = line.split(" ", 1)[1]
            elif p[0] == "tap":
                now = int(p[1]) * 1e-9 if len(p) > 1 else self.clock()
                if self._taps and now - self._taps[-1] > 2.0:
                    self._taps = []
                self._taps = (self._taps + [now])[-5:]
                if len(self._taps) >= 2:
                    bpm = 60.0 / ((self._taps[-1] - self._taps[0]) / (len(self._taps) - 1))
                    self._internal_bpm = max(30.0, min(300.0, bpm))
                    self._events.append(("tap", self._internal_bpm))
                    self._src = "tap"
        except (ValueError, IndexError):
            pass

    _src = "int"

    def poll(self) -> list[tuple]:
        now = self.clock()
        dt = now - self._last
        self._last = now
        t = now - self._t0
        bpm = 174.0
        beat = 60.0 / bpm
        bar = beat * 4
        ph = t % bar

        def env(at, dec):
            d = ph - at
            if d < 0:
                d += bar
            return math.exp(-d * dec)

        kick = max(env(0, 7), env(beat * 2.5, 7))
        snare = max(env(beat, 9), env(beat * 3, 9))
        hats = 0.5 + 0.5 * math.sin(t * math.pi * 2 * (bpm / 60) * 2)
        bass = 0.5 + 0.5 * math.sin(t * 0.9)
        base = 0.035 + 0.025 * bass + 0.008 * hats
        lvl = [base + 0.11 * kick + 0.07 * snare, base + 0.09 * kick + 0.085 * snare]
        wet = 0.0 if self.bypass else min(1.0, sum(self.mix.get(i, 0.3) for i, o in self.on.items() if o))
        k = 1 - math.exp(-dt / 0.045)
        m = self.state.meters
        pk = []
        for c in range(2):
            target = lvl[c] * (1.0 + 0.4 * wet) * (0.95 + 0.1 * random.random())
            self._vu[c] += (target * 0.8 - self._vu[c]) * k
            pk.append(target * 2.2)
        m.vu = (self._vu[0], self._vu[1])
        m.peak = (max(m.peak[0], pk[0]), max(m.peak[1], pk[1]))
        if self.on.get(7) and not self.bypass:
            over = 20 * math.log10(max(1e-6, max(pk))) - self.comp_threshold
            m.gr = max(0.0, over * (1 - 1 / max(1.0, self.comp_ratio)))
        else:
            m.gr = 0.0

        rx = self.sim_clock and self.clock_source in ("auto", "usb:SIM-CLOCK")
        if rx:
            self.state.tempo = TempoInfo(self.sim_clock_bpm, "usb:SIM-CLOCK", True, self.sync)
        else:
            self.state.tempo = TempoInfo(round(self._internal_bpm, 1), self._src, False, self.sync)
        self.state.last_contact = now
        ev, self._events = self._events, []
        return ev
