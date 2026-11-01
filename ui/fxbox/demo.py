"""Desktop demo: the real interface code with simulated controls and meters.

    python3 -m fxbox.demo                 # simulated engine (no audio)
    python3 -m fxbox.demo --engine SOCK   # connect to a real fxbox-engine (JACK on the desktop)

SIMULATED in the default mode: meters (a synthetic drum & bass level pattern),
tempo/tap handling and the MIDI clock (toggle with M). The screen shows "SIM".
The knobs and buttons are drawn around the screen like on the V4 front panel.

Mouse:   wheel over a knob = turn, click a knob = press (hold for long press),
         click S1-S4.
Keys:    Q/A  W/S  E/D  R/F  turn K1..K4 (left/right)
         1 2 3 4  press K1..K4 (hold for long press)
         Z X C V  S1..S4      M  simulated MIDI clock on/off
         Esc      quit
"""
from __future__ import annotations

import argparse
import os
import sys
import tempfile
import time

import cairo

from .controller import Controller
from .engine_link import EngineLink, SimEngine
from .render import H, W, Renderer
from .storage import DebouncedState, PresetLibrary

SCALE = 2
SIDE = 150
BOTTOM = 150
WIN_W = W * SCALE + 2 * SIDE
WIN_H = H * SCALE + BOTTOM + 40

