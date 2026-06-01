#!/usr/bin/env bash
#
# Run an example inside the ubuntu aarch64 container, with Xvfb on :99.
# If XQuartz is running (check `defaults read org.xquartz.X11 nolisten_tcp` = 0),
# DISPLAY will be forwarded to your Mac and the 320x170 Xvfb window appears.
#
# Usage: scripts/dev-on-mac/run-in-docker.sh <example_dir> [args...]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE="cardputerzero-devbox:ubuntu"

TARGET="${1:-}"
if [[ -z "$TARGET" ]]; then
    echo "usage: $0 <example_dir> [args...]" >&2
    exit 2
fi
shift

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "image missing; run scripts/dev-on-mac/setup-docker.sh first." >&2
    exit 1
fi

# Try to wire XQuartz if available; harmless if not.
XQUARTZ_ARGS=()
if [ -d /Applications/Utilities/XQuartz.app ] || [ -d /opt/X11 ]; then
    # Ensure xquartz allows network clients
    defaults write org.xquartz.X11 nolisten_tcp 0 >/dev/null 2>&1 || true
    open -a XQuartz 2>/dev/null || true
    if command -v xhost >/dev/null 2>&1; then xhost + 127.0.0.1 >/dev/null 2>&1 || true; fi
    XQUARTZ_ARGS=(-e "DISPLAY=host.docker.internal:0")
fi

exec docker run --rm -it \
    --platform linux/arm64 \
    -v "$REPO_ROOT":/work \
    -w /work \
    "${XQUARTZ_ARGS[@]}" \
    "$IMAGE" \
    bash /work/scripts/dev-on-mac/entrypoint.sh "$TARGET" run "$@"
