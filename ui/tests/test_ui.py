"""Unit tests for the UI process (no hardware needed). Run: python3 -m unittest discover -s tests"""
import json
import os
import queue
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from fxbox.controller import LONG_PRESS_S, Controller  # noqa: E402
from fxbox.engine_link import LinkState, parse_line  # noqa: E402
from fxbox.inputs import Debouncer, InputConfig, McpInputs, QuadratureDecoder  # noqa: E402
from fxbox.model import CHAIN_ORDER, EFFECTS, TIME_DIVISIONS  # noqa: E402
from fxbox.storage import DebouncedState, PresetLibrary  # noqa: E402

FACTORY = os.path.join(os.path.dirname(__file__), "..", "..", "presets", "factory.json")


class FakeLink:
    def __init__(self):
        self.state = LinkState(connected=True)
        self.sent = []

    def send(self, line):
        self.sent.append(line)

    def poll(self):
        return []


class Clock:
    def __init__(self):
        self.t = 1000.0

    def __call__(self):
        return self.t


def make(tmp=None):
    tmp = tmp or tempfile.mkdtemp()
    lib = PresetLibrary(FACTORY, os.path.join(tmp, "user.json"))
    clk = Clock()
    store = DebouncedState(os.path.join(tmp, "state.json"))
    link = FakeLink()
    return Controller(link, lib, store, clock=clk), link, clk, tmp


def press(ctl, clk, kind, idx, hold=0.05):
    ctl.handle((kind, idx, True, clk.t))
    clk.t += hold
    ctl.tick(clk.t)
    ctl.handle((kind, idx, False, clk.t))
    clk.t += 0.3


class QuadratureTests(unittest.TestCase):
    # Gray sequence for one detent clockwise, starting and ending at rest (A=B=1).
    CW = [(1, 1), (0, 1), (0, 0), (1, 0), (1, 1)]

    def run_seq(self, dec, seq):
        return sum(dec.feed(a, b) for a, b in seq)

    def test_full_step_cw_ccw(self):
        d = QuadratureDecoder()
        self.assertEqual(self.run_seq(d, self.CW), 1)
        self.assertEqual(self.run_seq(d, list(reversed(self.CW))), -1)

    def test_bounce_is_ignored(self):
        d = QuadratureDecoder()
        bouncy = [(1, 1), (0, 1), (1, 1), (0, 1), (0, 0), (0, 1), (0, 0), (1, 0), (1, 1)]
        self.assertEqual(self.run_seq(d, bouncy), 1)

    def test_partial_turn_gives_nothing(self):
        d = QuadratureDecoder()
        self.assertEqual(self.run_seq(d, [(1, 1), (0, 1), (0, 0), (0, 1), (1, 1)]), 0)

    def test_reverse(self):
        d = QuadratureDecoder(reverse=True)
        self.assertEqual(self.run_seq(d, self.CW), -1)


class DebounceTests(unittest.TestCase):
    def test_stable_level_required(self):
        db = Debouncer(stable_s=0.012)
        self.assertIsNone(db.feed(True, 0.000))
        self.assertIsNone(db.feed(False, 0.003))   # bounce
        self.assertIsNone(db.feed(True, 0.005))
        self.assertIsNone(db.feed(True, 0.010))
        self.assertTrue(db.feed(True, 0.018))
        self.assertIsNone(db.feed(True, 0.030))


class FakeBus:
    def __init__(self):
        self.word = 0xFFFF
        self.writes = []

    def read_i2c_block_data(self, addr, reg, n):
        return [self.word & 0xFF, self.word >> 8]

    def write_byte_data(self, addr, reg, val):
        self.writes.append((reg, val))


