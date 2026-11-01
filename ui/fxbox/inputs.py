"""Physical controls: MCP23017 port expander with 4 push encoders and S1-S4.

Pin assignment and interrupt lines come from the hardware config
(defaults = Zynthian V4 "MCP23017_ZynScreen" wiring, see docs/HARDWARE.md).

Events put on the queue (timestamps from time.monotonic()):
    ("turn",   knob 0-3, detents (+/-), t)
    ("knob",   knob 0-3, pressed bool, t)
    ("switch", s 0-3,    pressed bool, t)
"""
from __future__ import annotations

import logging
import queue
import threading
import time
from dataclasses import dataclass, field

log = logging.getLogger("fxbox.inputs")

# MCP23017 registers (IOCON.BANK = 0)
IODIRA, IPOLA, GPINTENA, DEFVALA, INTCONA, IOCON, GPPUA, INTFA, INTCAPA, GPIOA = (
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12)


# ----------------------------------------------------------- quadrature decoder

# Full-step state machine (after Ben Buxton's rotary encoder table): emits one
# step per detent and ignores contact bounce and invalid transitions.
_R_START, _R_CW_FINAL, _R_CW_BEGIN, _R_CW_NEXT, _R_CCW_BEGIN, _R_CCW_FINAL, _R_CCW_NEXT = range(7)
_DIR_CW, _DIR_CCW = 0x10, 0x20
_FULL_STEP = [
    [_R_START, _R_CW_BEGIN, _R_CCW_BEGIN, _R_START],
    [_R_CW_NEXT, _R_START, _R_CW_FINAL, _R_START | _DIR_CW],
    [_R_CW_NEXT, _R_CW_BEGIN, _R_START, _R_START],
    [_R_CW_NEXT, _R_CW_BEGIN, _R_CW_FINAL, _R_START],
    [_R_CCW_NEXT, _R_START, _R_CCW_BEGIN, _R_START],
    [_R_CCW_NEXT, _R_CCW_FINAL, _R_START, _R_START | _DIR_CCW],
    [_R_CCW_NEXT, _R_CCW_FINAL, _R_CCW_BEGIN, _R_START],
]
# Half-step variant for encoders with two detents per quadrature cycle.
_H_START, _H_CCW_BEGIN, _H_CW_BEGIN, _H_START_M, _H_CW_BEGIN_M, _H_CCW_BEGIN_M = range(6)
_HALF_STEP = [
    [_H_START_M, _H_CW_BEGIN, _H_CCW_BEGIN, _H_START],
    [_H_START_M | _DIR_CCW, _H_START, _H_CCW_BEGIN, _H_START],
    [_H_START_M | _DIR_CW, _H_CW_BEGIN, _H_START, _H_START],
    [_H_START_M, _H_CCW_BEGIN_M, _H_CW_BEGIN_M, _H_START],
    [_H_START_M, _H_START_M, _H_CW_BEGIN_M, _H_START | _DIR_CW],
    [_H_START_M, _H_CCW_BEGIN_M, _H_START_M, _H_START | _DIR_CCW],
]


class QuadratureDecoder:
    def __init__(self, half_step: bool = False, reverse: bool = False):
        self.table = _HALF_STEP if half_step else _FULL_STEP
        self.state = 0
        self.reverse = reverse

    def feed(self, a: int, b: int) -> int:
        """Feed the current pin levels (1 = high); returns +1, -1 or 0.

        +1 when A leads B (the usual clockwise convention); real direction is
        verified on the device with fxbox-hwtest and can be flipped per knob."""
        pins = ((a & 1) << 1) | (b & 1)
        self.state = self.table[self.state & 0x0F][pins]
        d = self.state & 0x30
        step = 1 if d == _DIR_CW else (-1 if d == _DIR_CCW else 0)
        return -step if self.reverse else step


class Debouncer:
    """A level must be stable for `stable_s` before it is reported."""

    def __init__(self, stable_s: float = 0.012, initial: bool = False):
        self.stable_s = stable_s
        self.state = initial
        self._cand = initial
        self._since = 0.0

    def feed(self, level: bool, t: float):
        """Returns the new state when it changed (confirmed), else None."""
        if level != self._cand:
            self._cand = level
            self._since = t
        if self._cand != self.state and t - self._since >= self.stable_s:
            self.state = self._cand
            return self.state
        return None

    @property
    def pending(self) -> bool:
        return self._cand != self.state


# ----------------------------------------------------------------- MCP23017

@dataclass
class InputConfig:
    i2c_bus: int = 1
    address: int = 0x20
    gpiochip: str = "gpiochip0"
    int_a_gpio: int = 27        # BCM; Zynthian config value "2" is a wiringPi number
    int_b_gpio: int = 4         # BCM; Zynthian config value "7" is a wiringPi number
    enc_a: list[int] = field(default_factory=lambda: [2, 5, 10, 13])   # MCP pin 0-7 = GPA0-7, 8-15 = GPB0-7
    enc_b: list[int] = field(default_factory=lambda: [1, 4, 9, 12])
    enc_sw: list[int] = field(default_factory=lambda: [0, 3, 8, 11])
    enc_reverse: list[bool] = field(default_factory=lambda: [False] * 4)
    half_step: bool = False
    switches: list[int] = field(default_factory=lambda: [6, 7, 14, 15])  # S1..S4 (verify with fxbox-hwtest)
    poll_fallback_ms: float = 20.0


