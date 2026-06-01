#!/usr/bin/env bash
#
# One-time macOS setup for running examples inside an aarch64 ubuntu container.
#
# - verifies Docker Desktop is running
# - installs XQuartz (needed for GUI forwarding) via brew if missing
# - builds the local image `cardputerzero-devbox:ubuntu`
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE="cardputerzero-devbox:ubuntu"

if ! command -v docker >/dev/null 2>&1; then
    cat >&2 <<EOM
docker not found on PATH. Install Docker Desktop from
  https://www.docker.com/products/docker-desktop/
then re-run this script.
EOM
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    echo "docker daemon not reachable — start Docker Desktop, then re-run." >&2
    exit 1
fi

# XQuartz is only needed if you want to see the Xvfb output in a window on Mac.
# Feel free to skip if you're only running smoke-deb.sh.
if ! [ -d /Applications/Utilities/XQuartz.app ] && ! command -v xquartz >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        echo "-- installing XQuartz via brew (optional, for GUI mirroring)"
        brew install --cask xquartz || true
    else
        echo "note: XQuartz not installed. GUI mirroring won't work, but CLI-only runs will."
    fi
fi

echo "-- docker build $IMAGE"
docker build --platform linux/arm64 \
    -t "$IMAGE" \
    -f "$SCRIPT_DIR/Dockerfile.ubuntu" \
    "$SCRIPT_DIR"

echo
echo "done. try:"
echo "  scripts/dev-on-mac/smoke-deb.sh FrameBuffer_HelloWorld"
echo "  scripts/dev-on-mac/run-in-docker.sh Python_FrameBuffer_HelloWorld"
