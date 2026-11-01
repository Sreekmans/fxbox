"""Presets and persistent state.

* Factory presets: read-only JSON shipped with the application.
* User presets and the last state: JSON files in the data directory
  (default /var/lib/fxbox), written atomically (temp file + fsync + rename)
  and only after the controls have been idle for a moment, so turning an
  encoder does not write to the SD card on every detent.
"""
from __future__ import annotations

import json
import logging
import os
import time
from dataclasses import dataclass
from typing import Optional

from .model import CHAIN_ORDER, EFFECTS, FX_INDEX, ParamDef, division_by_label, default_divs, default_values

log = logging.getLogger("fxbox.storage")

USER_BANKS = 4
USER_SLOTS = 8


def atomic_write_json(path: str, data) -> None:
    tmp = f"{path}.tmp"
    payload = json.dumps(data, indent=1, ensure_ascii=False)
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(payload)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, path)
    dfd = os.open(os.path.dirname(os.path.abspath(path)), os.O_RDONLY)
    try:
        os.fsync(dfd)
    finally:
        os.close(dfd)


def read_json(path: str):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


@dataclass
class Preset:
    """A whole chain: which effects are on, their settings, and which effect is shown."""
    name: str
    focus: int
    enabled: list[bool]
    values: dict[int, list[float]]     # only for effects that are on
    divs: dict[int, list[int]]

    @staticmethod
    def _read_fx(fx: int, params: dict, dv: dict) -> tuple[list[float], list[int]]:
        values, divs = default_values(fx), default_divs(fx)
        for i, p in enumerate(EFFECTS[fx].params):
            if not p.empty and p.key in params:
                values[i] = _clamp(p, float(params[p.key]))
            if p.sync and p.key in dv:
                divs[i] = division_by_label(p.divisions, str(dv[p.key]))
        return values, divs

    @classmethod
    def from_json(cls, d: dict) -> "Preset":
        enabled = [False] * len(EFFECTS)
        values: dict[int, list[float]] = {}
        divs: dict[int, list[int]] = {}
        if "chain" in d:
            for key, fd in d["chain"].items():
                fx = FX_INDEX[key]
                enabled[fx] = bool(fd.get("on", True))
                values[fx], divs[fx] = cls._read_fx(fx, fd.get("params", {}), fd.get("divs", {}))
            focus_key = d.get("focus")
            focus = FX_INDEX[focus_key] if focus_key in FX_INDEX else next(
                (f for f in CHAIN_ORDER if enabled[f]), CHAIN_ORDER[0])
        else:   # version 1: a single effect
            fx = FX_INDEX[d["fx"]]
            enabled[fx] = True
            values[fx], divs[fx] = cls._read_fx(fx, d.get("params", {}), d.get("divs", {}))
            focus = fx
        return cls(str(d.get("name", "?"))[:20], focus, enabled, values, divs)

    def to_json(self) -> dict:
        chain = {}
        for fx in CHAIN_ORDER:
            if not self.enabled[fx]:
                continue
            fxdef = EFFECTS[fx]
            vals = self.values.get(fx, default_values(fx))
            dvs = self.divs.get(fx, default_divs(fx))
            entry = {"params": {p.key: round(vals[i], 5) for i, p in enumerate(fxdef.params) if not p.empty}}
            dv = {p.key: p.divisions[dvs[i]].label for i, p in enumerate(fxdef.params) if p.sync}
            if dv:
                entry["divs"] = dv
            chain[fxdef.key] = entry
        return {"name": self.name, "focus": EFFECTS[self.focus].key, "chain": chain}


def _clamp(p: ParamDef, v: float) -> float:
    return min(p.vmax, max(p.vmin, v))


@dataclass
class Bank:
    id: str
    name: str
    presets: list[Optional[Preset]]
    user: bool


