#!/usr/bin/env bash
set -e

if [ "$(uname)" != "Linux" ]; then
  echo "This example only runs on Linux (aarch64 M5 CardputerZero)." >&2
  exit 1
fi

DIR="$(cd "$(dirname "$0")" && pwd)"
exec sudo python3 "$DIR/fb_hello.py" "$@"
