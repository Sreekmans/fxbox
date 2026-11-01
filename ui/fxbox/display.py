"""Output to the ZynScreen through the Linux framebuffer (fbtft, /dev/fb1).

Only rows that changed since the previous frame are written. fbtft refreshes
the panel per touched memory page, so an unchanged screen costs no SPI time
and a moving needle only refreshes the meter rows.
"""
from __future__ import annotations

import logging
import mmap
import os

import cairo

log = logging.getLogger("fxbox.display")


def _sysfs(fb: str, name: str) -> str:
    with open(f"/sys/class/graphics/{os.path.basename(fb)}/{name}") as f:
        return f.read().strip()


def find_framebuffer(hint: str = "ili9486") -> str:
    """Finds the ZynScreen framebuffer by driver name; its number depends on
    whether an HDMI framebuffer was registered first."""
    base = "/sys/class/graphics"
    try:
        for name in sorted(os.listdir(base)):
            if not name.startswith("fb"):
                continue
            try:
                with open(os.path.join(base, name, "name")) as f:
                    if hint in f.read():
                        return "/dev/" + name
            except OSError:
                continue
    except OSError:
        pass
    raise RuntimeError(f"no framebuffer with driver '{hint}' found")


class FramebufferDisplay:
    def __init__(self, device: str = "/dev/fb1", width: int = 480, height: int = 320):
        self.device = device
        self.width = width
        self.height = height
        self.fd = None
        self.mm = None
        self.prev = None

    def open(self) -> None:
        if self.device == "auto":
            self.device = find_framebuffer()
        bpp = int(_sysfs(self.device, "bits_per_pixel"))
        vw, vh = (int(x) for x in _sysfs(self.device, "virtual_size").split(","))
        stride = int(_sysfs(self.device, "stride"))
        if bpp != 16:
            raise RuntimeError(f"{self.device}: {bpp} bpp, expected 16 (RGB565)")
        if (vw, vh) != (self.width, self.height):
            raise RuntimeError(f"{self.device}: {vw}x{vh}, expected {self.width}x{self.height} "
                               "(check rotate= in the display overlay)")
        self.stride = stride
        self.fd = os.open(self.device, os.O_RDWR)
        self.mm = mmap.mmap(self.fd, stride * vh, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)
        log.info("framebuffer %s %dx%d stride %d", self.device, vw, vh, stride)

    def make_surface(self) -> cairo.ImageSurface:
        return cairo.ImageSurface(cairo.FORMAT_RGB16_565, self.width, self.height)

    def invalidate(self) -> None:
        """Forces the next show() to rewrite every row."""
        self.prev = None

    def show(self, surface: cairo.ImageSurface) -> int:
        """Copies changed rows to the framebuffer; returns the number of rows written."""
        surface.flush()
        data = surface.get_data()
        src_stride = surface.get_stride()
        row_bytes = self.width * 2
        cur = bytes(data)
        written = 0
        y = 0
        while y < self.height:
            off = y * src_stride
            if self.prev is not None and cur[off:off + row_bytes] == self.prev[off:off + row_bytes]:
                y += 1
                continue
            # Extend over the run of changed rows and write it in one go.
            y2 = y + 1
            while y2 < self.height:
                o2 = y2 * src_stride
                if self.prev is not None and cur[o2:o2 + row_bytes] == self.prev[o2:o2 + row_bytes]:
                    break
                y2 += 1
            for r in range(y, y2):
                so = r * src_stride
                do = r * self.stride
                self.mm[do:do + row_bytes] = cur[so:so + row_bytes]
            written += y2 - y
            y = y2
        self.prev = cur
        return written

    def blank(self) -> None:
        if self.mm:
            self.mm[:] = b"\x00" * len(self.mm)

    def close(self) -> None:
        if self.mm:
            self.mm.close()
        if self.fd is not None:
            os.close(self.fd)
