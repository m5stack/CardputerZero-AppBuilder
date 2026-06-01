#!/usr/bin/env bash
set -euo pipefail
apt-get install -y --no-install-recommends \
    cmake build-essential pkg-config git \
    libsdl2-dev libsdl2-ttf-dev
