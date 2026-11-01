"""Cairo renderer for the 480 x 320 ZynScreen.

Draws a ViewModel onto any cairo ImageSurface (RGB16_565 for the framebuffer,
ARGB32 for the desktop demo). Static meter faces are cached; everything else
is cheap to redraw each frame.
"""
from __future__ import annotations

import math

import cairo

from .controller import TileVM, ViewModel

W, H = 480, 320

# Palette (dark ground, amber accent, off-white text)
BG = (0x0A, 0x0B, 0x0D)
PANEL = (0x12, 0x14, 0x18)
PANEL_HI = (0x17, 0x14, 0x0E)
BORDER = (0x1E, 0x22, 0x28)
DIM = (0x2B, 0x2F, 0x36)
TRACK = (0x23, 0x27, 0x2E)
TEXT = (0xF4, 0xF1, 0xEA)
MUTED = (0x8B, 0x91, 0x9B)
SCALE = (0xD9, 0xD6, 0xCF)
SCALE_MINOR = (0x6B, 0x71, 0x7B)
LABEL = (0xB9, 0xB6, 0xAF)
FACE_TEXT = (0x34, 0x39, 0x41)
AMBER = (0xFF, 0xB0, 0x20)
RED = (0xFF, 0x4D, 0x2E)
RED_BG = (0x3A, 0x1A, 0x15)

SANS = "Inter"
MONO = "JetBrains Mono"

# Layout
MARGIN = 8
HEADER_H = 42
CHAIN_Y, CHAIN_H = 46, 18          # effect chain strip
PANEL_Y, PANEL_H = 46, 158         # area used by overlays and fault messages
METER_Y, METER_H = 68, 136
METER_W = (W - 2 * MARGIN - 8) / 2
TILE_Y, TILE_H = 210, 102
TILE_GAP = 6
TILE_W = (W - 2 * MARGIN - 3 * TILE_GAP) / 4

# VU face geometry (panel-local)
FACE_H = METER_H - 26
PIVOT_X = METER_W / 2
PIVOT_Y = FACE_H + 40
SCALE_R = PIVOT_Y - 38
SWEEP = 40.0   # degrees each side
VU_MAX_LIN = 10 ** (3 / 20)


def rgb(ctx, c, a=1.0):
    if a >= 1.0:
        ctx.set_source_rgb(c[0] / 255, c[1] / 255, c[2] / 255)
    else:
        ctx.set_source_rgba(c[0] / 255, c[1] / 255, c[2] / 255, a)


def rrect(ctx, x, y, w, h, r):
    ctx.new_sub_path()
    ctx.arc(x + w - r, y + r, r, -math.pi / 2, 0)
    ctx.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    ctx.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    ctx.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
    ctx.close_path()


