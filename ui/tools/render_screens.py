#!/usr/bin/env python3
"""Renders reference screenshots of the interface (480 x 320, and 2x) to PNG.

Uses the simulated engine; the screenshots are therefore marked "SIM".
    python3 tools/render_screens.py OUTPUT_DIR
"""
import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import cairo  # noqa: E402

from fxbox.controller import Controller  # noqa: E402
from fxbox.engine_link import SimEngine  # noqa: E402
from fxbox.render import H, W, Renderer  # noqa: E402
from fxbox.storage import DebouncedState, PresetLibrary  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def make():
    tmp = tempfile.mkdtemp()
    lib = PresetLibrary(os.path.join(ROOT, "presets", "factory.json"), os.path.join(tmp, "user.json"))
    clock = [100.0]
    sim = SimEngine(clock=lambda: clock[0])
    ctl = Controller(sim, lib, DebouncedState(os.path.join(tmp, "state.json")), clock=lambda: clock[0])
    for ev in sim.poll():
        ctl.engine_event(ev)
    return ctl, sim, clock


def settle(ctl, sim, clock, frames=25, phase=0.0):
    sim._t0 = clock[0] - phase
    for _ in range(frames):
        clock[0] += 1 / 30
        for ev in sim.poll():
            ctl.engine_event(ev)
        ctl.tick(clock[0])
        vm = ctl.view(clock[0])
    return vm


def save(vm, out_dir, name, renderer):
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
    renderer.render(surf, vm)
    surf.write_to_png(os.path.join(out_dir, f"{name}.png"))
    big = cairo.ImageSurface(cairo.FORMAT_ARGB32, W * 2, H * 2)
    c = cairo.Context(big)
    c.scale(2, 2)
    c.set_source_surface(surf, 0, 0)
    c.get_source().set_filter(cairo.FILTER_NEAREST)
    c.paint()
    big.write_to_png(os.path.join(out_dir, f"{name}@2x.png"))
    print("wrote", name)


def press(ctl, clock, kind, idx, hold=0.05):
    ctl.handle((kind, idx, True, clock[0]))
    clock[0] += hold
    ctl.tick(clock[0])
    ctl.handle((kind, idx, False, clock[0]))
    clock[0] += 0.2


def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    r = Renderer()

    ctl, sim, clock = make()
    press(ctl, clock, "switch", 3)          # bank F2
    clock[0] += 2
    ctl.handle(("turn", 1, 2, clock[0]))    # feedback edit -> highlighted tile
    save(settle(ctl, sim, clock, phase=0.05), out_dir, "01_tape_echo", r)

    ctl, sim, clock = make()
    press(ctl, clock, "switch", 3)
    press(ctl, clock, "knob", 2)            # sync on
    sim.sim_clock = True                    # simulated MIDI clock at 174 BPM
    sim.sync = True
    clock[0] += 2
    save(settle(ctl, sim, clock, phase=0.25), out_dir, "02_sync_midi_clock", r)

    ctl, sim, clock = make()
    press(ctl, clock, "switch", 3)
    press(ctl, clock, "switch", 3)          # bank F3
    press(ctl, clock, "switch", 1)
    press(ctl, clock, "switch", 1)          # reverb "Wolk"
    clock[0] += 2
    ctl.handle(("turn", 0, -1, clock[0]))
    save(settle(ctl, sim, clock, phase=0.6), out_dir, "03_reverb", r)

    ctl, sim, clock = make()
    press(ctl, clock, "knob", 0)            # effect choice
    ctl.handle(("turn", 0, 2, clock[0] + 0.3))
    save(settle(ctl, sim, clock, 3), out_dir, "04_effectketen", r)

    ctl, sim, clock = make()
    press(ctl, clock, "knob", 2, hold=1.0)  # settings
    ctl.handle(("turn", 0, 2, clock[0] + 0.3))
    save(settle(ctl, sim, clock, 3), out_dir, "05_instellingen", r)

    ctl, sim, clock = make()
    ctl.handle(("turn", 3, 3, clock[0] + 0.5))
    press(ctl, clock, "knob", 0, hold=1.0)  # save
    save(settle(ctl, sim, clock, 3), out_dir, "06_opslaan", r)

    ctl, sim, clock = make()
    press(ctl, clock, "switch", 3)
    press(ctl, clock, "switch", 3)
    press(ctl, clock, "switch", 3)          # F4 modulation
    press(ctl, clock, "knob", 1)            # bypass
    save(settle(ctl, sim, clock, phase=0.1), out_dir, "07_bypass", r)

    ctl, sim, clock = make()
    for _ in range(5):
        press(ctl, clock, "switch", 3)      # bank F6 "Ketens"
    clock[0] += 2
    ctl.handle(("turn", 0, -2, clock[0]))
    save(settle(ctl, sim, clock, phase=0.3), out_dir, "09_keten", r)

    ctl, sim, clock = make()
    for _ in range(4):
        press(ctl, clock, "switch", 3)      # bank F5 "Drive & Filter"
    press(ctl, clock, "switch", 0)          # last preset: filter wobble
    press(ctl, clock, "knob", 1)            # K2 short: shown effect off
    save(settle(ctl, sim, clock, phase=0.5), out_dir, "10_effect_uit", r)

    # Version 1.2: page 2 of the filter, and the compressor with gain reduction.
    ctl, sim, clock = make()
    for _ in range(6):
        press(ctl, clock, "switch", 3)      # bank F7 "Karakter"
    press(ctl, clock, "knob", 3, hold=1.0)  # K4 hold: page 2
    clock[0] += 2
    ctl.handle(("turn", 0, 0, clock[0]))
    ctl.highlight = 4
    save(settle(ctl, sim, clock, phase=0.4), out_dir, "11_filter_pagina2", r)

    ctl, sim, clock = make()
    for _ in range(6):
        press(ctl, clock, "switch", 3)      # bank F7
    press(ctl, clock, "switch", 0)          # last preset: parallel squash
    clock[0] += 2
    save(settle(ctl, sim, clock, phase=0.02), out_dir, "12_compressor", r)

    ctl, sim, clock = make()
    for _ in range(6):
        press(ctl, clock, "switch", 3)      # bank F7
    for _ in range(4):
        press(ctl, clock, "switch", 1)      # "Wavefolder"
    press(ctl, clock, "knob", 3, hold=1.0)
    clock[0] += 2
    ctl.handle(("turn", 0, 1, clock[0]))    # type -> the next mode
    ctl.handle(("turn", 0, -1, clock[0] + 0.2))
    save(settle(ctl, sim, clock, phase=0.15), out_dir, "13_distortion_type", r)

    ctl, sim, clock = make()
    sim.state.audio.state = "error"
    sim.state.audio.reason = "no-card"
    save(settle(ctl, sim, clock, 3), out_dir, "08_fout_geen_geluidskaart", r)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "screens")
