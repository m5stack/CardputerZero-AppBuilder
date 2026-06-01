#!/usr/bin/env bash
#
# Build the example's .deb inside the ubuntu aarch64 container, dpkg -i it, and
# verify the APPLaunch layout (.desktop rendered, bin wrapper executable).
#
# Usage: scripts/dev-on-mac/smoke-deb.sh <example_dir>
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE="cardputerzero-devbox:ubuntu"

TARGET="${1:-}"
if [[ -z "$TARGET" ]]; then
    echo "usage: $0 <example_dir>" >&2
    exit 2
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "image missing; run scripts/dev-on-mac/setup-docker.sh first." >&2
    exit 1
fi

exec docker run --rm -t \
    --platform linux/arm64 \
    -v "$REPO_ROOT":/work \
    -w /work \
    "$IMAGE" \
    bash /work/scripts/dev-on-mac/entrypoint.sh "$TARGET" smoke-deb
