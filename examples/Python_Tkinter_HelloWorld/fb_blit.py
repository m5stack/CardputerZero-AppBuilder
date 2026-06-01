"""Framebuffer helpers: open /dev/fb0, query vinfo/finfo, blit PIL RGB to RGB565."""

import fcntl
import mmap
import os
import struct

import numpy as np

FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602

VAR_FMT = "8I"
FIX_FMT = "16sL" + "I" * 6 + "HHHI"


def _read_vinfo(fd):
    buf = bytearray(160)
    fcntl.ioctl(fd, FBIOGET_VSCREENINFO, buf)
    xres, yres, xres_v, yres_v, xoff, yoff, bpp, grayscale = struct.unpack_from(VAR_FMT, buf, 0)
    return {
        "xres": xres,
        "yres": yres,
        "xres_virtual": xres_v,
        "yres_virtual": yres_v,
        "xoffset": xoff,
        "yoffset": yoff,
        "bits_per_pixel": bpp,
        "grayscale": grayscale,
    }


def _read_finfo(fd):
    buf = bytearray(struct.calcsize(FIX_FMT))
    fcntl.ioctl(fd, FBIOGET_FSCREENINFO, buf)
    fields = struct.unpack(FIX_FMT, buf)
    return {"smem_len": fields[2], "line_length": fields[8]}


def open_fb(path="/dev/fb0"):
    fd = os.open(path, os.O_RDWR)
    try:
        vinfo = _read_vinfo(fd)
        finfo = _read_finfo(fd)
    except Exception:
        os.close(fd)
        raise
    if vinfo["bits_per_pixel"] != 16:
        os.close(fd)
        raise RuntimeError("expected 16bpp RGB565, got {}".format(vinfo["bits_per_pixel"]))
    mm = mmap.mmap(fd, finfo["smem_len"], mmap.MAP_SHARED,
                   mmap.PROT_READ | mmap.PROT_WRITE)
    return mm, vinfo, finfo["line_length"], fd


def _pil_to_rgb565(img):
    arr = np.asarray(img, dtype=np.uint16)
    r = (arr[..., 0] >> 3) & 0x1F
    g = (arr[..., 1] >> 2) & 0x3F
    b = (arr[..., 2] >> 3) & 0x1F
    return ((r << 11) | (g << 5) | b).astype("<u2")


def blit_pil(mm, vinfo, line_length, pil_image):
    w, h = vinfo["xres"], vinfo["yres"]
    img = pil_image
    if img.mode != "RGB":
        img = img.convert("RGB")
    if img.size != (w, h):
        iw, ih = img.size
        scale = min(w / iw, h / ih)
        nw, nh = max(1, int(iw * scale)), max(1, int(ih * scale))
        img = img.resize((nw, nh))
        from PIL import Image
        canvas = Image.new("RGB", (w, h), (0, 0, 0))
        canvas.paste(img, ((w - nw) // 2, (h - nh) // 2))
        img = canvas
    rgb565 = _pil_to_rgb565(img)
    row_bytes = w * 2
    if line_length == row_bytes:
        mm.seek(0)
        mm.write(rgb565.tobytes())
        return
    raw = rgb565.tobytes()
    for y in range(h):
        mm.seek(y * line_length)
        mm.write(raw[y * row_bytes:(y + 1) * row_bytes])
