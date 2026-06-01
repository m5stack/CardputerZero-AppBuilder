#!/usr/bin/env python3
"""Tkinter Hello demo for CardputerZero 320x170 LCD.

Requires an X server (Xvfb or Xfbdev). With --mirror-to-fb a background
thread grabs the root window via PIL.ImageGrab and blits RGB565 to /dev/fb0.
"""

import argparse
import atexit
import fcntl
import os
import struct
import sys
import threading
import time
import tkinter as tk
from datetime import datetime

W, H = 320, 170

FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602


def _blank_fb(path="/dev/fb0"):
    """Zero /dev/fb0 on exit so APPLaunch's resumed screen shows without
    waiting for a left/right key to trigger repaint."""
    try:
        fd = os.open(path, os.O_RDWR)
    except OSError:
        return
    try:
        vbuf = bytearray(160)
        fcntl.ioctl(fd, FBIOGET_VSCREENINFO, vbuf)
        xres, yres, _xv, _yv, _xo, _yo, bpp, _g = struct.unpack_from("8I", vbuf, 0)
        fmt = "16sL" + "I" * 6 + "HHHI"
        fbuf = bytearray(struct.calcsize(fmt))
        fcntl.ioctl(fd, FBIOGET_FSCREENINFO, fbuf)
        smem_len = struct.unpack(fmt, bytes(fbuf))[2]
        need = smem_len or xres * yres * max(1, bpp // 8)
        os.lseek(fd, 0, 0)
        z = b"\x00" * 4096
        while need > 0:
            n = os.write(fd, z if need >= 4096 else z[:need])
            if n <= 0:
                break
            need -= n
    except Exception:
        pass
    finally:
        os.close(fd)


def _start_fb_mirror(display, fps=15):
    from PIL import ImageGrab
    import fb_blit

    mm, vinfo, line_length, fd = fb_blit.open_fb("/dev/fb0")
    period = 1.0 / fps

    def loop():
        try:
            while True:
                t0 = time.monotonic()
                try:
                    img = ImageGrab.grab(xdisplay=display)
                except Exception as e:
                    print("grab failed: {}".format(e), file=sys.stderr)
                    time.sleep(0.2)
                    continue
                try:
                    fb_blit.blit_pil(mm, vinfo, line_length, img)
                except Exception as e:
                    print("blit failed: {}".format(e), file=sys.stderr)
                dt = time.monotonic() - t0
                if dt < period:
                    time.sleep(period - dt)
        finally:
            try:
                mm.close()
            finally:
                os.close(fd)

    t = threading.Thread(target=loop, daemon=True)
    t.start()
    return t


def build_ui():
    root = tk.Tk()
    root.title("CardputerZero Tk Hello")
    try:
        root.overrideredirect(True)
    except tk.TclError:
        pass
    root.geometry("{}x{}+0+0".format(W, H))
    root.configure(bg="black")

    canvas = tk.Canvas(root, width=W, height=H, bg="black",
                       highlightthickness=0, bd=0)
    canvas.pack(fill="both", expand=True)

    canvas.create_rectangle(1, 1, W - 2, H - 2, outline="white", width=2)

    canvas.create_text(W // 2, H // 2 - 16,
                       text="Hello CardputerZero",
                       fill="white",
                       font=("DejaVu Sans", 14, "bold"))

    clock_id = canvas.create_text(W // 2, H // 2 + 18,
                                  text="",
                                  fill="#B4DCFF",
                                  font=("DejaVu Sans", 12))

    def tick():
        stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        canvas.itemconfigure(clock_id, text=stamp)
        root.after(1000, tick)

    tick()

    def quit_app(_event=None):
        root.destroy()

    root.bind("<Escape>", quit_app)
    root.bind("q", quit_app)
    root.bind("Q", quit_app)

    return root


def main():
    atexit.register(_blank_fb)

    parser = argparse.ArgumentParser()
    parser.add_argument("--mirror-to-fb", action="store_true",
                        help="mirror root window to /dev/fb0 via PIL+mmap")
    parser.add_argument("--fps", type=int, default=15)
    args = parser.parse_args()

    display = os.environ.get("DISPLAY", ":1")
    if not os.environ.get("DISPLAY"):
        os.environ["DISPLAY"] = display

    root = build_ui()

    if args.mirror_to_fb:
        _start_fb_mirror(display, fps=args.fps)

    try:
        root.mainloop()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
