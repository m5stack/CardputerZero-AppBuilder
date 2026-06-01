#!/usr/bin/env python3
"""Demo_SysDashboard: CPU / Mem / Temp / IP dashboard on /dev/fb0.

Reads stats from procfs/sysfs, renders with PIL, writes RGB565 via mmap.
Reference: ../Python_FrameBuffer_HelloWorld README for the mmap pattern.
"""

import mmap
import os
import re
import struct
import subprocess
import time

from PIL import Image, ImageDraw, ImageFont

FB_DEV = "/dev/fb0"
W, H = 320, 170
BPP = 2  # RGB565
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def image_to_rgb565(img):
    px = img.load()
    out = bytearray(W * H * 2)
    i = 0
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y][:3]
            v = rgb565(r, g, b)
            out[i] = v & 0xFF
            out[i + 1] = (v >> 8) & 0xFF
            i += 2
    return bytes(out)


class CPUStat:
    def __init__(self):
        self.prev_idle = 0
        self.prev_total = 0

    def read(self):
        with open("/proc/stat") as f:
            line = f.readline()
        parts = [int(x) for x in line.split()[1:]]
        idle = parts[3] + parts[4]
        total = sum(parts)
        d_idle = idle - self.prev_idle
        d_total = total - self.prev_total
        self.prev_idle, self.prev_total = idle, total
        if d_total <= 0:
            return 0.0
        return 100.0 * (d_total - d_idle) / d_total


def read_mem():
    info = {}
    with open("/proc/meminfo") as f:
        for line in f:
            k, _, v = line.partition(":")
            info[k.strip()] = int(v.strip().split()[0])  # kB
    total = info.get("MemTotal", 1)
    avail = info.get("MemAvailable", info.get("MemFree", 0))
    used_pct = 100.0 * (total - avail) / total
    return used_pct, total // 1024, (total - avail) // 1024  # pct, MB total, MB used


def read_temp():
    try:
        with open("/sys/class/thermal/thermal_zone0/temp") as f:
            return int(f.read().strip()) / 1000.0
    except OSError:
        return 0.0


def read_ip(iface="wlan0"):
    try:
        out = subprocess.check_output(
            ["ip", "-4", "addr", "show", iface], text=True, stderr=subprocess.DEVNULL
        )
        m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)", out)
        return m.group(1) if m else "-"
    except (OSError, subprocess.CalledProcessError):
        return "-"


def main():
    font_big = ImageFont.truetype(FONT_PATH, 20)
    font_med = ImageFont.truetype(FONT_PATH, 14)
    font_sm = ImageFont.truetype(FONT_PATH, 12)

    cpu = CPUStat()
    cpu.read()  # prime

    fd = os.open(FB_DEV, os.O_RDWR)
    mm = mmap.mmap(fd, W * H * BPP, mmap.MAP_SHARED, mmap.PROT_WRITE | mmap.PROT_READ)

    try:
        while True:
            cpu_pct = cpu.read()
            mem_pct, mem_total_mb, mem_used_mb = read_mem()
            temp_c = read_temp()
            ip_addr = read_ip()

            img = Image.new("RGB", (W, H), (0, 0, 0))
            d = ImageDraw.Draw(img)
            d.text((8, 4), "SysDashboard", font=font_big, fill=(255, 255, 255))

            y = 36
            d.text((8, y), f"CPU  {cpu_pct:5.1f}%", font=font_med, fill=(0, 255, 128))
            d.rectangle((140, y + 3, 140 + 160, y + 14), outline=(80, 80, 80))
            d.rectangle((141, y + 4, 141 + int(158 * cpu_pct / 100), y + 13),
                        fill=(0, 200, 100))

            y += 22
            d.text((8, y), f"MEM  {mem_pct:5.1f}%", font=font_med, fill=(0, 200, 255))
            d.rectangle((140, y + 3, 140 + 160, y + 14), outline=(80, 80, 80))
            d.rectangle((141, y + 4, 141 + int(158 * mem_pct / 100), y + 13),
                        fill=(0, 150, 220))

            y += 22
            d.text((8, y), f"TEMP {temp_c:5.1f} C", font=font_med, fill=(255, 180, 80))

            y += 22
            d.text((8, y), f"IP   {ip_addr}", font=font_med, fill=(220, 220, 220))

            d.text((8, H - 16),
                   f"{mem_used_mb}/{mem_total_mb} MB  wlan0",
                   font=font_sm, fill=(120, 120, 120))

            mm.seek(0)
            mm.write(image_to_rgb565(img))
            time.sleep(1.0)
    finally:
        mm.close()
        os.close(fd)


if __name__ == "__main__":
    main()
