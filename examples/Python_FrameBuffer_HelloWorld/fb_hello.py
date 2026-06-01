#!/usr/bin/env python3
"""Minimal RGB565 framebuffer 'Hello CardputerZero' demo for M5 CardputerZero."""

import fcntl
import mmap
import os
import select
import signal
import struct
import sys
import time
from datetime import datetime

from PIL import Image, ImageDraw, ImageFont

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

try:
    from evdev import InputDevice, categorize, ecodes, list_devices
    HAS_EVDEV = True
except ImportError:
    HAS_EVDEV = False

FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602

FB_DEVICE = "/dev/fb0"
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

VAR_FMT = "8I"
FIX_FMT = "16sL" + "I" * 6 + "HHHI"

_running = True


def _sigint(_signum, _frame):
    global _running
    _running = False


def read_var_screeninfo(fd):
    buf = bytearray(160)
    fcntl.ioctl(fd, FBIOGET_VSCREENINFO, buf)
    xres, yres, xres_v, yres_v, xoff, yoff, bpp, grayscale = struct.unpack_from(VAR_FMT, buf, 0)
    return xres, yres, bpp


def read_fix_screeninfo(fd):
    buf = bytearray(struct.calcsize(FIX_FMT))
    fcntl.ioctl(fd, FBIOGET_FSCREENINFO, buf)
    fields = struct.unpack(FIX_FMT, buf)
    smem_len = fields[2]
    line_length = fields[8]
    return smem_len, line_length


def load_font(size):
    try:
        return ImageFont.truetype(FONT_PATH, size)
    except (OSError, IOError):
        return ImageFont.load_default()


def draw_frame(w, h, font_big, font_small):
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rectangle([(0, 0), (w - 1, h - 1)], outline=(255, 255, 255), width=2)

    title = "Hello CardputerZero"
    tb = draw.textbbox((0, 0), title, font=font_big)
    tw, th = tb[2] - tb[0], tb[3] - tb[1]
    draw.text(((w - tw) // 2 - tb[0], (h // 2) - th - 4 - tb[1]),
              title, font=font_big, fill=(255, 255, 255))

    stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    sb = draw.textbbox((0, 0), stamp, font=font_small)
    sw, sh = sb[2] - sb[0], sb[3] - sb[1]
    draw.text(((w - sw) // 2 - sb[0], (h // 2) + 6 - sb[1]),
              stamp, font=font_small, fill=(180, 220, 255))
    return img


def rgb_to_rgb565_bytes(img):
    if HAS_NUMPY:
        arr = np.asarray(img, dtype=np.uint16)
        r = (arr[..., 0] >> 3) & 0x1F
        g = (arr[..., 1] >> 2) & 0x3F
        b = (arr[..., 2] >> 3) & 0x1F
        rgb565 = (r << 11) | (g << 5) | b
        return rgb565.astype("<u2").tobytes()
    out = bytearray()
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            out.append(v & 0xFF)
            out.append((v >> 8) & 0xFF)
    return bytes(out)


def blit(mm, frame_bytes, w, h, line_length):
    row_bytes = w * 2
    if line_length == row_bytes:
        mm.seek(0)
        mm.write(frame_bytes)
        return
    for y in range(h):
        mm.seek(y * line_length)
        mm.write(frame_bytes[y * row_bytes:(y + 1) * row_bytes])


def find_keyboards():
    if not HAS_EVDEV:
        return []
    devs = []
    for path in list_devices():
        try:
            d = InputDevice(path)
            caps = d.capabilities().get(ecodes.EV_KEY, [])
            if ecodes.KEY_ESC in caps or ecodes.KEY_Q in caps:
                devs.append(d)
            else:
                d.close()
        except (OSError, PermissionError):
            pass
    return devs


def check_quit(devs):
    if not devs:
        return False
    fds = [d.fd for d in devs]
    r, _, _ = select.select(fds, [], [], 0)
    for d in devs:
        if d.fd in r:
            try:
                for ev in d.read():
                    if ev.type == ecodes.EV_KEY and ev.value == 1:
                        if ev.code in (ecodes.KEY_ESC, ecodes.KEY_Q):
                            return True
            except BlockingIOError:
                pass
    return False


def main():
    if not sys.platform.startswith("linux"):
        print("This example only runs on Linux with /dev/fb0.", file=sys.stderr)
        return 1

    signal.signal(signal.SIGINT, _sigint)
    signal.signal(signal.SIGTERM, _sigint)

    fd = os.open(FB_DEVICE, os.O_RDWR)
    try:
        xres, yres, bpp = read_var_screeninfo(fd)
        smem_len, line_length = read_fix_screeninfo(fd)
        if bpp != 16:
            print("Expected 16 bpp (RGB565), got {}".format(bpp), file=sys.stderr)
            return 2
        mm = mmap.mmap(fd, smem_len, mmap.MAP_SHARED,
                       mmap.PROT_READ | mmap.PROT_WRITE)
    except Exception as e:
        os.close(fd)
        print("Framebuffer init failed: {}".format(e), file=sys.stderr)
        return 3

    font_big = load_font(18)
    font_small = load_font(14)
    devs = find_keyboards()

    try:
        while _running:
            img = draw_frame(xres, yres, font_big, font_small)
            blit(mm, rgb_to_rgb565_bytes(img), xres, yres, line_length)
            deadline = time.monotonic() + 1.0
            while _running and time.monotonic() < deadline:
                if check_quit(devs):
                    return 0
                time.sleep(0.05)
    finally:
        for d in devs:
            try:
                d.close()
            except OSError:
                pass
        try:
            mm.close()
        finally:
            os.close(fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