class PresetLibrary:
    def __init__(self, factory_path: str, user_path: str):
        self.factory_path = factory_path
        self.user_path = user_path
        self.factory: list[Bank] = []
        self.user: list[Bank] = []
        self.load()

    def load(self) -> None:
        self.factory = []
        try:
            data = read_json(self.factory_path)
            for b in data["banks"]:
                presets = [Preset.from_json(p) for p in b["presets"]]
                self.factory.append(Bank(b["id"], b["name"], presets, False))
        except Exception as e:  # missing or broken factory file must not stop the device
            log.error("factory presets unreadable (%s): %s", self.factory_path, e)
            self.factory = [Bank("F1", "Basis", [Preset("Init " + fx.name.title(), i,
                                                        [j == i for j in range(len(EFFECTS))],
                                                        {i: default_values(i)}, {i: default_divs(i)})
                                                 for i, fx in enumerate(EFFECTS)], False)]
        self.user = [Bank(f"U{i + 1}", f"Eigen {i + 1}", [None] * USER_SLOTS, True) for i in range(USER_BANKS)]
        if os.path.exists(self.user_path):
            try:
                data = read_json(self.user_path)
                for b in data.get("banks", []):
                    idx = next((i for i, ub in enumerate(self.user) if ub.id == b.get("id")), None)
                    if idx is None:
                        continue
                    for s, p in enumerate(b.get("presets", [])[:USER_SLOTS]):
                        if p:
                            try:
                                self.user[idx].presets[s] = Preset.from_json(p)
                            except Exception as e:
                                log.warning("skipping broken user preset %s/%d: %s", b.get("id"), s, e)
            except Exception as e:
                log.error("user presets unreadable, keeping a copy: %s", e)
                try:
                    os.replace(self.user_path, self.user_path + ".bad")
                except OSError:
                    pass

    def save_user(self) -> None:
        data = {"version": 2, "banks": [
            {"id": b.id, "name": b.name, "presets": [p.to_json() if p else None for p in b.presets]} for b in self.user]}
        os.makedirs(os.path.dirname(os.path.abspath(self.user_path)), exist_ok=True)
        atomic_write_json(self.user_path, data)

    def navigable_banks(self) -> list[Bank]:
        """Factory banks plus user banks that contain at least one preset."""
        return self.factory + [b for b in self.user if any(b.presets)]

    def find_bank(self, bank_id: str) -> Optional[Bank]:
        for b in self.factory + self.user:
            if b.id == bank_id:
                return b
        return None

    def user_slots(self) -> list[tuple[str, int]]:
        return [(b.id, s) for b in self.user for s in range(USER_SLOTS)]

    def store(self, bank_id: str, slot: int, preset: Preset) -> None:
        b = self.find_bank(bank_id)
        if not b or not b.user:
            raise ValueError("can only store into user banks")
        b.presets[slot] = preset
        self.save_user()


class DebouncedState:
    """Holds the last-used state and writes it after a quiet period."""

    def __init__(self, path: str, quiet_s: float = 2.0, max_delay_s: float = 15.0):
        self.path = path
        self.quiet_s = quiet_s
        self.max_delay_s = max_delay_s
        self._pending: Optional[dict] = None
        self._first_change = 0.0
        self._last_change = 0.0
        self.writes = 0

    def load(self) -> Optional[dict]:
        if not os.path.exists(self.path):
            return None
        try:
            return read_json(self.path)
        except Exception as e:
            log.error("state file unreadable, starting from defaults: %s", e)
            try:
                os.replace(self.path, self.path + ".bad")
            except OSError:
                pass
            return None

    def update(self, state: dict, now: Optional[float] = None) -> None:
        now = time.monotonic() if now is None else now
        if self._pending is None:
            self._first_change = now
        self._pending = state
        self._last_change = now

    def tick(self, now: Optional[float] = None) -> bool:
        if self._pending is None:
            return False
        now = time.monotonic() if now is None else now
        if now - self._last_change >= self.quiet_s or now - self._first_change >= self.max_delay_s:
            return self.flush()
        return False

    def flush(self) -> bool:
        if self._pending is None:
            return False
        try:
            os.makedirs(os.path.dirname(os.path.abspath(self.path)), exist_ok=True)
            atomic_write_json(self.path, self._pending)
            self.writes += 1
            self._pending = None
            return True
        except OSError as e:
            log.error("could not write state: %s", e)
            return False
