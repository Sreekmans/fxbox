"""fxbox-hwtest: checks the hardware and confirms the control mapping on the device.

    sudo fxbox-hwtest check        # what is present (I2C chips, screen, sound card, MIDI UART)
    sudo fxbox-hwtest screen       # test image on the ZynScreen
    sudo fxbox-hwtest controls     # guided: press/turn each control, compare with the config
    sudo fxbox-hwtest controls --write   # ... and store the measured mapping in /etc/fxbox/hardware.toml

Stop the UI first (sudo systemctl stop fxbox-ui) so the controls are free.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time

from . import config

PIN_NAMES = [f"GPA{i}" for i in range(8)] + [f"GPB{i}" for i in range(8)]


def sh(cmd: str) -> str:
    try:
        return subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=10).stdout
    except Exception as e:  # pragma: no cover
        return str(e)


def check(cfg) -> int:
    ok = True

    def line(good, what, detail=""):
        nonlocal ok
        ok &= bool(good)
        print(f"  [{'OK ' if good else 'MIS'}] {what}{(' — ' + detail) if detail else ''}")

    print("FXBox hardwarecontrole")
    if not os.path.exists("/dev/i2c-1"):
        print("  [MIS] /dev/i2c-1 ontbreekt — los op met: sudo modprobe i2c-dev "
              "&& echo i2c-dev | sudo tee /etc/modules-load.d/fxbox-i2c.conf")
    i2c = sh("i2cdetect -y 1")
    found = set()
    for row in i2c.splitlines()[1:]:
        parts = row.split()
        if not parts:
            continue
        base = int(parts[0].rstrip(":"), 16)
        for i, p in enumerate(parts[1:]):
            if p not in ("--", "UU"):
                found.add(base + i)
            elif p == "UU":
                found.add(base + i)
    line(cfg.inputs.address in found, f"MCP23017 (bediening) op I²C 0x{cfg.inputs.address:02X}")
    line(0x4A in found, "PCM1863 ADC op 0x4A (ZynADAC / HiFiBerry DAC+ ADC Pro)",
         "UU = in gebruik door de driver (goed)" if 0x4A in found else "")
    line(0x4D in found, "PCM5242 DAC op 0x4D")
    cards = sh("cat /proc/asound/cards")
    line("sndrpihifiberry" in cards, "ALSA-kaart sndrpihifiberry", cards.strip().splitlines()[0] if cards.strip() else "")
    fb = cfg.display.framebuffer
    if fb == "auto":
        from .display import find_framebuffer
        try:
            fb = find_framebuffer()
        except RuntimeError:
            fb = "(ili9486 niet gevonden)"
    if os.path.exists(fb):
        size = sh(f"cat /sys/class/graphics/{os.path.basename(fb)}/virtual_size").strip()
        bpp = sh(f"cat /sys/class/graphics/{os.path.basename(fb)}/bits_per_pixel").strip()
        line(size == f"{cfg.display.width},{cfg.display.height}" and bpp == "16", f"scherm {fb}", f"{size} @ {bpp} bpp")
    else:
        line(False, f"scherm {fb}", "framebuffer ontbreekt (overlay piscreen2r-notouch geladen?)")
    line(os.path.exists("/dev/ttyAMA0"), "DIN-MIDI UART /dev/ttyAMA0")
    line(os.path.exists("/dev/gpiochip0"), "GPIO-chip voor MCP-interrupts")
    print("Alles aanwezig." if ok else "Niet alles gevonden — zie docs/HANDLEIDING.md, Herstel.")
    return 0 if ok else 1


def screen(cfg) -> int:
    import cairo

    from .display import FramebufferDisplay
    from .render import AMBER, BG, MUTED, RED, SANS, TEXT, font, rgb, text
    d = FramebufferDisplay(cfg.display.framebuffer, cfg.display.width, cfg.display.height)
    d.open()
    s = d.make_surface()
    c = cairo.Context(s)
    rgb(c, BG)
    c.paint()
    cols = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 255), AMBER, RED, MUTED, TEXT]
    w = cfg.display.width / len(cols)
    for i, col in enumerate(cols):
        rgb(c, col)
        c.rectangle(i * w, 0, w, 60)
        c.fill()
    rgb(c, TEXT)
    c.set_line_width(1)
    c.rectangle(0.5, 0.5, cfg.display.width - 1, cfg.display.height - 1)
    c.stroke()
    font(c, SANS, 22, bold=True)
    text(c, "FXBox schermtest", cfg.display.width / 2, 140, "center")
    font(c, SANS, 13)
    text(c, "Rand moet rondom zichtbaar zijn; tekst rechtop en leesbaar.", cfg.display.width / 2, 170, "center")
    text(c, f"{cfg.display.width} x {cfg.display.height}", cfg.display.width / 2, 195, "center")
    d.show(s)
    print("Testbeeld staat op het scherm. Ctrl+C om te stoppen.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    return 0


def _read_word(m) -> int:
    # The I2C bus is shared with the ADC/DAC; retry transient NACKs (Errno 121).
    for attempt in range(20):
        try:
            return m._read()
        except OSError:
            time.sleep(0.005)
    return m._read()


def controls(cfg, write: bool, path: str) -> int:
    import queue

    from .inputs import McpInputs
    m = McpInputs(cfg.inputs, queue.Queue())
    m._make_irq_waiter = lambda: None      # polling is enough here; leave the IRQ lines free
    if not m.open():
        print(m.error)
        return 1
    idle = _read_word(m)
    print(f"Ruststand: {idle:016b} (alle ingangen hoog = niets ingedrukt)")

    def wait_change(prompt: str, timeout=20.0):
        print(prompt)
        base = _read_word(m)
        end = time.time() + timeout
        changed = []
        while time.time() < end:
            w = _read_word(m)
            diff = (w ^ base) & 0xFFFF
            for p in range(16):
                if diff >> p & 1 and p not in changed:
                    changed.append(p)
            if changed and w == base:
                return changed
            time.sleep(0.002)
        return changed

    sw_pins = []
    for i in range(4):
        ch = wait_change(f"  Druk kort op S{i + 1} en laat los...")
        print(f"    -> {', '.join(PIN_NAMES[p] for p in ch) or 'niets gemeten'}")
        sw_pins.append(ch[0] if len(ch) == 1 else None)
    enc_sw, enc_a, enc_b, rev = [], [], [], []
    names = ["K1 (linksboven)", "K2 (linksonder)", "K3 (rechtsboven)", "K4 (rechtsonder)"]
    for k in range(4):
        ch = wait_change(f"  Druk {names[k]} in en laat los...")
        print(f"    -> {', '.join(PIN_NAMES[p] for p in ch) or 'niets gemeten'}")
        enc_sw.append(ch[0] if len(ch) == 1 else None)
        print(f"  Draai {names[k]} langzaam één klik rechtsom...")
        base = _read_word(m)
        order = []
        end = time.time() + 20
        while time.time() < end:
            w = _read_word(m)
            for p in range(16):
                if (w ^ base) >> p & 1 and p not in order:
                    order.append(p)
            if len(order) >= 2 and w == base:
                break
            time.sleep(0.0005)
        if len(order) >= 2:
            a, b = order[0], order[1]
            print(f"    -> eerst {PIN_NAMES[a]}, dan {PIN_NAMES[b]}")
            enc_a.append(a)
            enc_b.append(b)
            rev.append(False)
        else:
            print("    -> geen draaiing gemeten")
            enc_a.append(None)
            enc_b.append(None)
            rev.append(False)

    measured = {"switches": sw_pins, "pin_sw": enc_sw, "pin_a": enc_a, "pin_b": enc_b}
    print("\nGemeten koppeling vs. configuratie:")
    ic = cfg.inputs
    for key, cur in (("switches", ic.switches), ("pin_sw", ic.enc_sw), ("pin_a", ic.enc_a), ("pin_b", ic.enc_b)):
        print(f"  {key:9s} gemeten {measured[key]}  config {cur}  {'(gelijk)' if measured[key] == cur else '(VERSCHILT)'}")
    if None in measured["switches"] or None in measured["pin_sw"]:
        print("Niet alles gemeten; configuratie niet aangepast.")
        return 1
    if write:
        body = open(path).read() if os.path.exists(path) else ""
        import re
        # A/B order is only informative (depends on where the detent rests); direction
        # is set with "reverse" in hardware.toml after a test turn in the interface.
        for key, sect in (("pin_sw", "encoders"), ("pins", "switches")):
            val = measured["switches" if key == "pins" else key]
            new = f"{key} = {val}"
            if re.search(rf"^{key}\s*=.*$", body, re.M):
                body = re.sub(rf"^{key}\s*=.*$", new, body, count=1, flags=re.M)
            else:
                body += f"\n[{sect}]\n{new}\n"
        with open(path + ".tmp", "w") as f:
            f.write(body)
        os.replace(path + ".tmp", path)
        print(f"Opgeslagen in {path}. Herstart de interface: sudo systemctl restart fxbox-ui")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("what", choices=["check", "screen", "controls"])
    ap.add_argument("--hw-config", default=config.DEFAULT_PATH)
    ap.add_argument("--write", action="store_true")
    a = ap.parse_args()
    cfg = config.load(a.hw_config)
    if a.what == "check":
        return check(cfg)
    if a.what == "screen":
        return screen(cfg)
    return controls(cfg, a.write, a.hw_config)


if __name__ == "__main__":
    sys.exit(main())