def font(ctx, family, size, bold=False):
    ctx.select_font_face(family, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD if bold else cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(size)


def text(ctx, s, x, y, align="left", spacing=0.0):
    """Draw text with baseline at y. align: left|center|right. Returns width."""
    if spacing:
        w = sum(ctx.text_extents(ch).x_advance for ch in s) + spacing * max(0, len(s) - 1)
    else:
        w = ctx.text_extents(s).x_advance
    if align == "center":
        x -= w / 2
    elif align == "right":
        x -= w
    if spacing:
        for ch in s:
            ctx.move_to(x, y)
            ctx.show_text(ch)
            x += ctx.text_extents(ch).x_advance + spacing
    else:
        ctx.move_to(x, y)
        ctx.show_text(s)
    return w


def vu_pos(db: float) -> float:
    """Classic VU scale: position is linear in voltage, 0..1 spans -inf..+3 VU."""
    return (10 ** (db / 20.0)) / VU_MAX_LIN


def vu_angle(db: float) -> float:
    p = min(1.06, max(0.0, vu_pos(db)))
    return math.radians(-SWEEP + 2 * SWEEP * p)


class Renderer:
    def __init__(self):
        self._face = None

    # ------------------------------------------------------------ meter face

    def _build_face(self):
        surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, int(math.ceil(METER_W)), METER_H)
        ctx = cairo.Context(surf)
        rrect(ctx, 0.5, 0.5, METER_W - 1, METER_H - 1, 12)
        rgb(ctx, PANEL)
        ctx.fill_preserve()
        rgb(ctx, BORDER)
        ctx.set_line_width(1)
        ctx.stroke()

        def pt(r, a):
            return PIVOT_X + r * math.sin(a), PIVOT_Y - r * math.cos(a)

        # Base arc and red zone (0..+3 VU).
        a0, a1, a3 = vu_angle(-60), vu_angle(0), vu_angle(3)
        ctx.set_line_width(1.2)
        rgb(ctx, (0x4A, 0x50, 0x5A))
        ctx.arc(PIVOT_X, PIVOT_Y, SCALE_R + 12, a0 - math.pi / 2, a1 - math.pi / 2)
        ctx.stroke()
        rgb(ctx, RED)
        ctx.set_line_width(4)
        ctx.arc(PIVOT_X, PIVOT_Y, SCALE_R + 14, a1 - math.pi / 2, a3 - math.pi / 2)
        ctx.stroke()

        ctx.set_line_cap(cairo.LINE_CAP_ROUND)
        majors = [(-20, "20"), (-10, "10"), (-5, "5"), (-3, "3"), (0, "0"), (3, "3")]
        minors = [-7, -2, -1, 1, 2]
        for db in minors:
            a = vu_angle(db)
            ctx.move_to(*pt(SCALE_R + 4, a))
            ctx.line_to(*pt(SCALE_R + 12, a))
            rgb(ctx, RED if db > 0 else SCALE_MINOR)
            ctx.set_line_width(1.2)
            ctx.stroke()
        font(ctx, MONO, 10.5, bold=True)
        for db, lab in majors:
            a = vu_angle(db)
            ctx.move_to(*pt(SCALE_R - 2, a))
            ctx.line_to(*pt(SCALE_R + 12, a))
            rgb(ctx, RED if db > 0 else SCALE)
            ctx.set_line_width(2)
            ctx.stroke()
            x, y = pt(SCALE_R + 22, a)
            rgb(ctx, RED if db > 0 else (TEXT if db == 0 else LABEL))
            text(ctx, lab, x, y + 4, "center")
        font(ctx, MONO, 12)
        rgb(ctx, SCALE_MINOR)
        x, y = pt(SCALE_R + 20, vu_angle(-60) - 0.03)
        text(ctx, "−", x, y + 20, "center")
        rgb(ctx, RED)
        x, y = pt(SCALE_R + 20, vu_angle(3) + 0.03)
        text(ctx, "+", x, y + 20, "center")

        font(ctx, SANS, 17, bold=True)
        rgb(ctx, FACE_TEXT)
        text(ctx, "VU", PIVOT_X, FACE_H - 22, "center", spacing=2)

        # Bottom readout strip.
        ctx.move_to(1, FACE_H + 0.5)
        ctx.line_to(METER_W - 1, FACE_H + 0.5)
        rgb(ctx, BORDER)
        ctx.set_line_width(1)
        ctx.stroke()
        self._face = surf

    def _meter(self, ctx, x, label, db, peak_text, clip, ref_text):
        if self._face is None:
            self._build_face()
        ctx.save()
        ctx.translate(x, METER_Y)
        ctx.set_source_surface(self._face, 0, 0)
        ctx.paint()

        # Scale reference (VU is relative; digital levels are shown in dBFS).
        font(ctx, SANS, 8.5, bold=True)
        rgb(ctx, MUTED)
        text(ctx, ref_text, PIVOT_X, FACE_H - 8, "center")
        font(ctx, MONO, 11, bold=True)
        rgb(ctx, LABEL)
        text(ctx, label, METER_W - 12, 17, "right")

        # Needle (clipped to the face, pivot hidden below it).
        ctx.save()
        rrect(ctx, 1, 1, METER_W - 2, FACE_H - 1, 11)
        ctx.clip()
        a = vu_angle(db)
        tip = (PIVOT_X + (SCALE_R + 8) * math.sin(a), PIVOT_Y - (SCALE_R + 8) * math.cos(a))
        ctx.set_line_cap(cairo.LINE_CAP_ROUND)
        rgb(ctx, AMBER, 0.28)
        ctx.set_line_width(6)
        ctx.move_to(PIVOT_X, PIVOT_Y)
        ctx.line_to(*tip)
        ctx.stroke()
        rgb(ctx, AMBER)
        ctx.set_line_width(2)
        ctx.move_to(PIVOT_X, PIVOT_Y)
        ctx.line_to(*tip)
        ctx.stroke()
        ctx.restore()

        y = FACE_H + 17
        font(ctx, SANS, 8.5, bold=True)
        rgb(ctx, MUTED)
        text(ctx, "PEAK", 12, y - 1, spacing=0.8)
        font(ctx, MONO, 11.5)
        rgb(ctx, RED if clip else TEXT)
        w = text(ctx, peak_text, 44, y)
        font(ctx, MONO, 9)
        rgb(ctx, MUTED)
        text(ctx, "dBFS", 44 + w + 4, y)
        font(ctx, SANS, 8.5, bold=True)
        rgb(ctx, RED if clip else MUTED)
        text(ctx, "CLIP", METER_W - 26, y - 1, "right", spacing=0.8)
        ctx.arc(METER_W - 16, y - 4.5, 4.5, 0, 2 * math.pi)
        rgb(ctx, RED if clip else DIM)
        ctx.fill()
        ctx.restore()

    # ------------------------------------------------------------ header

    def _pill(self, ctx, x, y, w, h, label, fill=None, border=None, color=TEXT, size=10):
        rrect(ctx, x, y, w, h, h / 2 if h < 20 else 6)
        if fill:
            rgb(ctx, fill)
            ctx.fill_preserve()
        if border:
            rgb(ctx, border)
            ctx.set_line_width(1)
            ctx.stroke()
        else:
            ctx.new_path()
        font(ctx, SANS, size, bold=True)
        rgb(ctx, color)
        text(ctx, label, x + w / 2, y + h / 2 + size * 0.36, "center", spacing=0.9)

    def _header(self, ctx, vm: ViewModel):
        # Effect number chip and name.
        active = vm.fx_on and not vm.bypass
        rrect(ctx, MARGIN, 8, 34, 26, 6)
        rgb(ctx, AMBER if active else DIM)
        ctx.fill()
        font(ctx, MONO, 14, bold=True)
        rgb(ctx, BG if active else MUTED)
        text(ctx, f"{vm.fx_pos + 1:02d}", MARGIN + 17, 26, "center")

        font(ctx, SANS, 18 if len(vm.fx_name) <= 9 else 16, bold=True)
        rgb(ctx, TEXT if active else MUTED)
        name_w = text(ctx, vm.fx_name, 50, 23, spacing=0.6)
        if not vm.fx_on:
            self._pill(ctx, 50 + name_w + 8, 9, 30, 16, "UIT", border=MUTED, color=MUTED, size=8)
            name_w += 38
        font(ctx, SANS, 10, bold=True)
        rgb(ctx, MUTED)
        pw = text(ctx, vm.preset_line, 50, 37, spacing=0.4)
        if vm.dirty:
            ctx.arc(50 + pw + 6, 33.5, 2.6, 0, 2 * math.pi)
            rgb(ctx, AMBER)
            ctx.fill()

        # Middle badges.
        bx = max(50 + name_w + 10, 172)
        if vm.simulated:
            self._pill(ctx, bx, 14, 40, 16, "SIM", border=AMBER, color=AMBER, size=8.5)
            bx += 46
        if vm.pages > 1:
            # Page indicator: two segments, the current page lit.
            self._pill(ctx, bx, 14, 44, 16, f"PAG {vm.page + 1}", fill=AMBER if vm.page else None,
                       border=AMBER, color=BG if vm.page else AMBER, size=8.5)
            bx += 50
        if vm.in_clip:
            self._pill(ctx, bx, 14, 54, 16, "IN CLIP", fill=RED_BG, border=RED, color=RED, size=8.5)

        # Tempo block.
        font(ctx, MONO, 17, bold=True)
        rgb(ctx, AMBER if vm.tap_flash else TEXT)
        bw = text(ctx, vm.bpm_text, 330, 23, "right")
        font(ctx, SANS, 8.5, bold=True)
        rgb(ctx, MUTED)
        text(ctx, "BPM", 334, 23, spacing=0.6)
        font(ctx, SANS, 8.5, bold=True)
        rgb(ctx, TEXT if vm.clock_rx else MUTED)
        sw = text(ctx, vm.tempo_source, 352, 37, "right", spacing=0.6)
        ctx.arc(352 - sw - 7, 34, 3, 0, 2 * math.pi)
        rgb(ctx, TEXT if vm.clock_rx else DIM)
        ctx.fill()

        # Sync and on/bypass.
        if vm.sync:
            self._pill(ctx, 362, 10, 50, 22, "SYNC", fill=AMBER, color=BG)
        else:
            self._pill(ctx, 362, 10, 50, 22, "SYNC", border=DIM, color=MUTED)
        if vm.bypass:
            self._pill(ctx, 418, 10, 54, 22, "BYPASS", fill=RED_BG, border=RED, color=RED, size=9)
        else:
            self._pill(ctx, 418, 10, 54, 22, "AAN", border=DIM, color=TEXT)

    # ------------------------------------------------------------ chain

    def _chain(self, ctx, vm: ViewModel):
        n = len(vm.chain)
        gap = 4
        w = (W - 2 * MARGIN - (n - 1) * gap) / n
        for i, (short, on, shown) in enumerate(vm.chain):
            x = MARGIN + i * (w + gap)
            rrect(ctx, x + 0.5, CHAIN_Y + 0.5, w - 1, CHAIN_H - 1, 5)
            if on and not vm.bypass:
                rgb(ctx, (0x2E, 0x24, 0x10))
                ctx.fill_preserve()
            elif on:
                rgb(ctx, (0x1C, 0x1E, 0x22))
                ctx.fill_preserve()
            rgb(ctx, TEXT if shown else (AMBER if on and not vm.bypass else BORDER))
            ctx.set_line_width(1.5 if shown else 1)
            ctx.stroke()
            label = short
            if short == "COMP" and on and not vm.bypass and vm.gr_db >= 0.5:
                # Compressor gain reduction: value and a bar growing from the right.
                # Shown as a value plus a darker fill growing from the right (0..20 dB).
                label = f"−{vm.gr_db:.0f} dB"
                bw = (w - 2) * min(1.0, vm.gr_db / 20.0)
                ctx.save()
                rrect(ctx, x + 1, CHAIN_Y + 1, w - 2, CHAIN_H - 2, 4.5)
                ctx.clip()
                ctx.rectangle(x + w - 1 - bw, CHAIN_Y, bw, CHAIN_H)
                rgb(ctx, (0x5C, 0x40, 0x0E))
                ctx.fill()
                ctx.restore()
            font(ctx, SANS, 8.5, bold=True)
            if on:
                rgb(ctx, AMBER if not vm.bypass else MUTED)
            else:
                rgb(ctx, (0x5A, 0x60, 0x6A) if not shown else LABEL)
            text(ctx, label, x + w / 2, CHAIN_Y + 12.5, "center", spacing=0.4)

    # ------------------------------------------------------------ tiles

    def _tile(self, ctx, t: TileVM, x):
        y = TILE_Y
        if not t.label:   # unused position on page 2
            rrect(ctx, x + 0.5, y + 0.5, TILE_W - 1, TILE_H - 1, 10)
            rgb(ctx, BG)
            ctx.fill_preserve()
            rgb(ctx, BORDER)
            ctx.set_line_width(1)
            ctx.set_dash([3, 3])
            ctx.stroke()
            ctx.set_dash([])
            font(ctx, MONO, 18)
            rgb(ctx, DIM)
            text(ctx, "—", x + TILE_W / 2, y + 58, "center")
            return
        rrect(ctx, x + 0.5, y + 0.5, TILE_W - 1, TILE_H - 1, 10)
        rgb(ctx, PANEL_HI if t.highlight else PANEL)
        ctx.fill_preserve()
        rgb(ctx, AMBER if t.highlight else BORDER)
        ctx.set_line_width(1.5 if t.highlight else 1)
        ctx.stroke()

        font(ctx, SANS, 9.5, bold=True)
        rgb(ctx, AMBER if t.highlight else MUTED)
        text(ctx, t.label, x + 10, y + 18, spacing=1.0)

        # Knob position glyph (K1 top-left, K2 bottom-left, K3 top-right, K4 bottom-right).
        gx, gy = x + TILE_W - 22, y + 9
        pos = {0: (0, 0), 1: (0, 1), 2: (1, 0), 3: (1, 1)}
        for k, (cx, cy) in pos.items():
            rrect(ctx, gx + cx * 7, gy + cy * 7, 5, 5, 1.2)
            if k == t.knob:
                rgb(ctx, AMBER if t.highlight else TEXT)
            else:
                rgb(ctx, DIM)
            ctx.fill()

        font(ctx, MONO, 23 if len(t.value) <= 5 else 19 if len(t.value) <= 7 else 14.5, bold=False)
        rgb(ctx, TEXT)
        vw = text(ctx, t.value, x + 10, y + 56)
        if t.unit:
            font(ctx, MONO, 11)
            rgb(ctx, MUTED)
            text(ctx, t.unit, x + 10 + vw + 3, y + 56)

        if t.sub or t.synced:
            font(ctx, MONO, 9.5)
            rgb(ctx, MUTED)
            text(ctx, t.sub, x + 10, y + 73)
            if t.synced:
                font(ctx, SANS, 8, bold=True)
                rgb(ctx, AMBER)
                text(ctx, "SYNC", x + TILE_W - 10, y + 73, "right", spacing=0.8)

        bx, bw = x + 10, TILE_W - 20
        rrect(ctx, bx, y + 84, bw, 4, 2)
        rgb(ctx, TRACK)
        ctx.fill()
        fw = max(4.0, bw * max(0.0, min(1.0, t.norm)))
        rrect(ctx, bx, y + 84, fw, 4, 2)
        rgb(ctx, AMBER if t.highlight else SCALE)
        ctx.fill()

    # ------------------------------------------------------------ overlays

    def _overlay(self, ctx, ov):
        x, y, w, h = MARGIN, PANEL_Y, W - 2 * MARGIN, PANEL_H
        rrect(ctx, x + 0.5, y + 0.5, w - 1, h - 1, 12)
        rgb(ctx, (0x0F, 0x11, 0x14))
        ctx.fill_preserve()
        rgb(ctx, DIM)
        ctx.set_line_width(1)
        ctx.stroke()
        font(ctx, SANS, 10, bold=True)
        rgb(ctx, AMBER)
        text(ctx, ov.title, x + 14, y + 20, spacing=1.2)
        n = len(ov.items)
        rows = 4
        first = max(0, min(ov.cursor - 1, n - rows)) if n > rows else 0
        if n > rows:
            font(ctx, MONO, 9)
            rgb(ctx, MUTED)
            text(ctx, f"{ov.cursor + 1}/{n}", x + w - 14, y + 20, "right")
        for r in range(min(rows, n)):
            i = first + r
            label, value = ov.items[i]
            ry = y + 30 + r * 26
            sel = i == ov.cursor
            if sel:
                rrect(ctx, x + 8, ry, w - 16, 23, 7)
                rgb(ctx, (0x22, 0x1C, 0x10))
                ctx.fill_preserve()
                rgb(ctx, AMBER)
                ctx.set_line_width(1)
                ctx.stroke()
            font(ctx, MONO if ov.kind != "settings" else SANS, 11, bold=ov.kind == "settings")
            rgb(ctx, AMBER if sel else MUTED)
            text(ctx, label, x + 18, ry + 16)
            font(ctx, SANS, 12, bold=True)
            rgb(ctx, TEXT if sel else LABEL)
            text(ctx, value, x + w - 18, ry + 16, "right")
        font(ctx, SANS, 9)
        rgb(ctx, MUTED)
        text(ctx, ov.hint, x + w / 2, y + h - 10, "center")

    def _fault(self, ctx, title, body):
        x, y, w, h = MARGIN, PANEL_Y, W - 2 * MARGIN, PANEL_H
        rrect(ctx, x + 0.5, y + 0.5, w - 1, h - 1, 12)
        rgb(ctx, (0x16, 0x0F, 0x0E))
        ctx.fill_preserve()
        rgb(ctx, RED)
        ctx.set_line_width(1.5)
        ctx.stroke()
        # Warning triangle.
        cx, cy = x + 34, y + 44
        ctx.move_to(cx, cy - 14)
        ctx.line_to(cx + 15, cy + 12)
        ctx.line_to(cx - 15, cy + 12)
        ctx.close_path()
        rgb(ctx, RED)
        ctx.set_line_width(2)
        ctx.set_line_join(cairo.LINE_JOIN_ROUND)
        ctx.stroke()
        ctx.move_to(cx, cy - 5)
        ctx.line_to(cx, cy + 3)
        ctx.stroke()
        ctx.arc(cx, cy + 7.5, 1.4, 0, 2 * math.pi)
        ctx.fill()
        font(ctx, SANS, 15, bold=True)
        rgb(ctx, TEXT)
        text(ctx, title, x + 62, y + 48)
        font(ctx, SANS, 11)
        rgb(ctx, LABEL)
        self._wrap(ctx, body, x + 62, y + 72, w - 80, 16)

    def _wrap(self, ctx, s, x, y, maxw, lh):
        line = ""
        for word in s.split():
            trial = (line + " " + word).strip()
            if ctx.text_extents(trial).x_advance > maxw and line:
                text(ctx, line, x, y)
                y += lh
                line = word
            else:
                line = trial
        if line:
            text(ctx, line, x, y)

    def _toast(self, ctx, s):
        font(ctx, SANS, 12, bold=True)
        w = ctx.text_extents(s).x_advance + 28
        x, y = (W - w) / 2, METER_Y + FACE_H - 50    # above the PEAK strip
        rrect(ctx, x, y, w, 26, 13)
        rgb(ctx, (0x1B, 0x1E, 0x23))
        ctx.fill_preserve()
        rgb(ctx, DIM)
        ctx.set_line_width(1)
        ctx.stroke()
        rgb(ctx, TEXT)
        text(ctx, s, W / 2, y + 17.5, "center")

    def _warning(self, ctx, s):
        font(ctx, SANS, 9.5, bold=True)
        w = ctx.text_extents(s).x_advance + 22
        x, y = (W - w) / 2, METER_Y + 4
        rrect(ctx, x, y, w, 18, 9)
        rgb(ctx, RED_BG)
        ctx.fill_preserve()
        rgb(ctx, RED)
        ctx.set_line_width(1)
        ctx.stroke()
        rgb(ctx, RED)
        text(ctx, s, W / 2, y + 12.5, "center")

    # ------------------------------------------------------------ frame

    def render(self, surface: cairo.ImageSurface, vm: ViewModel) -> None:
        ctx = cairo.Context(surface)
        fo = cairo.FontOptions()
        fo.set_antialias(cairo.ANTIALIAS_GRAY)
        fo.set_hint_style(cairo.HINT_STYLE_SLIGHT)
        ctx.set_font_options(fo)
        rgb(ctx, BG)
        ctx.paint()
        self._header(ctx, vm)

        if vm.fault:
            self._fault(ctx, *vm.fault)
        elif vm.overlay:
            self._overlay(ctx, vm.overlay)
        else:
            self._chain(ctx, vm)
            self._meter(ctx, MARGIN, "L", vm.vu_db[0], vm.peak_text[0], vm.clip[0], vm.vu_ref_text)
            self._meter(ctx, MARGIN + METER_W + 8, "R=L" if vm.mono_in else "R", vm.vu_db[1], vm.peak_text[1],
                        vm.clip[1], vm.vu_ref_text)
            if vm.warning:
                self._warning(ctx, vm.warning)
            if vm.toast:
                self._toast(ctx, vm.toast)

        for i, t in enumerate(vm.tiles):
            self._tile(ctx, t, MARGIN + i * (TILE_W + TILE_GAP))
        surface.flush()
