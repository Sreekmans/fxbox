"""Hardware configuration (/etc/fxbox/hardware.toml), kept separate from the code."""
from __future__ import annotations

import logging
import os
from dataclasses import dataclass, field

try:
    import tomllib  # Python 3.11+
except ImportError:  # pragma: no cover
    tomllib = None

from .inputs import InputConfig

log = logging.getLogger("fxbox.config")

DEFAULT_PATH = "/etc/fxbox/hardware.toml"


@dataclass
class DisplayConfig:
    framebuffer: str = "auto"   # "auto" = find the ili9486 (fbtft) framebuffer
    width: int = 480
    height: int = 320
    fps: int = 30


@dataclass
class HardwareConfig:
    display: DisplayConfig = field(default_factory=DisplayConfig)
    inputs: InputConfig = field(default_factory=InputConfig)


def _int(v) -> int:
    return int(v, 0) if isinstance(v, str) else int(v)


def load(path: str = DEFAULT_PATH) -> HardwareConfig:
    cfg = HardwareConfig()
    if not path or not os.path.exists(path) or tomllib is None:
        if path and not os.path.exists(path):
            log.warning("%s not found, using built-in V4 defaults", path)
        return cfg
    with open(path, "rb") as f:
        d = tomllib.load(f)
    disp = d.get("display", {})
    cfg.display.framebuffer = str(disp.get("framebuffer", cfg.display.framebuffer))
    cfg.display.width = int(disp.get("width", cfg.display.width))
    cfg.display.height = int(disp.get("height", cfg.display.height))
    cfg.display.fps = max(10, min(60, int(disp.get("fps", cfg.display.fps))))

    mcp = d.get("mcp23017", {})
    ic = cfg.inputs
    ic.i2c_bus = int(mcp.get("i2c_bus", ic.i2c_bus))
    ic.address = _int(mcp.get("address", ic.address))
    ic.gpiochip = str(mcp.get("gpiochip", ic.gpiochip))
    ic.int_a_gpio = int(mcp.get("int_a_gpio", ic.int_a_gpio))
    ic.int_b_gpio = int(mcp.get("int_b_gpio", ic.int_b_gpio))

    enc = d.get("encoders", {})
    for key, attr in (("pin_a", "enc_a"), ("pin_b", "enc_b"), ("pin_sw", "enc_sw")):
        if key in enc:
            vals = [int(x) for x in enc[key]]
            if len(vals) != 4 or not all(0 <= x <= 15 for x in vals):
                raise ValueError(f"encoders.{key} needs 4 MCP pin numbers 0..15")
            setattr(ic, attr, vals)
    if "reverse" in enc:
        ic.enc_reverse = [bool(x) for x in enc["reverse"]][:4]
    ic.half_step = int(enc.get("steps_per_detent", 4)) == 2

    sw = d.get("switches", {})
    if "pins" in sw:
        vals = [int(x) for x in sw["pins"]]
        if len(vals) != 4 or not all(0 <= x <= 15 for x in vals):
            raise ValueError("switches.pins needs 4 MCP pin numbers 0..15")
        ic.switches = vals
    return cfg
