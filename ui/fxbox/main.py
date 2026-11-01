"""Device entry point: framebuffer UI + physical controls + engine link.

Started by systemd (fxbox-ui.service). Audio keeps running in fxbox-engine
when this process restarts.
"""
from __future__ import annotations

import argparse
import logging
import os
import queue
import signal
import socket
import time

from . import __version__, config
from .controller import Controller
from .display import FramebufferDisplay
from .engine_link import EngineLink
from .inputs import McpInputs
from .render import Renderer
from .storage import DebouncedState, PresetLibrary

log = logging.getLogger("fxbox")


def sd_notify(msg: str) -> None:
    """Minimal systemd notify (READY / WATCHDOG) without extra dependencies."""
    path = os.environ.get("NOTIFY_SOCKET")
    if not path:
        return
    if path.startswith("@"):
        path = "\0" + path[1:]
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
            s.connect(path)
            s.sendall(msg.encode())
    except OSError:
        pass


def main() -> int:
    ap = argparse.ArgumentParser(description="FXBox user interface")
    ap.add_argument("--hw-config", default=config.DEFAULT_PATH)
    ap.add_argument("--data-dir", default="/var/lib/fxbox")
    ap.add_argument("--factory", default="/opt/fxbox/presets/factory.json")
    ap.add_argument("--socket", default="/run/fxbox/engine.sock")
    ap.add_argument("--no-inputs", action="store_true", help="run without the MCP23017 (diagnostics)")
    args = ap.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(name)s: %(message)s")
    log.info("fxbox-ui %s", __version__)

    hw = config.load(args.hw_config)
    os.makedirs(args.data_dir, exist_ok=True)
    lib = PresetLibrary(args.factory, os.path.join(args.data_dir, "user_presets.json"))
    store = DebouncedState(os.path.join(args.data_dir, "state.json"))
    link = EngineLink(args.socket)
    ctl = Controller(link, lib, store)
    link.start()

    events: "queue.Queue[tuple]" = queue.Queue()
    inputs = None if args.no_inputs else McpInputs(hw.inputs, events)
    inputs_ok = False
    next_input_try = 0.0

    display = FramebufferDisplay(hw.display.framebuffer, hw.display.width, hw.display.height)
    display_ok = False
    next_display_try = 0.0
    renderer = Renderer()
    surface = display.make_surface()

    stop = {"flag": False}

    def on_term(*_):
        stop["flag"] = True
    signal.signal(signal.SIGTERM, on_term)
    signal.signal(signal.SIGINT, on_term)

    sd_notify("READY=1")
    frame = 1.0 / hw.display.fps
    next_frame = time.monotonic()
    next_watchdog = 0.0
    next_full_refresh = 0.0
    while not stop["flag"]:
        now = time.monotonic()
        if inputs and not inputs_ok and now >= next_input_try:
            inputs_ok = inputs.open()
            if inputs_ok:
                inputs.start()
                ctl.hw_warning = ""
            else:
                ctl.hw_warning = inputs.error
                next_input_try = now + 5.0
        if inputs_ok and inputs.error:
            ctl.hw_warning = inputs.error
        if not display_ok and now >= next_display_try:
            try:
                display.open()
                display_ok = True
            except (OSError, RuntimeError) as e:
                log.error("display unavailable: %s", e)
                next_display_try = now + 5.0

        # Wait for input until the next frame is due.
        timeout = max(0.0, next_frame - time.monotonic())
        try:
            ev = events.get(timeout=timeout)
            ctl.handle(ev)
            while True:
                ctl.handle(events.get_nowait())
        except queue.Empty:
            pass

        for ev in link.poll():
            ctl.engine_event(ev)
        now = time.monotonic()
        ctl.tick(now)
        if now >= next_frame:
            next_frame += frame
            if next_frame < now:          # we fell behind: do not try to catch up
                next_frame = now + frame
            if display_ok:
                if now >= next_full_refresh:     # repaint everything now and then
                    display.invalidate()
                    next_full_refresh = now + 10.0
                renderer.render(surface, ctl.view(now))
                try:
                    display.show(surface)
                except OSError as e:
                    log.error("display write failed: %s", e)
                    display_ok = False
            else:
                ctl.view(now)
        if now >= next_watchdog:
            sd_notify("WATCHDOG=1")
            next_watchdog = now + 2.0

    log.info("stopping: saving state")
    ctl.shutdown()
    if inputs:
        inputs.stop()
    link.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
