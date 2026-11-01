"""Device logic: physical controls -> sound state -> engine, plus the view model.

Controls (knob numbers follow Zynthian's order: K1 top-left, K2 bottom-left,
K3 top-right, K4 bottom-right; each controls the tile with the same number):

  turn K1..K4      always edits the matching parameter of the shown effect
  K1 short press   chain view (turn K1 to pick an effect, K2 = on/off, press K1 to show it)
  K1 hold 0.8 s    save as user preset
  K2 short press   shown effect on/off in the chain
  K2 hold 0.8 s    bypass of the whole chain
  K3 short press   tempo sync on/off
  K3 hold 0.8 s    settings
  K4 press         tap tempo (acts immediately)
  K4 hold 0.8 s    page 1 / page 2 of the shown effect (when it has a second page)
  S1 / S2          previous / next preset
  S3 / S4          previous / next bank
"""
from __future__ import annotations

import logging
import math
import time
from dataclasses import dataclass, field
from typing import Callable, Optional

from .model import (CHAIN_ORDER, EFFECTS, NUM_PARAMS, PARAMS_PER_PAGE, VU_REFS, Settings, SoundState, accel,
                    default_divs, default_values)
from .storage import Bank, DebouncedState, Preset, PresetLibrary

log = logging.getLogger("fxbox.controller")

LONG_PRESS_S = 0.8
PRESS_SUPPRESS_S = 0.08      # rotation ignored around a knob press (mechanical crosstalk)
FX_SELECT_TIMEOUT_S = 8.0
SAVE_TIMEOUT_S = 20.0
SETTINGS_TIMEOUT_S = 45.0
PEAK_HOLD_S = 1.5
CLIP_HOLD_S = 1.5
ENGINE_GRACE_S = 8.0          # after start-up, before "engine missing" is shown

K1, K2, K3, K4 = 0, 1, 2, 3


# ----------------------------------------------------------------- view model

@dataclass
class TileVM:
    label: str
    value: str
    unit: str
    sub: str
    norm: float
    highlight: bool
    synced: bool
    knob: int


@dataclass
class OverlayVM:
    kind: str                  # chain | save | settings
    title: str
    items: list[tuple[str, str]]   # (label, value)
    cursor: int
    hint: str


@dataclass
class ViewModel:
    fx_index: int = 0
    fx_pos: int = 0                 # position of the shown effect in the chain (0..7)
    page: int = 0                   # 0 = page 1, 1 = page 2
    pages: int = 1
    gr_db: float = 0.0              # compressor gain reduction (dB, >= 0)
    fx_name: str = ""
    fx_on: bool = True
    chain: list = field(default_factory=list)   # (short name, on, shown) in chain order
    mono_in: bool = False
    preset_line: str = ""
    dirty: bool = False
    bpm_text: str = "120.0"
    tempo_source: str = "INT"
    clock_rx: bool = False
    sync: bool = False
    bypass: bool = False
    tap_flash: bool = False
    tiles: list[TileVM] = field(default_factory=list)
    vu_db: tuple[float, float] = (-40.0, -40.0)    # relative to the VU reference
    peak_text: tuple[str, str] = ("", "")
    clip: tuple[bool, bool] = (False, False)
    in_clip: bool = False
    vu_ref_text: str = "0 VU = −18 dBFS"
    overlay: Optional[OverlayVM] = None
    toast: str = ""
    fault: Optional[tuple[str, str]] = None
    warning: str = ""
    simulated: bool = False


def fmt_dbfs(lin: float) -> str:
    if lin <= 1e-5:
        return "−∞"
    db = 20 * math.log10(lin)
    if db < -60:
        return "< −60"
    s = f"{db:.1f}"
    return s.replace("-", "−")


