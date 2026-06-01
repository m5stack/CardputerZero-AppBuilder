#!/usr/bin/env bash
#
# Build and run an SDL2 example natively on macOS (no framebuffer needed).
#
# Usage: scripts/dev-on-mac/run-sdl.sh <example_dir>
#   example_dir is one of:  SDL2_HelloWorld  SDL2_Game
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

TARGET="${1:-}"
if [[ -z "$TARGET" ]]; then
    echo "usage: $0 <example_dir>" >&2
    echo "       e.g. $0 SDL2_HelloWorld" >&2
    exit 2
fi

DIR="$REPO_ROOT/$TARGET"
[[ -d "$DIR" ]] || { echo "not a directory: $DIR" >&2; exit 1; }

if ! command -v pkg-config >/dev/null 2>&1; then
    echo "pkg-config missing; run: brew install pkg-config sdl2 sdl2_ttf sdl2_mixer" >&2
    exit 1
fi
if ! pkg-config --exists sdl2; then
    echo "SDL2 missing; run: brew install sdl2 sdl2_ttf sdl2_mixer" >&2
    exit 1
fi

cd "$DIR"

echo "-- build ($TARGET, macOS native)"
# Point CFLAGS at Homebrew include; use Make if a Makefile exists, else fall
# back to a one-liner clang build.
if [[ -f Makefile ]]; then
    # Some Makefiles hardcode $(shell pkg-config ...) — that just works.
    make
else
    CFLAGS="$(pkg-config --cflags sdl2 SDL2_ttf SDL2_mixer)"
    LDFLAGS="$(pkg-config --libs sdl2 SDL2_ttf SDL2_mixer)"
    clang $CFLAGS main.c -o hello_sdl2 $LDFLAGS
fi

# Point DejaVuSans lookup at a real mac font if our source hardcodes the
# Debian path. We export CARDPUTERZERO_TTF and apps that honor it win;
# fall back is silent so even apps that ignore this env will boot and just
# render without TTF.
FONT="/System/Library/Fonts/Supplemental/Arial.ttf"
[[ -f "$FONT" ]] || FONT="/System/Library/Fonts/HelveticaNeue.ttc"
export CARDPUTERZERO_TTF="$FONT"

# Locate whatever binary the build produced.
BIN=""
for candidate in hello_sdl2 breakout sdl2_hello sdl2_game sdl_app; do
    if [[ -x "./$candidate" ]]; then BIN="./$candidate"; break; fi
done
if [[ -z "$BIN" ]]; then
    BIN="$(find . -maxdepth 1 -type f -perm -u+x ! -name '*.sh' ! -name 'Makefile' | head -1)"
fi
[[ -n "$BIN" && -x "$BIN" ]] || { echo "no built binary found in $DIR" >&2; exit 1; }

echo "-- run $BIN"
echo "   (close the SDL window or press ESC to exit)"
exec "$BIN"
