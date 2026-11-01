"""Effect and parameter definitions shared by the controller and the renderer.

Values are kept in physical units (ms, s, Hz, fraction 0..1). The engine
receives the same units; it enforces its own safety limits as well.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import Optional


@dataclass(frozen=True)
class Division:
    label: str
    beats: float  # quarter-note beats (delay) or beats per LFO cycle (rate)


# Delay divisions, quarter note = 1 beat.
TIME_DIVISIONS = [
    Division("1/32", 0.125), Division("1/16T", 1 / 6), Division("1/16", 0.25), Division("1/16D", 0.375),
    Division("1/8T", 1 / 3), Division("1/8", 0.5), Division("1/8D", 0.75), Division("1/4T", 2 / 3),
    Division("1/4", 1.0), Division("1/4D", 1.5), Division("1/2T", 4 / 3), Division("1/2", 2.0),
    Division("1/2D", 3.0), Division("1/1", 4.0),
]
# Modulation divisions: beats per LFO cycle.
RATE_DIVISIONS = [
    Division("1/16", 0.25), Division("1/8T", 1 / 3), Division("1/8", 0.5), Division("1/8D", 0.75),
    Division("1/4T", 2 / 3), Division("1/4", 1.0), Division("1/4D", 1.5), Division("1/2", 2.0),
    Division("1/1", 4.0), Division("2/1", 8.0), Division("4/1", 16.0), Division("8/1", 32.0),
]


def division_by_label(divs: list[Division], label: str) -> int:
    for i, d in enumerate(divs):
        if d.label == label:
            return i
    return min(range(len(divs)), key=lambda i: abs(divs[i].beats - 1.0))


@dataclass(frozen=True)
class ParamDef:
    key: str
    label: str
    unit: str
    vmin: float
    vmax: float
    default: float
    curve: str = "lin"          # "lin" or "log"
    steps: int = 100            # encoder detents across the full range (before acceleration)
    display: str = "num"        # "pct" shows 0..1 as %, "ms", "s", "hz", "num"
    sync: Optional[str] = None  # "time" or "rate": follows tempo when sync is on
    default_div: str = "1/4"
    options: tuple = ()         # display "enum": names of the values 0, 1, 2, ...

    @property
    def empty(self) -> bool:
        return not self.key

    @property
    def divisions(self) -> list[Division]:
        return TIME_DIVISIONS if self.sync == "time" else RATE_DIVISIONS

    def to_norm(self, v: float) -> float:
        v = min(self.vmax, max(self.vmin, v))
        if self.curve == "log":
            return math.log(v / self.vmin) / math.log(self.vmax / self.vmin)
        return (v - self.vmin) / (self.vmax - self.vmin)

    def from_norm(self, n: float) -> float:
        n = min(1.0, max(0.0, n))
        if self.curve == "log":
            return self.vmin * (self.vmax / self.vmin) ** n
        return self.vmin + n * (self.vmax - self.vmin)

    def step(self, v: float, detents: int) -> float:
        if self.display == "enum":
            # One option per detent, no acceleration.
            d = 1 if detents > 0 else -1 if detents < 0 else 0
            return float(max(0, min(len(self.options) - 1, int(round(v)) + d)))
        n = self.to_norm(v) + detents / self.steps
        out = self.from_norm(n)
        # Snap percentages to whole percent for tidy values.
        if self.display == "pct":
            out = round(out * 100) / 100
            out = min(self.vmax, max(self.vmin, out))
        return out

    def format(self, v: float) -> tuple[str, str]:
        """Return (value text, unit text) for the tile."""
        if self.empty:
            return "—", ""
        if self.display == "enum":
            return self.options[max(0, min(len(self.options) - 1, int(round(v))))], ""
        if self.display == "pct":
            return f"{round(v * 100):d}", "%"
        if self.display == "ratio":
            return (f"{v:.1f}" if v < 10 else f"{v:.0f}"), ":1"
        if self.display == "ms":
            if v < 1:
                return f"{v:.2f}", "ms"
            if v < 10:
                return f"{v:.1f}", "ms"
            if v >= 1000:
                return f"{v / 1000:.2f}", "s"
            return f"{v:.0f}", "ms"
        if self.display == "s":
            return (f"{v:.1f}" if v < 10 else f"{v:.0f}"), "s"
        if self.display == "hz":
            if v >= 1000:
                return f"{v / 1000:.2f}", "kHz"
            if v >= 100:
                return f"{v:.0f}", "Hz"
            if v >= 1:
                return f"{v:.1f}", "Hz"
            return f"{v:.2f}", "Hz"
        if self.display == "db":
            return f"{v:+.1f}".replace("-", "−"), "dB"
        return f"{v:.2f}", self.unit


# Placeholder for an unused position on page 2.
EMPTY = ParamDef("", "", "", 0, 1, 0, display="none")
PARAMS_PER_PAGE = 4
NUM_PARAMS = 8           # two pages of four; the engine uses the same indices


def enum(key: str, label: str, options: tuple, default: int = 0) -> ParamDef:
    return ParamDef(key, label, "", 0, len(options) - 1, default, steps=len(options) - 1,
                    display="enum", options=options)


@dataclass(frozen=True)
class EffectDef:
    key: str
    name: str
    short: str
    params: tuple   # NUM_PARAMS entries; EMPTY where a page has fewer controls

    def __post_init__(self):
        p = tuple(self.params) + (EMPTY,) * (NUM_PARAMS - len(self.params))
        object.__setattr__(self, "params", p)

    @property
    def pages(self) -> int:
        return 2 if any(not p.empty for p in self.params[PARAMS_PER_PAGE:]) else 1


EFFECTS: list[EffectDef] = [
    EffectDef("tape", "TAPE ECHO", "ECHO", (
        ParamDef("time", "TIME", "ms", 10, 2000, 375, "log", 160, "ms", sync="time", default_div="1/8D"),
        ParamDef("feedback", "FEEDBACK", "%", 0, 0.95, 0.35, steps=95, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 0.30, steps=100, display="pct"),
        ParamDef("wow", "WOW", "%", 0, 1, 0.15, steps=100, display="pct"),
    )),
    EffectDef("reverb", "REVERB", "REVERB", (
        ParamDef("decay", "DECAY", "s", 0.2, 15, 2.5, "log", 120, "s"),
        ParamDef("tone", "TONE", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 0.30, steps=100, display="pct"),
        ParamDef("predelay", "PREDELAY", "ms", 0, 250, 20, "lin", 125, "ms"),
    )),
    EffectDef("chorus", "CHORUS", "CHORUS", (
        ParamDef("rate", "RATE", "Hz", 0.05, 5, 0.8, "log", 120, "hz", sync="rate", default_div="1/2"),
        ParamDef("depth", "DEPTH", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("feedback", "FEEDBACK", "%", 0, 0.9, 0.0, steps=90, display="pct"),
    )),
    EffectDef("phaser", "PHASER", "PHASER", (
        ParamDef("rate", "RATE", "Hz", 0.02, 5, 0.4, "log", 120, "hz", sync="rate", default_div="1/1"),
        ParamDef("depth", "DEPTH", "%", 0, 1, 0.7, steps=100, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("feedback", "FEEDBACK", "%", 0, 0.9, 0.3, steps=90, display="pct"),
    )),
    EffectDef("distortion", "DISTORTION", "DIST", (
        ParamDef("drive", "DRIVE", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("tone", "TONE", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 1.0, steps=100, display="pct"),
        ParamDef("level", "LEVEL", "dB", -24, 6, -6, "lin", 60, "db"),
        enum("mode", "TYPE", ("OVERDRIVE", "DISTORTION", "FUZZ", "BITCRUSH", "FOLDER")),
        ParamDef("crush", "CRUSH", "%", 0, 1, 0.5, steps=100, display="pct"),
        ParamDef("gate", "GATE", "%", 0, 1, 0.0, steps=100, display="pct"),
    )),
    EffectDef("saturation", "SATURATION", "SAT", (
        ParamDef("drive", "DRIVE", "%", 0, 1, 0.4, steps=100, display="pct"),
        ParamDef("warmth", "WARMTE", "%", 0, 1, 0.3, steps=100, display="pct"),
        ParamDef("mix", "MIX", "%", 0, 1, 1.0, steps=100, display="pct"),
        ParamDef("level", "LEVEL", "dB", -12, 6, 0, "lin", 36, "db"),
    )),
    EffectDef("filter", "FILTER", "FILTER", (
        ParamDef("cutoff", "CUTOFF", "Hz", 40, 16000, 1200, "log", 160, "hz"),
        ParamDef("reso", "RESO", "%", 0, 1, 0.3, steps=100, display="pct"),
        ParamDef("rate", "LFO RATE", "Hz", 0.02, 10, 0.5, "log", 120, "hz", sync="rate", default_div="1/1"),
        ParamDef("depth", "LFO DIEPTE", "%", 0, 1, 0.0, steps=100, display="pct"),
        enum("mode", "TYPE", ("LADDER LP", "LOW-PASS", "HIGH-PASS", "BAND-PASS", "NOTCH")),
        enum("shape", "LFO VORM", ("SINUS", "DRIEHOEK", "BLOK", "RANDOM")),
        ParamDef("env", "ENVELOPE", "%", 0, 1, 0.0, steps=100, display="pct"),
        ParamDef("drive", "DRIVE", "%", 0, 1, 0.0, steps=100, display="pct"),
    )),
    EffectDef("compressor", "COMPRESSOR", "COMP", (
        ParamDef("threshold", "DREMPEL", "dB", -40, 0, -18, "lin", 80, "db"),
        ParamDef("ratio", "RATIO", ":1", 1, 20, 4, "log", 60, "ratio"),
        ParamDef("attack", "ATTACK", "ms", 0.1, 100, 10, "log", 90, "ms"),
        ParamDef("makeup", "MAKE-UP", "dB", 0, 24, 0, "lin", 48, "db"),
        ParamDef("release", "RELEASE", "ms", 20, 1000, 150, "log", 90, "ms"),
        ParamDef("knee", "KNEE", "dB", 0, 12, 6, "lin", 24, "db"),
        ParamDef("mix", "MIX", "%", 0, 1, 1.0, steps=100, display="pct"),
        enum("limiter", "LIMITER", ("UIT", "AAN"), 1),
    )),
]

# Fixed signal order of the chain (pedalboard): indices into EFFECTS.
CHAIN_ORDER = [6, 5, 4, 2, 3, 0, 1, 7]   # Filter, Sat, Dist, Chorus, Phaser, Tape Echo, Reverb, Compressor

FX_INDEX = {e.key: i for i, e in enumerate(EFFECTS)}


def default_values(fx: int) -> list[float]:
    return [p.default for p in EFFECTS[fx].params]


def default_divs(fx: int) -> list[int]:
    out = []
    for p in EFFECTS[fx].params:
        out.append(division_by_label(p.divisions, p.default_div) if p.sync else 0)
    return out


def accel(dt: float) -> int:
    """Encoder acceleration: detent multiplier from time since the previous detent."""
    if dt < 0.025:
        return 5
    if dt < 0.050:
        return 3
    if dt < 0.090:
        return 2
    return 1


@dataclass
class Settings:
    input_mode: str = "auto"        # "auto", "stereo" or "mono"
    input_gain_db: float = 0.0
    clock_source: str = "auto"      # "auto", "din" or "usb:<name>"
    trails: bool = True
    vu_ref_dbfs: float = -18.0      # level that reads 0 VU
    manual_tempo: float = 120.0

    def to_dict(self) -> dict:
        return dict(self.__dict__)

    @classmethod
    def from_dict(cls, d: dict) -> "Settings":
        s = cls()
        d = d or {}
        try:
            if d.get("input_mode") in ("auto", "stereo", "mono"):
                s.input_mode = d["input_mode"]
            if isinstance(d.get("input_gain_db"), (int, float)):
                s.input_gain_db = max(-12.0, min(12.0, float(d["input_gain_db"])))
            if isinstance(d.get("clock_source"), str):
                s.clock_source = d["clock_source"]
            if isinstance(d.get("trails"), bool):
                s.trails = d["trails"]
            if isinstance(d.get("vu_ref_dbfs"), (int, float)) and float(d["vu_ref_dbfs"]) in VU_REFS:
                s.vu_ref_dbfs = float(d["vu_ref_dbfs"])
            if isinstance(d.get("manual_tempo"), (int, float)):
                s.manual_tempo = max(30.0, min(300.0, float(d["manual_tempo"])))
        except (TypeError, ValueError):
            pass
        return s


VU_REFS = [-12.0, -14.0, -18.0, -20.0]


@dataclass
class SoundState:
    """The sound currently loaded: the whole chain. `fx` is the effect shown on screen."""
    fx: int = 0
    enabled: list[bool] = field(default_factory=lambda: [i == 0 for i in range(len(EFFECTS))])
    values: dict[int, list[float]] = field(default_factory=lambda: {i: default_values(i) for i in range(len(EFFECTS))})
    divs: dict[int, list[int]] = field(default_factory=lambda: {i: default_divs(i) for i in range(len(EFFECTS))})
