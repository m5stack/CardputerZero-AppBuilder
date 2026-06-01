#!/usr/bin/env bash
# Path B: Xvfb + Python's own PIL mirror (no ffmpeg)
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"

Xvfb :1 -screen 0 320x170x16 -nolisten tcp &
XVFB_PID=$!
cleanup() {
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export DISPLAY=:1
sleep 0.5

python3 hello_tk.py --mirror-to-fb
