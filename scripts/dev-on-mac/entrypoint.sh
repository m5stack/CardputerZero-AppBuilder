#!/usr/bin/env bash
#
# Container entrypoint. Args:
#   $1 example dir (relative to /work)
#   $2 mode:  run | smoke-deb
#   $@ rest  passed to the example
set -euo pipefail

TARGET="${1:-}"
MODE="${2:-run}"
shift || true
shift || true

if [[ -z "$TARGET" ]]; then
    echo "usage: entrypoint.sh <example_dir> [run|smoke-deb] [extra args...]" >&2
    exit 2
fi

DIR="/work/$TARGET"
[[ -d "$DIR" ]] || { echo "missing: $DIR" >&2; exit 1; }

# Boot Xvfb on :99 so any GUI example has a DISPLAY.
if ! pgrep -f 'Xvfb :99' >/dev/null 2>&1; then
    Xvfb :99 -screen 0 320x170x24 -nolisten tcp &
    # wait for it
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        xdpyinfo -display :99 >/dev/null 2>&1 && break
        sleep 0.2
    done
fi
export DISPLAY=:99
export FB_DEVICE=/fbfile
export CARDPUTERZERO_TTF="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

case "$MODE" in
    smoke-deb)
        OUT_DIR="/work/dist"
        mkdir -p "$OUT_DIR"
        /work/scripts/pack-deb.sh "$DIR" --output-dir "$OUT_DIR"
        # shellcheck disable=SC1091
        source "$DIR/packaging/meta.env"
        DEB="$OUT_DIR/${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION:-m5stack1}_${PKG_ARCH:-arm64}.deb"
        echo "-- dpkg -i $DEB"
        apt-get install -y --no-install-recommends "$DEB" || dpkg -i "$DEB" || true
        echo "-- installed files:"
        dpkg -L "$PKG_NAME" | sed 's/^/   /'
        echo "-- rendered .desktop:"
        sed 's/^/   /' "/usr/share/APPLaunch/applications/${PKG_NAME}.desktop"
        echo "-- bin exec:"
        ls -la "/usr/share/APPLaunch/bin/${PKG_NAME}" 2>/dev/null || true
        ;;
    run)
        # Build via packaging/build.sh if present; otherwise fall back to make/cargo.
        if [[ -x "$DIR/packaging/build.sh" ]]; then
            ( cd "$DIR" && ./packaging/build.sh )
        elif [[ -f "$DIR/Makefile" ]]; then
            make -C "$DIR"
        elif [[ -f "$DIR/Cargo.toml" ]]; then
            ( cd "$DIR" && cargo build --release )
        elif [[ -f "$DIR/CMakeLists.txt" ]]; then
            cmake -S "$DIR" -B "$DIR/build" -DCMAKE_BUILD_TYPE=Release
            cmake --build "$DIR/build" -j
        fi
        # Stage into a throwaway prefix so we know what the wrapper would do.
        STAGE="$(mktemp -d)"
        export STAGE INSTALL_PREFIX=/usr/share/APPLaunch
        # shellcheck disable=SC1091
        source "$DIR/packaging/meta.env"
        APP_INSTALL_DIR="$INSTALL_PREFIX/apps/$PKG_NAME"
        export APP_INSTALL_DIR PKG_NAME
        mkdir -p "$STAGE$INSTALL_PREFIX/bin" "$STAGE$APP_INSTALL_DIR"
        ( cd "$DIR" && ./packaging/stage.sh )
        # Install into real / so wrapper paths resolve
        sudo -n cp -a "$STAGE"/. / 2>/dev/null || cp -a "$STAGE"/. /
        echo "-- launching /usr/share/APPLaunch/bin/$PKG_NAME"
        echo "   (XQuartz should show the Xvfb :99 window if started)"
        exec "/usr/share/APPLaunch/bin/$PKG_NAME" "$@"
        ;;
    *)
        echo "unknown mode: $MODE" >&2
        exit 2
        ;;
esac
