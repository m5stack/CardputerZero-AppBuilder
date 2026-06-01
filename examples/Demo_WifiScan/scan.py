#!/usr/bin/env python3
"""Demo_WifiScan: scan nearby WiFi networks and render top 10 on fb0.

Keys (evdev keycodes):
  Up / k   - scroll up
  Down / j - scroll down
  R        - rescan
  Esc / Q  - quit
"""

import glob
import mmap
import os
import re
import select
import struct
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

FB_DEV = "/dev/fb0"
W, H = 320, 170
BPP = 2
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
VISIBLE_ROWS = 10
ROW_H = 14

# Linux input event struct: long tv_sec, long tv_usec, __u16 type, __u16 code, __s32 value
# On aarch64/x86_64 (time_t=8) this is: qqHHi + 2 bytes pad = 24 bytes
EV_FORMAT = "llHHi"
EV_SIZE = struct.calcsize(EV_FORMAT)
EV_KEY = 0x01

# Relevant keycodes from <linux/input-event-codes.h>
KEY_ESC = 1
KEY_Q = 16
KEY_R = 19
KEY_UP = 103
KEY_DOWN = 108
KEY_J = 36
KEY_K = 37


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


def scan():
    """Return list of (ssid, signal_dbm). Requires sudo for iw scan."""
    try:
        out = subprocess.check_output(
            ["sudo", "iw", "dev", "wlan0", "scan"],
            text=True, stderr=subprocess.DEVNULL, timeout=15,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return []

    nets = []
    cur_ssid = None
    cur_sig = None
    for line in out.splitlines():
        if line.startswith("BSS "):
            if cur_ssid:
                nets.append((cur_ssid, cur_sig if cur_sig is not None else -100))
            cur_ssid, cur_sig = None, None
        m = re.match(r"\s*signal:\s*(-?\d+\.?\d*)", line)
        if m:
            cur_sig = float(m.group(1))
        m = re.match(r"\s*SSID:\s*(.*)", line)
        if m:
            cur_ssid = m.group(1).strip() or "<hidden>"
    if cur_ssid:
        nets.append((cur_ssid, cur_sig if cur_sig is not None else -100))

    nets.sort(key=lambda n: n[1], reverse=True)
    return nets


def render(nets, offset, font, font_sm):
    img = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(img)
    d.text((6, 2), f"WiFi Scan ({len(nets)})", font=font, fill=(255, 255, 0))
    visible = nets[offset: offset + VISIBLE_ROWS]
    y = 20
    for ssid, sig in visible:
        bar = max(0, min(100, int((sig + 90) * 100 / 60)))
        color = (0, 255, 100) if bar > 60 else (255, 200, 0) if bar > 30 else (255, 80, 80)
        d.text((6, y), ssid[:24], font=font_sm, fill=(220, 220, 220))
        d.text((220, y), f"{int(sig):>4} dBm", font=font_sm, fill=color)
        y += ROW_H
    d.text((6, H - 12), "R=rescan  Up/Down=scroll  Esc=quit",
           font=font_sm, fill=(100, 100, 100))
    return img


def write_fb(img, mm):
    mm.seek(0)
    mm.write(image_to_rgb565(img))


def open_input_devices():
    """Open all /dev/input/event* nodes in non-blocking mode.

    Using multiple devices lets us work on any board layout; the primary
    TCA8418 keyboard is typically /dev/input/by-path/platform-3f804000.i2c-event.
    """
    fds = []
    env = os.environ.get("INPUT_DEV", "").strip()
    candidates = []
    if env:
        candidates.append(env)
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
        fds.append(fd)
    return fds


def read_keycodes(fds, timeout=0.1):
    """Return list of keycodes pressed (value==1) across all input fds."""
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
    font = ImageFont.truetype(FONT_PATH, 14)
    font_sm = ImageFont.truetype(FONT_PATH, 11)

    fd = os.open(FB_DEV, os.O_RDWR)
    mm = mmap.mmap(fd, W * H * BPP, mmap.MAP_SHARED, mmap.PROT_WRITE | mmap.PROT_READ)

    input_fds = open_input_devices()

    nets = scan()
    offset = 0
    write_fb(render(nets, offset, font, font_sm), mm)

    try:
        while True:
            codes = read_keycodes(input_fds, timeout=0.1)
            if not codes:
                continue
            dirty = False
            for code in codes:
                if code in (KEY_ESC, KEY_Q):
                    return 0
                if code == KEY_R:
                    nets = scan()
                    offset = 0
                    dirty = True
                elif code in (KEY_DOWN, KEY_J):
                    if offset + VISIBLE_ROWS < len(nets):
                        offset += 1
                        dirty = True
                elif code in (KEY_UP, KEY_K):
                    if offset > 0:
                        offset -= 1
                        dirty = True
            if dirty:
                write_fb(render(nets, offset, font, font_sm), mm)
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
