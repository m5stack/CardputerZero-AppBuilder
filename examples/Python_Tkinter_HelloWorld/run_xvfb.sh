#!/usr/bin/env bash
# Path A: Xvfb + ffmpeg mirror to /dev/fb0
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"

Xvfb :1 -screen 0 320x170x16 -nolisten tcp &
XVFB_PID=$!

blank_fb() {
  # Blank /dev/fb0 so APPLaunch's resumed UI is visible immediately
  # after we exit. 'pi' is in the 'video' group -> no sudo needed.
  # Read vinfo with python so we never hardcode resolution.
  python3 - <<'PY' 2>/dev/null || true
import fcntl, os, struct
FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602
try:
    fd = os.open("/dev/fb0", os.O_RDWR)
except OSError:
    raise SystemExit(0)
try:
    vbuf = bytearray(160)
    fcntl.ioctl(fd, FBIOGET_VSCREENINFO, vbuf)
    xres, yres, _xv, _yv, _xo, _yo, bpp, _g = struct.unpack_from("8I", vbuf, 0)
    fmt = "16sL" + "I"*6 + "HHHI"
    fbuf = bytearray(struct.calcsize(fmt))
    fcntl.ioctl(fd, FBIOGET_FSCREENINFO, fbuf)
    smem_len = struct.unpack(fmt, bytes(fbuf))[2]
    need = smem_len or xres*yres*max(1, bpp//8)
    os.lseek(fd, 0, 0)
    z = b"\x00" * 4096
    while need > 0:
        n = os.write(fd, z if need >= 4096 else z[:need])
        if n <= 0:
            break
        need -= n
finally:
    os.close(fd)
PY
}

cleanup() {
  if [ -n "${FFMPEG_PID:-}" ]; then
    kill "$FFMPEG_PID" 2>/dev/null || true
    # Give ffmpeg a moment to release /dev/fb0 before we blank it.
    sleep 0.1
  fi
  blank_fb
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export DISPLAY=:1
sleep 0.5

# 'pi' is in the 'video' group so we can write /dev/fb0 without sudo
# when using the Python blitter path. ffmpeg's fbdev muxer also works
# for group 'video' writers, so drop sudo here too.
ffmpeg -loglevel error -f x11grab -video_size 320x170 -framerate 15 -i :1 \
  -pix_fmt rgb565le -f fbdev /dev/fb0 &
FFMPEG_PID=$!

python3 hello_tk.py
