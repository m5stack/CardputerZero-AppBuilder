#!/usr/bin/env python3
"""Demo_ImageViewer: browse JPG/PNG from a directory on /dev/fb0.

Usage: viewer.py <dir>
Keys: Left/Right prev/next, Esc quit.
"""

import errno
import grp
import mmap
import os
import select
import sys

from PIL import Image, ImageDraw, ImageFont

FB_DEV = "/dev/fb0"
W, H = 320, 170
BPP = 2
EXTS = (".jpg", ".jpeg", ".png", ".bmp", ".gif")


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


def list_images(d):
    try:
        files = sorted(f for f in os.listdir(d) if f.lower().endswith(EXTS))
    except OSError:
        files = []
    return [os.path.join(d, f) for f in files]


def load_and_fit(path):
    canvas = Image.new("RGB", (W, H), (0, 0, 0))
    try:
        im = Image.open(path).convert("RGB")
    except OSError:
        return canvas
    im.thumbnail((W, H), Image.LANCZOS)
    x = (W - im.width) // 2
    y = (H - im.height) // 2
    canvas.paste(im, (x, y))
    return canvas


def banner_image(lines):
    canvas = Image.new("RGB", (W, H), (0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    try:
        font = ImageFont.load_default()
    except OSError:
        font = None
    y = 20
    for line in lines:
        draw.text((10, y), line, fill=(255, 255, 255), font=font)
        y += 16
    return canvas


def read_key():
    dr, _, _ = select.select([sys.stdin], [], [], 0.2)
    if not dr:
        return ""
    try:
        return sys.stdin.read(1)
    except OSError:
        return ""


def groups_of(user):
    try:
        return [g.gr_name for g in grp.getgrall() if user in g.gr_mem]
    except OSError:
        return []


def open_fb():
    """Open /dev/fb0 O_RDWR; return fd or raise with a clear message."""
    try:
        return os.open(FB_DEV, os.O_RDWR)
    except PermissionError as e:
        user = os.environ.get("USER") or ""
        if not user:
            try:
                import pwd
                user = pwd.getpwuid(os.getuid()).pw_name
            except Exception:
                user = "?"
        gs = groups_of(user)
        print(
            "ERROR: EACCES opening %s as user=%s (uid=%d). "
            "Groups for this user: %s. "
            "Need membership in 'video'. If just added via usermod, the "
            "session must be restarted (systemctl restart APPLaunch.service). "
            "NOT re-introducing sudo -- see packaging/README." % (
                FB_DEV, user, os.getuid(), ",".join(gs) or "(none)"),
            file=sys.stderr,
        )
        raise
    except FileNotFoundError:
        print("ERROR: %s does not exist. Is the fbcon driver loaded?" % FB_DEV,
              file=sys.stderr)
        raise
    except OSError as e:
        print("ERROR: open(%s) failed: errno=%d %s" % (
            FB_DEV, e.errno or 0, os.strerror(e.errno or 0)), file=sys.stderr)
        raise


def main():
    # Startup banner (diagnostic; captured by wrapper's log redirect).
    print(
        "demo-imgview start: pid=%d uid=%d gid=%d argv=%r" % (
            os.getpid(), os.getuid(), os.getgid(), sys.argv),
        file=sys.stderr,
    )
    sys.stderr.flush()

    if len(sys.argv) < 2:
        print("usage: viewer.py <dir>", file=sys.stderr)
        return 2

    d = sys.argv[1]
    files = list_images(d)
    missing = not os.path.isdir(d)
    print("demo-imgview: dir=%s missing=%s count=%d" % (d, missing, len(files)),
          file=sys.stderr)
    sys.stderr.flush()

    try:
        fd = open_fb()
    except OSError as e:
        # Already logged a clear message above.
        return 3 if e.errno == errno.EACCES else 4

    try:
        mm = mmap.mmap(fd, W * H * BPP, mmap.MAP_SHARED,
                       mmap.PROT_WRITE | mmap.PROT_READ)
    except OSError as e:
        print("ERROR: mmap(%s) failed: %s" % (FB_DEV, e), file=sys.stderr)
        os.close(fd)
        return 5

    try:
        if not files:
            # Draw a "no images" banner and wait for ESC.
            msg = "No images found" if not missing else "Missing samples dir"
            img = banner_image([
                msg,
                d if len(d) < 40 else "..." + d[-37:],
                "Press ESC to quit",
            ])
            mm.seek(0)
            mm.write(image_to_rgb565(img))
            while True:
                k = read_key()
                if k == "\x1b":
                    break
            return 0

        idx = 0
        while True:
            img = load_and_fit(files[idx])
            mm.seek(0)
            mm.write(image_to_rgb565(img))
            k = ""
            while not k:
                k = read_key()
            if k == "\x1b":
                break
            if k in ("n", "l", "d"):
                idx = (idx + 1) % len(files)
            elif k in ("p", "h", "a"):
                idx = (idx - 1) % len(files)
    finally:
        mm.close()
        os.close(fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