KNOB_POS = {0: (SIDE // 2, 40 + int(H * SCALE * 0.25)), 1: (SIDE // 2, 40 + int(H * SCALE * 0.75)),
            2: (WIN_W - SIDE // 2, 40 + int(H * SCALE * 0.25)), 3: (WIN_W - SIDE // 2, 40 + int(H * SCALE * 0.75))}
KNOB_FUNC = {0: ("K1", "keten · houd: opslaan"), 1: ("K2", "aan/uit · houd: bypass"),
             2: ("K3", "sync · houd: instellingen"), 3: ("K4", "tap · houd: pagina 2")}
SW_FUNC = ["S1  vorige preset", "S2  volgende preset", "S3  vorige bank", "S4  volgende bank"]
TURN_KEYS = {"q": (0, -1), "a": (0, 1), "w": (1, -1), "s": (1, 1), "e": (2, -1), "d": (2, 1), "r": (3, -1), "f": (3, 1)}
PRESS_KEYS = {"1": 0, "2": 1, "3": 2, "4": 3}
SW_KEYS = {"z": 0, "x": 1, "c": 2, "v": 3}


def sw_rect(i):
    import pygame
    bw, gap = 200, 20
    total = 4 * bw + 3 * gap
    x0 = (WIN_W - total) // 2
    return pygame.Rect(x0 + i * (bw + gap), 40 + H * SCALE + 30, bw, 44)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="FXBox desktop demo")
    ap.add_argument("--engine", help="connect to a real fxbox-engine socket instead of the simulation")
    ap.add_argument("--data-dir", default=os.path.join(tempfile.gettempdir(), "fxbox-demo"))
    ap.add_argument("--factory", default=os.path.join(os.path.dirname(__file__), "..", "..", "presets", "factory.json"))
    ap.add_argument("--frames", type=int, default=0, help="quit after N frames (testing)")
    args = ap.parse_args(argv)

    os.environ.setdefault("PYGAME_HIDE_SUPPORT_PROMPT", "1")
    import pygame

    os.makedirs(args.data_dir, exist_ok=True)
    lib = PresetLibrary(os.path.abspath(args.factory), os.path.join(args.data_dir, "user_presets.json"))
    store = DebouncedState(os.path.join(args.data_dir, "state.json"))
    link = EngineLink(args.engine) if args.engine else SimEngine()
    link.start()
    ctl = Controller(link, lib, store)
    renderer = Renderer()
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)

    pygame.init()
    win = pygame.display.set_mode((WIN_W, WIN_H))
    pygame.display.set_caption("FXBox demo — " + ("SIMULATIE (geen audio)" if not args.engine else "echte engine"))
    small = pygame.font.SysFont("Inter,DejaVu Sans", 15)
    tiny = pygame.font.SysFont("Inter,DejaVu Sans", 13)
    clock = pygame.time.Clock()
    pressed_knob = None
    pressed_sw = None
    frames = 0
    running = True
    while running:
        now = time.monotonic()
        for e in pygame.event.get():
            if e.type == pygame.QUIT:
                running = False
            elif e.type == pygame.KEYDOWN:
                name = pygame.key.name(e.key)
                if e.key == pygame.K_ESCAPE:
                    running = False
                elif name in TURN_KEYS:
                    k, d = TURN_KEYS[name]
                    ctl.handle(("turn", k, d, now))
                elif name in PRESS_KEYS and not e.mod & pygame.KMOD_SHIFT:
                    ctl.handle(("knob", PRESS_KEYS[name], True, now))
                elif name in SW_KEYS:
                    ctl.handle(("switch", SW_KEYS[name], True, now))
                elif name == "m" and isinstance(link, SimEngine):
                    link.sim_clock = not link.sim_clock
            elif e.type == pygame.KEYUP:
                name = pygame.key.name(e.key)
                if name in PRESS_KEYS:
                    ctl.handle(("knob", PRESS_KEYS[name], False, now))
                elif name in SW_KEYS:
                    ctl.handle(("switch", SW_KEYS[name], False, now))
            elif e.type == pygame.MOUSEWHEEL:
                mx, my = pygame.mouse.get_pos()
                for k, (kx, ky) in KNOB_POS.items():
                    if (mx - kx) ** 2 + (my - ky) ** 2 < 60 ** 2:
                        ctl.handle(("turn", k, 1 if e.y > 0 else -1, now))
            elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1:
                for k, (kx, ky) in KNOB_POS.items():
                    if (e.pos[0] - kx) ** 2 + (e.pos[1] - ky) ** 2 < 45 ** 2:
                        pressed_knob = k
                        ctl.handle(("knob", k, True, now))
                for i in range(4):
                    if sw_rect(i).collidepoint(e.pos):
                        pressed_sw = i
                        ctl.handle(("switch", i, True, now))
            elif e.type == pygame.MOUSEBUTTONUP and e.button == 1:
                if pressed_knob is not None:
                    ctl.handle(("knob", pressed_knob, False, now))
                    pressed_knob = None
                if pressed_sw is not None:
                    ctl.handle(("switch", pressed_sw, False, now))
                    pressed_sw = None

        for ev in link.poll():
            ctl.engine_event(ev)
        ctl.tick(now)
        renderer.render(surf, ctl.view(now))

        win.fill((22, 24, 28))
        img = pygame.image.frombuffer(bytes(surf.get_data()), (W, H), "BGRA")
        win.blit(pygame.transform.scale(img, (W * SCALE, H * SCALE)), (SIDE, 40))
        pygame.draw.rect(win, (60, 64, 72), (SIDE - 4, 36, W * SCALE + 8, H * SCALE + 8), 2, border_radius=6)
        banner = "SIMULATIE: meters, tempo en MIDI-clock zijn nagebootst — er wordt geen audio verwerkt" \
            if isinstance(link, SimEngine) else "Verbonden met fxbox-engine (" + str(args.engine) + ")"
        win.blit(small.render(banner, True, (255, 176, 32)), (SIDE, 10))
        for k, (kx, ky) in KNOB_POS.items():
            down = ctl.knob_down[k] is not None
            pygame.draw.circle(win, (48, 52, 60), (kx, ky), 42)
            pygame.draw.circle(win, (255, 176, 32) if down else (90, 96, 106), (kx, ky), 42, 3)
            lab, func = KNOB_FUNC[k]
            t = small.render(lab, True, (244, 241, 234))
            win.blit(t, (kx - t.get_width() // 2, ky - 9))
            for j, part in enumerate(func.split(" · ")):
                ft = tiny.render(part, True, (139, 145, 155))
                win.blit(ft, (kx - ft.get_width() // 2, ky + 50 + j * 16))
        for i in range(4):
            r = sw_rect(i)
            pygame.draw.rect(win, (48, 52, 60), r, border_radius=10)
            pygame.draw.rect(win, (255, 176, 32) if pressed_sw == i else (90, 96, 106), r, 2, border_radius=10)
            t = small.render(SW_FUNC[i], True, (244, 241, 234))
            win.blit(t, (r.centerx - t.get_width() // 2, r.centery - t.get_height() // 2))
        help_text = "Toetsen: Q/A W/S E/D R/F draaien · 1-4 drukken (vasthouden = lang) · Z X C V = S1-S4 · M = MIDI-clock (sim)"
        ht = tiny.render(help_text, True, (139, 145, 155))
        win.blit(ht, ((WIN_W - ht.get_width()) // 2, WIN_H - 30))
        pygame.display.flip()
        clock.tick(30)
        frames += 1
        if args.frames and frames >= args.frames:
            running = False

    ctl.shutdown()
    pygame.quit()
    return 0


if __name__ == "__main__":
    sys.exit(main())
