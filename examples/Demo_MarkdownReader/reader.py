#!/usr/bin/env python3
"""Demo_MarkdownReader: paged text reader for .md files on /dev/fb0.

Usage: reader.py <file.md>

Keys (evdev):
  Space / Down / PageDown / J / N - next page
  B     / Up   / PageUp   / K / P - previous page
  Esc   / Q                       - quit
"""

import fcntl
import glob
import mmap
import os
import select
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

FB_DEV = "/dev/fb0"
W, H = 320, 170
BPP = 2
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
FONT_SIZE = 11
LINE_H = 13
MARGIN = 4
LINES_PER_PAGE = (H - MARGIN * 2 - 14) // LINE_H  # reserve footer
MAX_COLS = 48

# Linux input_event: long tv_sec, long tv_usec, u16 type, u16 code, s32 value
EV_FORMAT = "llHHi"
EV_SIZE = struct.calcsize(EV_FORMAT)
EV_KEY = 0x01
EV_REL = 0x02  # relative axis (pointer). HDMI CEC pseudo-kb often advertises this.

# EVIOCGBIT(0, len) ioctl to query supported event types on a device.
# _IOC(_IOC_READ, 'E', 0x20, len) = 0x80004520 | (len<<16) on Linux.
EVIOCGBIT_0 = 0x80004520  # |= (len << 16) below

# Keycodes from <linux/input-event-codes.h>
KEY_ESC = 1
KEY_Q = 16
KEY_B = 48
KEY_J = 36
KEY_K = 37
KEY_N = 49
KEY_P = 25
KEY_SPACE = 57
KEY_UP = 103
KEY_DOWN = 108
KEY_PAGEUP = 104
KEY_PAGEDOWN = 109
KEY_HOME = 102
KEY_END = 107


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def image_to_rgb565(img):
    px = img.load()
    buf = bytearray(W * H * 2)
    i = 0
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y][:3]
            v = rgb565(r, g, b)
            buf[i] = v & 0xFF
            buf[i + 1] = (v >> 8) & 0xFF
            i += 2
    return bytes(buf)


def wrap(text):
    out = []
    for raw in text.splitlines():
        if not raw:
            out.append("")
            continue
        while len(raw) > MAX_COLS:
            cut = raw.rfind(" ", 0, MAX_COLS)
            if cut <= 0:
                cut = MAX_COLS
            out.append(raw[:cut])
            raw = raw[cut:].lstrip()
        out.append(raw)
    return out


def paginate(lines):
    pages = []
    for i in range(0, len(lines), LINES_PER_PAGE):
        pages.append(lines[i: i + LINES_PER_PAGE])
    return pages or [[""]]


def render(page, idx, total, font, font_sm):
    img = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(img)
    y = MARGIN
    for line in page:
        d.text((MARGIN, y), line, font=font, fill=(230, 230, 230))
        y += LINE_H
    d.text((MARGIN, H - 14),
           f"Page {idx + 1}/{total}   Space=next  b=prev  Esc=quit",
           font=font_sm, fill=(120, 120, 120))
    return img


def _device_supports(fd, ev_type):
    """Return True if this evdev node advertises the given EV_* type bit."""
    try:
        # 4 bytes is enough to cover EV_KEY(1), EV_REL(2), EV_ABS(3) etc.
        buf = bytearray(4)
        ioc = EVIOCGBIT_0 | (len(buf) << 16)
        fcntl.ioctl(fd, ioc, buf)
    except OSError:
        return False
    bits = int.from_bytes(buf, "little")
    return bool(bits & (1 << ev_type))


def open_input_devices():
    """Open evdev nodes non-blocking, preferring the TCA8418 keypad.

    HDMI CEC adapters expose a pseudo-keyboard that also reports EV_REL;
    reject those so media keys from a TV remote don't hijack our app.
    Prefer the by-path symlink, fall back to /dev/input/event* scan.
    """
    fds = []
    env = os.environ.get("INPUT_DEV", "").strip()
    candidates = []
    if env:
        candidates.append(env)
    # Primary CardputerZero keypad (TCA8418 on i2c).
    candidates.append("/dev/input/by-path/platform-3f804000.i2c-event")
    candidates.extend(sorted(glob.glob("/dev/input/event*")))
    seen = set()
    for path in candidates:
        try:
            real = os.path.realpath(path)
        except OSError:
            real = path
        if real in seen:
            continue
        seen.add(real)
        try:
            fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        except OSError:
            continue
        # Must emit key events, must NOT emit relative-axis (pointer) events.
        if not _device_supports(fd, EV_KEY) or _device_supports(fd, EV_REL):
            os.close(fd)
            continue
        fds.append(fd)
    return fds


def read_keycodes(fds, timeout=0.2):
    """Return list of keycodes newly pressed (value==1)."""
    pressed = []
    if not fds:
        return pressed
    try:
        dr, _, _ = select.select(fds, [], [], timeout)
    except (OSError, ValueError):
        return pressed
    for fd in dr:
        try:
            data = os.read(fd, EV_SIZE * 64)
        except BlockingIOError:
            continue
        except OSError:
            continue
        for off in range(0, len(data) - EV_SIZE + 1, EV_SIZE):
            _sec, _usec, etype, code, value = struct.unpack(
                EV_FORMAT, data[off:off + EV_SIZE]
            )
            if etype == EV_KEY and value == 1:
                pressed.append(code)
    return pressed


def main():
    if len(sys.argv) < 2:
        print("usage: reader.py <file.md>", file=sys.stderr)
        return 2
    with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
        text = f.read()

    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    font_sm = ImageFont.truetype(FONT_PATH, 9)

    pages = paginate(wrap(text))
    idx = 0

    fd = os.open(FB_DEV, os.O_RDWR)
    mm = mmap.mmap(fd, W * H * BPP, mmap.MAP_SHARED, mmap.PROT_WRITE | mmap.PROT_READ)

    input_fds = open_input_devices()

    next_keys = {KEY_SPACE, KEY_DOWN, KEY_PAGEDOWN, KEY_J, KEY_N}
    prev_keys = {KEY_B, KEY_UP, KEY_PAGEUP, KEY_K, KEY_P}
    home_keys = {KEY_HOME}
    end_keys = {KEY_END}
    quit_keys = {KEY_ESC, KEY_Q}

    try:
        # initial render
        img = render(pages[idx], idx, len(pages), font, font_sm)
        mm.seek(0)
        mm.write(image_to_rgb565(img))

        while True:
            codes = read_keycodes(input_fds, timeout=0.2)
            if not codes:
                continue
            dirty = False
            for code in codes:
                if code in quit_keys:
                    return 0
                if code in next_keys:
                    if idx + 1 < len(pages):
                        idx += 1
                        dirty = True
                elif code in prev_keys:
                    if idx > 0:
                        idx -= 1
                        dirty = True
                elif code in home_keys:
                    if idx != 0:
                        idx = 0
                        dirty = True
                elif code in end_keys:
                    if idx != len(pages) - 1:
                        idx = len(pages) - 1
                        dirty = True
            if dirty:
                img = render(pages[idx], idx, len(pages), font, font_sm)
                mm.seek(0)
                mm.write(image_to_rgb565(img))
    finally:
        for ifd in input_fds:
            try:
                os.close(ifd)
            except OSError:
                pass
        mm.close()
        os.close(fd)


if __name__ == "__main__":
    sys.exit(main() or 0)