class Controller:
    def __init__(self, link, library: PresetLibrary, store: DebouncedState,
                 clock: Callable[[], float] = time.monotonic):
        self.link = link
        self.lib = library
        self.store = store
        self.clock = clock
        self.started = clock()

        self.sound = SoundState()
        self.bank_id = "F1"
        self.preset_idx = 0
        self.preset_name = ""
        self.dirty = False
        self.sync = False
        self.bypass = False
        self.settings = Settings()

        self.mode = "play"
        self.mode_touched = 0.0
        self.cursor = 0
        self.highlight: Optional[int] = None   # parameter index 0..7 last edited
        self.page = 0

        self.knob_down: list[Optional[float]] = [None] * 4
        self.long_fired = [False] * 4
        self.suppress_until = [0.0] * 4
        self.last_detent = [0.0] * 4

        self.toast_text = ""
        self.toast_until = 0.0
        self.tap_flash_until = 0.0
        self.hw_warning = ""

        self.peak_hold = [0.0, 0.0]
        self.peak_hold_t = [0.0, 0.0]
        self.clip_until = [0.0, 0.0]
        self.in_clip_until = 0.0
        self.vu_disp = [0.0, 0.0]

        self._restore()

    # ------------------------------------------------------------ persistence

    def _state_dict(self) -> dict:
        return {
            "version": 1,
            "bank": self.bank_id, "preset": self.preset_idx, "preset_name": self.preset_name,
            "dirty": self.dirty, "fx": EFFECTS[self.sound.fx].key,
            "enabled": [EFFECTS[i].key for i in range(len(EFFECTS)) if self.sound.enabled[i]],
            "values": {EFFECTS[i].key: [round(v, 6) for v in self.sound.values[i]] for i in range(len(EFFECTS))},
            "divs": {EFFECTS[i].key: list(self.sound.divs[i]) for i in range(len(EFFECTS))},
            "sync": self.sync, "bypass": self.bypass, "settings": self.settings.to_dict(),
        }

    def _persist(self) -> None:
        self.store.update(self._state_dict(), self.clock())

    def _restore(self) -> None:
        d = self.store.load()
        if not d:
            banks = self.lib.navigable_banks()
            if banks and any(banks[0].presets):
                self._load_preset(banks[0], next(i for i, p in enumerate(banks[0].presets) if p), send=False)
            return
        try:
            keys = [e.key for e in EFFECTS]
            for i, k in enumerate(keys):
                vals = d.get("values", {}).get(k)
                # Version 1.1 stored 4 values per effect; missing page-2 values get defaults.
                if isinstance(vals, list) and 1 <= len(vals) <= NUM_PARAMS:
                    vals = list(vals) + default_values(i)[len(vals):]
                    self.sound.values[i] = [min(p.vmax, max(p.vmin, float(v))) for p, v in zip(EFFECTS[i].params, vals)]
                dv = d.get("divs", {}).get(k)
                if isinstance(dv, list) and 1 <= len(dv) <= NUM_PARAMS:
                    dv = list(dv) + default_divs(i)[len(dv):]
                    self.sound.divs[i] = [max(0, min(len(p.divisions) - 1, int(x))) if p.sync else 0
                                          for p, x in zip(EFFECTS[i].params, dv)]
            if d.get("fx") in keys:
                self.sound.fx = keys.index(d["fx"])
            if isinstance(d.get("enabled"), list):
                self.sound.enabled = [k in d["enabled"] for k in keys]
            else:   # state from version 1: only the shown effect was active
                self.sound.enabled = [i == self.sound.fx for i in range(len(keys))]
            self.bank_id = str(d.get("bank", "F1"))
            self.preset_idx = int(d.get("preset", 0))
            self.preset_name = str(d.get("preset_name", ""))[:20]
            self.dirty = bool(d.get("dirty", False))
            self.sync = bool(d.get("sync", False))
            self.bypass = bool(d.get("bypass", False))
            self.settings = Settings.from_dict(d.get("settings", {}))
        except (TypeError, ValueError) as e:
            log.error("state ignored: %s", e)

    def shutdown(self) -> None:
        self._persist()
        self.store.flush()

    # ------------------------------------------------------------ engine sync

    def send_all(self) -> None:
        L = self.link.send
        s = self.settings
        L(f"inmode {s.input_mode}")
        L(f"ingain {s.input_gain_db:.2f}")
        L(f"trails {1 if s.trails else 0}")
        L(f"clksrc {s.clock_source}")
        L(f"tempo {s.manual_tempo:.3f}")
        for f in range(len(EFFECTS)):
            for p in range(NUM_PARAMS):
                self._send_param(f, p)
            L(f"on {f} {1 if self.sound.enabled[f] else 0}")
        L(f"sync {1 if self.sync else 0}")
        L(f"bypass {1 if self.bypass else 0}")
        L(f"fx {self.sound.fx}")

    def _send_param(self, f: int, p: int) -> None:
        pd = EFFECTS[f].params[p]
        if pd.empty:
            return
        self.link.send(f"p {f} {p} {self.sound.values[f][p]:.6f}")
        if pd.sync:
            self.link.send(f"d {f} {p} {pd.divisions[self.sound.divs[f][p]].beats:.6f}")

    # ------------------------------------------------------------ presets

    def _banks(self) -> list[Bank]:
        return self.lib.navigable_banks()

    def _bank_pos(self, banks: list[Bank]) -> int:
        for i, b in enumerate(banks):
            if b.id == self.bank_id:
                return i
        return 0

    def _load_preset(self, bank: Bank, idx: int, send: bool = True) -> None:
        p = bank.presets[idx]
        if p is None:
            return
        for f in range(len(EFFECTS)):
            self.sound.values[f] = list(p.values[f]) if f in p.values else default_values(f)
            self.sound.divs[f] = list(p.divs[f]) if f in p.divs else default_divs(f)
        self.sound.enabled = list(p.enabled)
        self.sound.fx = p.focus
        self.bank_id, self.preset_idx, self.preset_name = bank.id, idx, p.name
        self.dirty = False
        self.highlight = None
        self.page = 0
        if send:
            for f in range(len(EFFECTS)):
                for i in range(NUM_PARAMS):
                    self._send_param(f, i)
                self.link.send(f"on {f} {1 if self.sound.enabled[f] else 0}")
            self.link.send(f"fx {p.focus}")
        self._persist()

    def _step_preset(self, direction: int) -> None:
        banks = self._banks()
        if not banks:
            return
        bank = banks[self._bank_pos(banks)]
        filled = [i for i, p in enumerate(bank.presets) if p]
        if not filled:
            return
        if self.preset_idx in filled:
            pos = (filled.index(self.preset_idx) + direction) % len(filled)
        else:
            pos = 0
        self._load_preset(bank, filled[pos])

    def _step_bank(self, direction: int) -> None:
        banks = self._banks()
        if not banks:
            return
        bank = banks[(self._bank_pos(banks) + direction) % len(banks)]
        filled = [i for i, p in enumerate(bank.presets) if p]
        if not filled:
            return
        idx = self.preset_idx if self.preset_idx in filled else filled[0]
        self._load_preset(bank, idx)
        self._toast(f"Bank {bank.id} · {bank.name}")

    # ------------------------------------------------------------ input events

    def handle(self, ev: tuple) -> None:
        kind = ev[0]
        if kind == "turn":
            self._turn(ev[1], ev[2], ev[3])
        elif kind == "knob":
            (self._knob_press if ev[2] else self._knob_release)(ev[1], ev[3])
        elif kind == "switch":
            if ev[2]:
                self._switch(ev[1])

    def _turn(self, k: int, delta: int, t: float) -> None:
        if delta == 0:
            return
        # A turn that coincides with pressing the same knob is mechanical noise.
        if self.knob_down[k] is not None or t < self.suppress_until[k]:
            return
        mult = accel(t - self.last_detent[k])
        self.last_detent[k] = t

        if self.mode == "chain" and k == K1:
            self.cursor = max(0, min(len(CHAIN_ORDER) - 1, self.cursor + (1 if delta > 0 else -1)))
            self.mode_touched = t
            return
        if self.mode == "save" and k == K1:
            n = len(self.lib.user_slots())
            self.cursor = (self.cursor + (1 if delta > 0 else -1)) % n
            self.mode_touched = t
            return
        if self.mode == "settings" and k in (K1, K4):
            self.mode_touched = t
            if k == K1:
                self.cursor = (self.cursor + (1 if delta > 0 else -1)) % len(self._settings_items())
            else:
                self._change_setting(self.cursor, delta * mult)
            return
        self._edit_param(k, delta, mult)

    def _edit_param(self, k: int, delta: int, mult: int) -> None:
        f = self.sound.fx
        if self.page >= EFFECTS[f].pages:
            self.page = 0
        k = self.page * PARAMS_PER_PAGE + k
        pd = EFFECTS[f].params[k]
        if pd.empty:
            return
        if self.sync and pd.sync:
            i = self.sound.divs[f][k] + (1 if delta > 0 else -1)
            self.sound.divs[f][k] = max(0, min(len(pd.divisions) - 1, i))
        else:
            self.sound.values[f][k] = pd.step(self.sound.values[f][k], delta * mult)
        self._send_param(f, k)
        self.highlight = k
        self.dirty = True
        self._persist()

    def _knob_press(self, k: int, t: float) -> None:
        self.knob_down[k] = t
        self.long_fired[k] = False
        if k == K4 and self.mode != "settings":
            self.link.send(f"tap {int(t * 1e9)}")
            self.tap_flash_until = t + 0.12

    def _knob_release(self, k: int, t: float) -> None:
        if self.knob_down[k] is None:
            return
        self.knob_down[k] = None
        self.suppress_until[k] = t + PRESS_SUPPRESS_S
        if self.long_fired[k]:
            return
        if k == K4 and self.mode == "settings":
            return
        if k == K1:
            if self.mode == "play":
                self._open("chain", t, cursor=CHAIN_ORDER.index(self.sound.fx))
            elif self.mode == "chain":
                self._show_fx(CHAIN_ORDER[self.cursor])
                self.mode = "play"
            elif self.mode == "save":
                self._save(self.cursor)
                self.mode = "play"
            elif self.mode == "settings":
                self.mode = "play"
        elif k == K2:
            if self.mode == "chain":
                self.mode_touched = t
                self._toggle_fx(CHAIN_ORDER[self.cursor])
            elif self.mode == "play":
                self._toggle_fx(self.sound.fx)
        elif k == K3:
            if self.mode == "settings":
                self.mode = "play"
            elif self.mode == "save":
                self.mode = "play"
                self._toast("Opslaan geannuleerd")
            else:
                self.sync = not self.sync
                self.link.send(f"sync {1 if self.sync else 0}")
                self._toast("Sync aan" if self.sync else "Sync uit")
                self._persist()

    def _switch(self, s: int) -> None:
        if self.mode != "play":
            if self.mode == "save":
                self._toast("Opslaan geannuleerd")
            self.mode = "play"
            return
        if s == 0:
            self._step_preset(-1)
        elif s == 1:
            self._step_preset(+1)
        elif s == 2:
            self._step_bank(-1)
        elif s == 3:
            self._step_bank(+1)

    def _open(self, mode: str, t: float, cursor: int = 0) -> None:
        self.mode = mode
        self.mode_touched = t
        self.cursor = cursor

    def _show_fx(self, f: int) -> None:
        """Shows another effect of the chain on screen (the sound does not change)."""
        if f == self.sound.fx:
            return
        self.sound.fx = f
        self.link.send(f"fx {f}")
        self.highlight = None
        self.page = 0
        self._persist()

    def _toggle_fx(self, f: int) -> None:
        on = not self.sound.enabled[f]
        self.sound.enabled[f] = on
        self.link.send(f"on {f} {1 if on else 0}")
        self.dirty = True
        self._toast(f"{EFFECTS[f].name.title()} {'aan' if on else 'uit'}")
        self._persist()

    def _toggle_page(self) -> None:
        if self.mode != "play":
            return
        fx = EFFECTS[self.sound.fx]
        if fx.pages < 2:
            self._toast("Geen pagina 2")
            return
        self.page = 1 - self.page
        self.highlight = None
        self._toast(f"Pagina {self.page + 1}")

    def _set_bypass(self, on: bool) -> None:
        self.bypass = on
        self.link.send(f"bypass {1 if on else 0}")
        self._toast("Hele keten in bypass" if on else "Keten actief")
        self._persist()

    # ------------------------------------------------------------ save

    def _save_default_slot(self) -> int:
        slots = self.lib.user_slots()
        if (self.bank_id, self.preset_idx) in slots:
            return slots.index((self.bank_id, self.preset_idx))
        for i, (b, s) in enumerate(slots):
            if self.lib.find_bank(b).presets[s] is None:
                return i
        return 0

    def _save(self, slot_pos: int) -> None:
        bank_id, slot = self.lib.user_slots()[slot_pos]
        f = self.sound.fx
        name = (self.preset_name or EFFECTS[f].name.title()).rstrip(" *")[:20]
        on = list(self.sound.enabled)
        preset = Preset(name, f, on,
                        {i: list(self.sound.values[i]) for i in range(len(EFFECTS)) if on[i]},
                        {i: list(self.sound.divs[i]) for i in range(len(EFFECTS)) if on[i]})
        try:
            self.lib.store(bank_id, slot, preset)
        except OSError as e:
            log.error("saving preset failed: %s", e)
            self._toast("Opslaan mislukt")
            return
        self.bank_id, self.preset_idx, self.preset_name = bank_id, slot, name
        self.dirty = False
        self._persist()
        self._toast(f"Opgeslagen in {bank_id}·{slot + 1}")

    # ------------------------------------------------------------ settings

    def _clock_choices(self) -> list[str]:
        return ["auto", "din"] + [s for s in self.link.state.sources if s.startswith("usb:")]

    @staticmethod
    def _clock_label(src: str) -> str:
        if src == "auto":
            return "Automatisch"
        if src == "din":
            return "DIN-MIDI"
        if src.startswith("usb:"):
            return "USB " + src[4:][:14]
        return src

    def _settings_items(self) -> list[tuple[str, str]]:
        s = self.settings
        a = self.link.state.audio
        info = f"{a.sample_rate / 1000:g} kHz · {a.period} · xrun {a.xruns} · DSP {a.load:.0f}%" if a.sample_rate else "audio niet actief"
        return [
            ("Ingang", {"mono": "Mono (L)", "stereo": "Stereo"}.get(s.input_mode, "Automatisch")),
            ("Ingangsniveau", f"{s.input_gain_db:+.1f} dB".replace("-", "−")),
            ("Clockbron", self._clock_label(s.clock_source)),
            ("Tempo (handmatig)", f"{s.manual_tempo:.1f} BPM"),
            ("Staarten bij bypass", "Aan" if s.trails else "Uit"),
            ("VU-referentie", f"0 VU = {s.vu_ref_dbfs:.0f} dBFS".replace("-", "−")),
            ("Info", info),
        ]

    def _change_setting(self, item: int, delta: int) -> None:
        s = self.settings
        d = 1 if delta > 0 else -1
        if item == 0:
            modes = ["auto", "stereo", "mono"]
            i = modes.index(s.input_mode) if s.input_mode in modes else 0
            s.input_mode = modes[(i + d) % len(modes)]
            self.link.send(f"inmode {s.input_mode}")
        elif item == 1:
            s.input_gain_db = max(-12.0, min(12.0, round((s.input_gain_db + 0.5 * delta) * 2) / 2))
            self.link.send(f"ingain {s.input_gain_db:.2f}")
        elif item == 2:
            ch = self._clock_choices()
            i = ch.index(s.clock_source) if s.clock_source in ch else 0
            s.clock_source = ch[(i + d) % len(ch)]
            self.link.send(f"clksrc {s.clock_source}")
        elif item == 3:
            s.manual_tempo = max(30.0, min(300.0, round((s.manual_tempo + 0.5 * delta) * 2) / 2))
            self.link.send(f"tempo {s.manual_tempo:.3f}")
        elif item == 4:
            s.trails = not s.trails
            self.link.send(f"trails {1 if s.trails else 0}")
        elif item == 5:
            i = VU_REFS.index(s.vu_ref_dbfs) if s.vu_ref_dbfs in VU_REFS else 2
            s.vu_ref_dbfs = VU_REFS[(i + d) % len(VU_REFS)]
        else:
            return
        self._persist()

    # ------------------------------------------------------------ engine events

    def engine_event(self, ev: tuple) -> None:
        if ev[0] == "connected":
            self.send_all()
        elif ev[0] == "tap":
            bpm = ev[1]
            self.settings.manual_tempo = round(bpm, 2)
            tempo = self.link.state.tempo
            if self.sync and tempo.receiving:
                self._toast("Externe clock heeft voorrang")
            elif not self.sync:
                # Without sync, tap sets the echo time directly (quarter note).
                for f, fxdef in enumerate(EFFECTS):
                    if not self.sound.enabled[f]:
                        continue
                    for i, pd in enumerate(fxdef.params):
                        if pd.sync == "time":
                            self.sound.values[f][i] = max(pd.vmin, min(pd.vmax, 60000.0 / bpm))
                            self._send_param(f, i)
                            if f == self.sound.fx:
                                self.highlight = i
                            self.dirty = True
                self._toast(f"Tap {bpm:.1f} BPM")
            else:
                self._toast(f"Tap {bpm:.1f} BPM")
            self._persist()

    def _toast(self, text: str, dur: float = 1.4) -> None:
        self.toast_text = text
        self.toast_until = self.clock() + dur

    # ------------------------------------------------------------ periodic

    def tick(self, now: float) -> None:
        for k in (K1, K2, K3, K4):
            t = self.knob_down[k]
            if t is not None and not self.long_fired[k] and now - t >= LONG_PRESS_S:
                self.long_fired[k] = True
                if k == K4:
                    self._toggle_page()
                elif k == K1:
                    self._open("save", now, cursor=self._save_default_slot())
                elif k == K2:
                    self._set_bypass(not self.bypass)
                else:
                    self._open("settings", now, cursor=0)
        timeout = {"chain": FX_SELECT_TIMEOUT_S, "save": SAVE_TIMEOUT_S, "settings": SETTINGS_TIMEOUT_S}.get(self.mode)
        if timeout and now - self.mode_touched > timeout and all(d is None for d in self.knob_down):
            self.mode = "play"
        self.store.tick(now)

    # ------------------------------------------------------------ view

    def _meters(self, now: float) -> None:
        m = self.link.state.meters
        for c in range(2):
            if m.peak[c] >= self.peak_hold[c] or now - self.peak_hold_t[c] > PEAK_HOLD_S:
                self.peak_hold[c] = m.peak[c]
                self.peak_hold_t[c] = now
            if m.peak[c] >= 0.999:
                self.clip_until[c] = now + CLIP_HOLD_S
            # Interpolate between 30 Hz engine updates for a smooth needle.
            self.vu_disp[c] += (m.vu[c] - self.vu_disp[c]) * 0.55
        if m.in_clips > 0:
            self.in_clip_until = now + CLIP_HOLD_S
        m.peak = (0.0, 0.0)
        m.out_clips = 0
        m.in_clips = 0
        m.in_peak = (0.0, 0.0)

    def _fault(self, now: float) -> tuple[Optional[tuple[str, str]], str]:
        st = self.link.state
        if not st.connected:
            if now - self.started > ENGINE_GRACE_S:
                return ("Audio-engine niet bereikbaar",
                        "De effectverwerking start opnieuw. Blijft dit staan, zie de herstelhandleiding."), ""
            return None, "Audio start…"
        a = st.audio
        if a.state == "error":
            msg = {
                "no-card": ("Geen audio-interface gevonden",
                            "Controleer of de geluidskaart goed op de Pi zit. Het toestel blijft opnieuw proberen."),
                "no-jack": ("Audioserver niet actief", "Wordt automatisch opnieuw gestart."),
                "jack-stopped": ("Audio onderbroken", "Wordt automatisch opnieuw gestart."),
                "activate-failed": ("Audio kon niet starten", "Wordt automatisch opnieuw geprobeerd."),
            }.get(a.reason, ("Audiofout", a.reason))
            return msg, ""
        warn = "Audio-aansluitingen niet gekoppeld" if a.reason == "ports-not-connected" else ""
        if a.faults:
            warn = "DSP hersteld na rekenfout"
        return None, warn

    def view(self, now: Optional[float] = None) -> ViewModel:
        now = self.clock() if now is None else now
        self._meters(now)
        st = self.link.state
        f = self.sound.fx
        fx = EFFECTS[f]
        tempo = st.tempo
        bpm = tempo.bpm if tempo.bpm > 0 else self.settings.manual_tempo

        tiles = []
        if self.page >= fx.pages:
            self.page = 0
        base = self.page * PARAMS_PER_PAGE
        for k in range(PARAMS_PER_PAGE):
            i = base + k
            pd = fx.params[i]
            v = self.sound.values[f][i]
            if pd.empty:
                tiles.append(TileVM("", "—", "", "", 0.0, False, False, k))
                continue
            synced = bool(self.sync and pd.sync)
            if synced:
                div = pd.divisions[self.sound.divs[f][i]]
                value, unit = div.label, ""
                if pd.sync == "time":
                    ms = div.beats * 60000.0 / bpm
                    while ms > pd.vmax:
                        ms /= 2
                    sub = f"{ms:.0f} ms"
                else:
                    sub = f"{bpm / 60.0 / div.beats:.2f} Hz"
                norm = self.sound.divs[f][i] / max(1, len(pd.divisions) - 1)
            else:
                value, unit = pd.format(v)
                sub = ""
                norm = pd.to_norm(v)
            tiles.append(TileVM(pd.label, value, unit, sub, norm, self.highlight == i, synced, k))

        bank = self.lib.find_bank(self.bank_id)
        preset_line = f"{self.bank_id}·{self.preset_idx + 1}  {self.preset_name.upper()}" if bank else self.preset_name.upper()

        if tempo.receiving and tempo.source not in ("tap", "int"):
            src = "MIDI DIN" if tempo.source == "din" else "MIDI USB"
        else:
            src = "TAP" if tempo.source == "tap" else "INT"

        ref = self.settings.vu_ref_dbfs
        vu_db = []
        for c in range(2):
            lin = self.vu_disp[c]
            vu_db.append((20 * math.log10(lin) - ref) if lin > 1e-6 else -60.0)

        overlay = None
        if self.mode == "chain":
            items = []
            for pos, fi in enumerate(CHAIN_ORDER):
                items.append((f"{pos + 1:02d}  {EFFECTS[fi].name}", "AAN" if self.sound.enabled[fi] else "uit"))
            overlay = OverlayVM("chain", "EFFECTKETEN", items, self.cursor,
                                "K1 kies · K2 aan/uit · druk K1 = tonen · S-knop sluit")
        elif self.mode == "save":
            slots = self.lib.user_slots()
            items = []
            for b, s in slots:
                p = self.lib.find_bank(b).presets[s]
                items.append((f"{b}·{s + 1}", p.name if p else "(leeg)"))
            overlay = OverlayVM("save", "OPSLAAN ALS", items, self.cursor,
                                "Draai K1 · druk K1 = opslaan · K3 of S-knop annuleert")
        elif self.mode == "settings":
            overlay = OverlayVM("settings", "INSTELLINGEN", self._settings_items(), self.cursor,
                                "K1 kiest · K4 wijzigt · druk K1 of K3 om te sluiten")

        fault, warn = self._fault(now)
        if self.hw_warning:
            warn = self.hw_warning

        chain = [(EFFECTS[fi].short, self.sound.enabled[fi], fi == f) for fi in CHAIN_ORDER]
        return ViewModel(
            fx_index=f, fx_pos=CHAIN_ORDER.index(f), fx_name=fx.name, fx_on=self.sound.enabled[f],
            page=self.page, pages=fx.pages, gr_db=getattr(st.meters, "gr", 0.0),
            chain=chain, mono_in=getattr(st.meters, "mono", False), preset_line=preset_line, dirty=self.dirty,
            bpm_text=f"{bpm:.1f}", tempo_source=src, clock_rx=tempo.receiving, sync=self.sync,
            bypass=self.bypass, tap_flash=now < self.tap_flash_until, tiles=tiles,
            vu_db=(vu_db[0], vu_db[1]),
            peak_text=(fmt_dbfs(self.peak_hold[0]), fmt_dbfs(self.peak_hold[1])),
            clip=(now < self.clip_until[0], now < self.clip_until[1]), in_clip=now < self.in_clip_until,
            vu_ref_text=f"0 VU = {ref:.0f} dBFS".replace("-", "−"),
            overlay=overlay, toast=self.toast_text if now < self.toast_until else "",
            fault=fault, warning=warn, simulated=st.simulated,
        )