class McpInputs:
    """Reads the MCP23017 on interrupt (with a polling fallback)."""

    def __init__(self, cfg: InputConfig, out: "queue.Queue[tuple]"):
        self.cfg = cfg
        self.out = out
        self.bus = None
        self._stop = threading.Event()
        self._thread = None
        self._wait = None
        self.decoders = [QuadratureDecoder(cfg.half_step, cfg.enc_reverse[i]) for i in range(4)]
        self.knob_db = [Debouncer() for _ in range(4)]
        self.sw_db = [Debouncer() for _ in range(4)]
        self.error = ""

    # --- setup

    def open(self) -> bool:
        try:
            try:
                from smbus2 import SMBus  # type: ignore
            except ImportError:
                from smbus import SMBus  # type: ignore
            self.bus = SMBus(self.cfg.i2c_bus)
            a = self.cfg.address
            self.bus.write_byte_data(a, IOCON, 0x02)          # BANK=0, MIRROR=0, ODR=0, INTPOL=1 (active high)
            for reg in (IODIRA, IODIRA + 1, GPPUA, GPPUA + 1):
                self.bus.write_byte_data(a, reg, 0xFF)        # all inputs, pull-ups on
            for reg in (IPOLA, IPOLA + 1, INTCONA, INTCONA + 1):
                self.bus.write_byte_data(a, reg, 0x00)        # no inversion, interrupt on change
            for reg in (GPINTENA, GPINTENA + 1):
                self.bus.write_byte_data(a, reg, 0xFF)
            self._read()                                       # clears pending interrupts
        except Exception as e:
            self.error = f"Bediening niet gevonden (I²C 0x{self.cfg.address:02X})"
            log.error("MCP23017 init failed: %s", e)
            return False
        self._wait = self._make_irq_waiter()
        return True

    def _make_irq_waiter(self):
        """Returns wait(timeout_s) that blocks until an MCP interrupt or timeout."""
        try:
            import gpiod  # type: ignore
        except ImportError:
            log.warning("python gpiod not available: polling the MCP23017")
            return None
        pins = [self.cfg.int_a_gpio, self.cfg.int_b_gpio]
        try:
            if hasattr(gpiod, "request_lines"):   # libgpiod v2 API
                from gpiod.line import Edge  # type: ignore
                req = gpiod.request_lines(
                    "/dev/" + self.cfg.gpiochip, consumer="fxbox",
                    config={tuple(pins): gpiod.LineSettings(edge_detection=Edge.RISING)})

                def wait(timeout):
                    if req.wait_edge_events(timeout):
                        req.read_edge_events()
                        return True
                    return False
                return wait
            chip = gpiod.Chip(self.cfg.gpiochip)          # libgpiod v1 API (Bookworm)
            lines = chip.get_lines(pins)
            lines.request(consumer="fxbox", type=gpiod.LINE_REQ_EV_RISING_EDGE)

            def wait_v1(timeout):
                sec = int(timeout)
                ev = lines.event_wait(sec=sec, nsec=int((timeout - sec) * 1e9))
                if ev:
                    for line in ev:
                        line.event_read()
                    return True
                return False
            return wait_v1
        except Exception as e:
            log.warning("interrupt lines unavailable (%s): polling the MCP23017", e)
            return None

    # --- runtime

    def _read(self) -> int:
        a, b = self.bus.read_i2c_block_data(self.cfg.address, GPIOA, 2)
        return a | (b << 8)

    def start(self) -> None:
        self._thread = threading.Thread(target=self._run, name="mcp23017", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def _process(self, word: int, t: float) -> None:
        bit = lambda pin: (word >> pin) & 1  # noqa: E731
        c = self.cfg
        for k in range(4):
            step = self.decoders[k].feed(bit(c.enc_a[k]), bit(c.enc_b[k]))
            if step:
                self.out.put(("turn", k, step, t))
            pressed = bit(c.enc_sw[k]) == 0              # switches pull the pin low
            ch = self.knob_db[k].feed(pressed, t)
            if ch is not None:
                self.out.put(("knob", k, ch, t))
        for s in range(4):
            pressed = bit(c.switches[s]) == 0
            ch = self.sw_db[s].feed(pressed, t)
            if ch is not None:
                self.out.put(("switch", s, ch, t))

    def _run(self) -> None:
        errors = 0
        while not self._stop.is_set():
            pending = any(d.pending for d in self.knob_db + self.sw_db)
            timeout = 0.004 if pending else self.cfg.poll_fallback_ms / 1000.0
            if self._wait:
                try:
                    self._wait(timeout)
                except Exception:
                    time.sleep(timeout)
            else:
                time.sleep(0.001 if not pending else 0.002)
            try:
                word = self._read()
                errors = 0
            except OSError as e:
                errors += 1
                if errors == 50:
                    log.error("I2C read errors: %s", e)
                    self.error = "Bediening reageert niet (I²C-fout)"
                time.sleep(0.01)
                continue
            self._process(word, time.monotonic())