class McpProcessTests(unittest.TestCase):
    def test_v4_pin_mapping(self):
        q = queue.Queue()
        m = McpInputs(InputConfig(), q)
        cfg = m.cfg
        t = 0.0

        def word(a=1, b=1, k=0, sw_pressed=None, s_pressed=None):
            w = 0xFFFF
            if not a:
                w &= ~(1 << cfg.enc_a[k])
            if not b:
                w &= ~(1 << cfg.enc_b[k])
            if sw_pressed is not None:
                w &= ~(1 << cfg.enc_sw[sw_pressed])
            if s_pressed is not None:
                w &= ~(1 << cfg.switches[s_pressed])
            return w
        for a, b in [(1, 1), (0, 1), (0, 0), (1, 0), (1, 1)]:
            t += 0.002
            m._process(word(a, b, k=2), t)
        for _ in range(10):
            t += 0.002
            m._process(word(s_pressed=3), t)
        evs = []
        while not q.empty():
            evs.append(q.get())
        self.assertIn(("turn", 2, 1), [e[:3] for e in evs])
        self.assertIn(("switch", 3, True), [e[:3] for e in evs])


class ControllerTests(unittest.TestCase):
    def test_turn_edits_matching_parameter(self):
        ctl, link, clk, _ = make()
        f = ctl.sound.fx
        before = ctl.sound.values[f][1]
        ctl.handle(("turn", 1, 1, clk.t))
        self.assertGreater(ctl.sound.values[f][1], before)
        self.assertTrue(any(s.startswith(f"p {f} 1 ") for s in link.sent))
        self.assertEqual(ctl.highlight, 1)
        self.assertTrue(ctl.dirty)

    def test_turn_ignored_while_pressed_and_just_after(self):
        ctl, link, clk, _ = make()
        f = ctl.sound.fx
        v = list(ctl.sound.values[f])
        ctl.handle(("knob", 3, True, clk.t))         # K4 = tap
        ctl.handle(("turn", 3, 1, clk.t + 0.01))
        ctl.handle(("knob", 3, False, clk.t + 0.05))
        ctl.handle(("turn", 3, 1, clk.t + 0.07))     # within the suppression window
        self.assertEqual(ctl.sound.values[f], v)
        ctl.handle(("turn", 3, 1, clk.t + 0.5))
        self.assertNotEqual(ctl.sound.values[f], v)

    def test_k2_short_toggles_effect_long_bypasses_chain(self):
        ctl, link, clk, _ = make()
        f = ctl.sound.fx
        was = ctl.sound.enabled[f]
        press(ctl, clk, "knob", 1)                    # short: shown effect on/off
        self.assertEqual(ctl.sound.enabled[f], not was)
        self.assertIn(f"on {f} {0 if was else 1}", link.sent)
        self.assertFalse(ctl.bypass)
        press(ctl, clk, "knob", 1, hold=LONG_PRESS_S + 0.1)   # long: whole chain bypass
        self.assertTrue(ctl.bypass)
        self.assertIn("bypass 1", link.sent)
        self.assertEqual(ctl.sound.enabled[f], not was)        # long press did not toggle the effect

    def test_tap_acts_on_press(self):
        ctl, link, clk, _ = make()
        ctl.handle(("knob", 3, True, clk.t))
        self.assertTrue(any(s.startswith("tap ") for s in link.sent))

    def test_chain_view_flow(self):
        ctl, link, clk, _ = make()
        press(ctl, clk, "knob", 0)
        self.assertEqual(ctl.mode, "chain")
        for _ in range(7):
            ctl.handle(("turn", 0, -1, clk.t))
            clk.t += 0.2
        self.assertEqual(ctl.cursor, 0)                        # first in chain: Filter
        press(ctl, clk, "knob", 1)                             # K2 in chain view: toggle highlighted
        filt = CHAIN_ORDER[0]
        self.assertTrue(ctl.sound.enabled[filt])
        self.assertIn(f"on {filt} 1", link.sent)
        press(ctl, clk, "knob", 0)                             # K1: show it
        self.assertEqual(ctl.mode, "play")
        self.assertEqual(ctl.sound.fx, filt)
        vm = ctl.view(clk.t)
        self.assertEqual(vm.fx_name, "FILTER")
        self.assertTrue(vm.fx_on)
        self.assertEqual(sum(1 for c in vm.chain if c[1]), 2)  # tape echo + filter

    def test_chain_preset_roundtrip(self):
        ctl, link, clk, tmp = make()
        ctl.sound.enabled = [False] * len(EFFECTS)
        for f in (6, 4, 1):                                    # filter, distortion, reverb
            ctl.sound.enabled[f] = True
        ctl.sound.values[4][0] = 0.9
        press(ctl, clk, "knob", 0, hold=LONG_PRESS_S + 0.1)
        press(ctl, clk, "knob", 0)
        lib2 = PresetLibrary(FACTORY, os.path.join(tmp, "user.json"))
        p = lib2.find_bank("U1").presets[0]
        self.assertEqual([i for i, e in enumerate(p.enabled) if e], [1, 4, 6])
        self.assertAlmostEqual(p.values[4][0], 0.9)
        ctl.handle(("switch", 3, True, clk.t))                 # another bank, then load the user preset again
        ctl._load_preset(lib2.find_bank("U1"), 0)
        self.assertEqual([i for i, e in enumerate(ctl.sound.enabled) if e], [1, 4, 6])

    def test_legacy_state_enables_shown_effect_only(self):
        tmp = tempfile.mkdtemp()
        with open(os.path.join(tmp, "state.json"), "w") as f:
            json.dump({"version": 1, "fx": "reverb", "bank": "F1", "preset": 2}, f)
        ctl, _, _, _ = make(tmp)
        self.assertEqual([i for i, e in enumerate(ctl.sound.enabled) if e], [1])

    def test_long_press_opens_save_and_saves(self):
        ctl, link, clk, tmp = make()
        press(ctl, clk, "knob", 0, hold=LONG_PRESS_S + 0.1)
        self.assertEqual(ctl.mode, "save")
        press(ctl, clk, "knob", 0)
        self.assertEqual(ctl.mode, "play")
        data = json.load(open(os.path.join(tmp, "user.json")))
        self.assertIsNotNone(data["banks"][0]["presets"][0])
        self.assertEqual(ctl.bank_id, "U1")
        self.assertFalse(ctl.dirty)
        # The user bank becomes navigable.
        self.assertIn("U1", [b.id for b in ctl.lib.navigable_banks()])

    def test_switches_navigate_presets_and_banks(self):
        ctl, link, clk, _ = make()
        press(ctl, clk, "switch", 1)
        self.assertEqual((ctl.bank_id, ctl.preset_idx), ("F1", 1))
        press(ctl, clk, "switch", 0)
        press(ctl, clk, "switch", 0)
        self.assertEqual((ctl.bank_id, ctl.preset_idx), ("F1", 5))     # wraps within the bank
        press(ctl, clk, "switch", 3)
        self.assertEqual(ctl.bank_id, "F2")
        press(ctl, clk, "switch", 2)
        press(ctl, clk, "switch", 2)
        self.assertEqual(ctl.bank_id, "F7")                            # wraps over banks (7 factory banks)

    def test_sync_turns_select_divisions(self):
        ctl, link, clk, _ = make()
        press(ctl, clk, "knob", 2)                                     # sync on
        self.assertTrue(ctl.sync)
        f = ctl.sound.fx
        self.assertEqual(EFFECTS[f].key, "tape")
        i0 = ctl.sound.divs[f][0]
        ctl.handle(("turn", 0, 1, clk.t))
        self.assertEqual(ctl.sound.divs[f][0], i0 + 1)
        beats = TIME_DIVISIONS[i0 + 1].beats
        self.assertIn(f"d {f} 0 {beats:.6f}", link.sent)

    def test_tap_without_sync_sets_echo_time(self):
        ctl, link, clk, _ = make()
        ctl.engine_event(("tap", 120.0))
        f = ctl.sound.fx
        self.assertAlmostEqual(ctl.sound.values[f][0], 500.0)

    def test_state_is_written_debounced_and_restored(self):
        ctl, link, clk, tmp = make()
        for i in range(20):
            clk.t += 0.05
            ctl.handle(("turn", 2, 1, clk.t))
            ctl.tick(clk.t)
        self.assertEqual(ctl.store.writes, 0)                           # still turning: nothing written
        clk.t += 2.5
        ctl.tick(clk.t)
        self.assertEqual(ctl.store.writes, 1)
        mix = ctl.sound.values[ctl.sound.fx][2]
        ctl2, _, _, _ = make(tmp)
        self.assertAlmostEqual(ctl2.sound.values[ctl2.sound.fx][2], mix, places=5)
        self.assertTrue(ctl2.dirty)

    def test_corrupt_state_falls_back(self):
        tmp = tempfile.mkdtemp()
        with open(os.path.join(tmp, "state.json"), "w") as f:
            f.write("{not json")
        ctl, _, _, _ = make(tmp)
        self.assertEqual(ctl.bank_id, "F1")
        self.assertTrue(os.path.exists(os.path.join(tmp, "state.json.bad")))

    def test_resend_everything_on_engine_connect(self):
        ctl, link, clk, _ = make()
        ctl.engine_event(("connected",))
        self.assertTrue(any(s.startswith("fx ") for s in link.sent))
        expected = sum(1 for e in EFFECTS for p in e.params if not p.empty)   # page-2 params included
        self.assertEqual(sum(1 for s in link.sent if s.startswith("p ")), expected)
        self.assertEqual(sum(1 for s in link.sent if s.startswith("on ")), len(EFFECTS))

    def test_fault_messages(self):
        ctl, link, clk, _ = make()
        link.state.audio.state = "error"
        link.state.audio.reason = "no-card"
        vm = ctl.view(clk.t)
        self.assertIn("audio-interface", vm.fault[0])
        link.state.connected = False
        clk.t += 20
        vm = ctl.view(clk.t)
        self.assertIn("engine", vm.fault[0])


class ProtocolTests(unittest.TestCase):
    def test_parse(self):
        st = LinkState()
        parse_line("m 0.1 0.2 0.5 0.6 3 0.1 0.1 0 1", st)
        self.assertEqual(st.meters.out_clips, 3)
        self.assertTrue(st.meters.mono)
        parse_line("t 174.0 din 1 1", st)
        self.assertTrue(st.tempo.receiving and st.tempo.sync and st.tempo.source == "din")
        parse_line("srcs din|usb:Digitakt", st)
        self.assertEqual(st.sources, ["din", "usb:Digitakt"])
        self.assertEqual(parse_line("tap 121.5", st), ("tap", 121.5))
        parse_line("a error no-card 0 0 0 0.0 0", st)
        self.assertEqual(st.audio.reason, "no-card")


FX = {e.key: i for i, e in enumerate(EFFECTS)}


def show(ctl, key):
    ctl._show_fx(FX[key])


class PageTests(unittest.TestCase):
    """Version 1.2: second parameter page, mode selectors, compressor."""

    def test_k4_hold_toggles_page_and_knobs_follow(self):
        ctl, link, clk, _ = make()
        show(ctl, "filter")
        press(ctl, clk, "knob", 3, hold=LONG_PRESS_S + 0.1)
        self.assertEqual(ctl.page, 1)
        vm = ctl.view(clk.t)
        self.assertEqual(vm.page, 1)
        self.assertEqual([t.label for t in vm.tiles], ["TYPE", "LFO VORM", "ENVELOPE", "DRIVE"])
        link.sent.clear()
        ctl.handle(("turn", 0, 1, clk.t))         # K1 on page 2 = filter type
        self.assertIn(f"p {FX['filter']} 4 1.000000", link.sent)
        self.assertEqual(ctl.view(clk.t).tiles[0].value, "LOW-PASS")
        press(ctl, clk, "knob", 3, hold=LONG_PRESS_S + 0.1)
        self.assertEqual(ctl.page, 0)

    def test_enum_steps_one_per_detent_and_clamps(self):
        ctl, link, clk, _ = make()
        show(ctl, "distortion")
        ctl.page = 1
        for _ in range(10):                       # fast turning: still one option per detent
            ctl.handle(("turn", 0, 1, clk.t))
            clk.t += 0.01
        self.assertEqual(ctl.sound.values[FX["distortion"]][4], 4.0)
        self.assertEqual(ctl.view(clk.t).tiles[0].value, "FOLDER")
        ctl.handle(("turn", 0, -1, clk.t + 0.5))
        self.assertEqual(ctl.sound.values[FX["distortion"]][4], 3.0)

    def test_effect_without_page_two(self):
        ctl, link, clk, _ = make()
        show(ctl, "reverb")
        press(ctl, clk, "knob", 3, hold=LONG_PRESS_S + 0.1)
        self.assertEqual(ctl.page, 0)
        self.assertIn("pagina 2", ctl.view(clk.t).toast.lower())

    def test_empty_slot_is_inert(self):
        ctl, link, clk, _ = make()
        show(ctl, "distortion")
        ctl.page = 1
        vm = ctl.view(clk.t)
        self.assertEqual(vm.tiles[3].label, "")
        link.sent.clear()
        ctl.handle(("turn", 3, 1, clk.t))
        self.assertFalse(any(s.startswith("p ") for s in link.sent))

    def test_switching_effect_returns_to_page_one(self):
        ctl, link, clk, _ = make()
        show(ctl, "filter")
        ctl.page = 1
        show(ctl, "compressor")
        self.assertEqual(ctl.page, 0)

    def test_state_from_v11_gets_page_two_defaults(self):
        tmp = tempfile.mkdtemp()
        with open(os.path.join(tmp, "state.json"), "w") as f:
            json.dump({"version": 1, "fx": "filter", "bank": "F1", "preset": 0, "enabled": ["filter"],
                       "values": {"filter": [500, 0.5, 1.0, 0.2]}}, f)
        ctl, _, _, _ = make(tmp)
        v = ctl.sound.values[FX["filter"]]
        self.assertEqual(len(v), 8)
        self.assertAlmostEqual(v[0], 500)
        self.assertEqual(v[4], 0.0)                  # ladder, the 1.1 behaviour

    def test_compressor_is_last_and_shows_gain_reduction(self):
        self.assertEqual(EFFECTS[CHAIN_ORDER[-1]].key, "compressor")
        st = LinkState()
        parse_line("m 0.1 0.1 0.2 0.2 0 0.1 0.1 0 0 6.40", st)
        self.assertAlmostEqual(st.meters.gr, 6.4)
        parse_line("m 0.1 0.1 0.2 0.2 0 0.1 0.1 0 0", st)   # engine 1.1: no GR field
        self.assertEqual(st.meters.gr, 0.0)

    def test_page_two_values_survive_a_user_preset(self):
        ctl, link, clk, tmp = make()
        ctl.sound.enabled = [False] * len(EFFECTS)
        ctl.sound.enabled[FX["filter"]] = True
        ctl.sound.values[FX["filter"]][4] = 3.0          # band-pass
        ctl.sound.values[FX["filter"]][6] = 0.7          # envelope
        press(ctl, clk, "knob", 0, hold=LONG_PRESS_S + 0.1)
        press(ctl, clk, "knob", 0)
        lib2 = PresetLibrary(FACTORY, os.path.join(tmp, "user.json"))
        p = lib2.find_bank("U1").presets[0]
        self.assertEqual(p.values[FX["filter"]][4], 3.0)
        self.assertAlmostEqual(p.values[FX["filter"]][6], 0.7)


if __name__ == "__main__":
    unittest.main()
