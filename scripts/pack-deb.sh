#!/usr/bin/env bash
#
# Usage: scripts/pack-deb.sh <example_dir> [--skip-build] [--output-dir DIR]
#
# Produces an APPLaunch-compatible .deb at $OUTPUT_DIR/<pkg>_<ver>-<rev>_arm64.deb.
#
# Each example that opts in MUST provide:
#   <example_dir>/packaging/meta.env      # PKG_NAME, PKG_VERSION, PKG_REVISION,
#                                         # PKG_DESC, PKG_DEPENDS, APP_NAME,
#                                         # APP_EXEC, APP_TERMINAL, APP_ICON
#   <example_dir>/packaging/build.sh      # compiles whatever is needed (idempotent).
#                                         # runs in the example directory.
#   <example_dir>/packaging/stage.sh      # copies artifacts under $STAGE/usr/share/APPLaunch/...
#                                         # receives $STAGE + $INSTALL_PREFIX + $APP_INSTALL_DIR.
#
# Optional files:
#   <example_dir>/packaging/app.desktop.in  # template; {{APP_NAME}} etc substituted
#   <example_dir>/packaging/icon.png        # 48x48 recommended
#   <example_dir>/packaging/postinst        # passed through if present
#   <example_dir>/packaging/prerm
#
# APPLaunch runtime layout (from doc/APPLaunch-App-打包指南.md):
#   /usr/share/APPLaunch/
#       applications/<pkg>.desktop
#       bin/<exec>
#       share/images/<icon>.png
#       share/font/*.ttf     # optional
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_BUILD=0
OUTPUT_DIR="$REPO_ROOT/dist"
EXAMPLE_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-build) SKIP_BUILD=1; shift ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*) echo "unknown flag: $1" >&2; exit 2 ;;
        *)
            if [[ -z "$EXAMPLE_DIR" ]]; then EXAMPLE_DIR="$1"; else
                echo "unexpected arg: $1" >&2; exit 2
            fi
            shift
            ;;
    esac
done

if [[ -z "$EXAMPLE_DIR" ]]; then
    echo "usage: $0 <example_dir> [--skip-build] [--output-dir DIR]" >&2
    exit 2
fi

EXAMPLE_DIR="$(cd "$EXAMPLE_DIR" && pwd)"
PKG_DIR="$EXAMPLE_DIR/packaging"
if [[ ! -d "$PKG_DIR" ]]; then
    echo "error: $EXAMPLE_DIR has no packaging/ directory" >&2
    exit 1
fi
if [[ ! -f "$PKG_DIR/meta.env" ]]; then
    echo "error: $PKG_DIR/meta.env missing" >&2
    exit 1
fi

# shellcheck disable=SC1091
source "$PKG_DIR/meta.env"

: "${PKG_NAME:?PKG_NAME required in meta.env}"
: "${PKG_VERSION:?PKG_VERSION required in meta.env}"
: "${PKG_REVISION:=m5stack1}"
: "${PKG_DESC:?PKG_DESC required in meta.env}"
: "${PKG_DEPENDS:=}"
: "${APP_NAME:=$PKG_NAME}"
: "${APP_EXEC:?APP_EXEC required in meta.env (absolute path or command)}"
: "${APP_TERMINAL:=false}"
: "${APP_ICON:=share/images/${PKG_NAME}.png}"
: "${APP_SYSPLAUSE:=true}"

ARCH="${PKG_ARCH:-arm64}"
INSTALL_PREFIX="/usr/share/APPLaunch"
APP_INSTALL_DIR="$INSTALL_PREFIX/apps/$PKG_NAME"

mkdir -p "$OUTPUT_DIR"

echo "==[ $PKG_NAME $PKG_VERSION-$PKG_REVISION ($ARCH) ]=="

# 1. Build phase
if [[ $SKIP_BUILD -eq 0 ]]; then
    if [[ -x "$PKG_DIR/build.sh" ]]; then
        echo "-- build.sh"
        ( cd "$EXAMPLE_DIR" && "$PKG_DIR/build.sh" )
    else
        echo "-- no build.sh; skipping build step"
    fi
fi

# 2. Stage into fakeroot
STAGE="$(mktemp -d -t pack-deb-XXXXXXXX)"
trap 'rm -rf "$STAGE"' EXIT

mkdir -p \
    "$STAGE/DEBIAN" \
    "$STAGE$INSTALL_PREFIX/applications" \
    "$STAGE$INSTALL_PREFIX/bin" \
    "$STAGE$INSTALL_PREFIX/share/images" \
    "$STAGE$APP_INSTALL_DIR"

# 3. Copy icon if provided, otherwise drop placeholder
if [[ -f "$PKG_DIR/icon.png" ]]; then
    cp "$PKG_DIR/icon.png" "$STAGE$INSTALL_PREFIX/share/images/${PKG_NAME}.png"
elif [[ -f "$REPO_ROOT/scripts/default-icon.png" ]]; then
    cp "$REPO_ROOT/scripts/default-icon.png" "$STAGE$INSTALL_PREFIX/share/images/${PKG_NAME}.png"
fi

# 4. Run example's stage.sh to place binaries/assets
if [[ -x "$PKG_DIR/stage.sh" ]]; then
    echo "-- stage.sh"
    export STAGE INSTALL_PREFIX APP_INSTALL_DIR PKG_NAME
    ( cd "$EXAMPLE_DIR" && "$PKG_DIR/stage.sh" )
else
    echo "error: $PKG_DIR/stage.sh missing or not executable" >&2
    exit 1
fi

# 5. Render .desktop
DESKTOP_OUT="$STAGE$INSTALL_PREFIX/applications/${PKG_NAME}.desktop"
if [[ -f "$PKG_DIR/app.desktop.in" ]]; then
    sed \
        -e "s|{{APP_NAME}}|$APP_NAME|g" \
        -e "s|{{APP_EXEC}}|$APP_EXEC|g" \
        -e "s|{{APP_TERMINAL}}|$APP_TERMINAL|g" \
        -e "s|{{APP_ICON}}|$APP_ICON|g" \
        -e "s|{{APP_SYSPLAUSE}}|$APP_SYSPLAUSE|g" \
        "$PKG_DIR/app.desktop.in" > "$DESKTOP_OUT"
else
    cat > "$DESKTOP_OUT" <<EOF
[Desktop Entry]
Name=$APP_NAME
Exec=$APP_EXEC
Terminal=$APP_TERMINAL
Sysplause=$APP_SYSPLAUSE
Icon=$APP_ICON
Type=Application
EOF
fi

# 6. DEBIAN/control
PACKAGED_DATE="$(date -u '+%Y-%m-%d %H:%M:%S')"
{
    echo "Package: $PKG_NAME"
    echo "Version: $PKG_VERSION"
    echo "Architecture: $ARCH"
    echo "Maintainer: ${PKG_MAINTAINER:-CardputerZero-Examples <noreply@example.com>}"
    echo "Original-Maintainer: m5stack <m5stack@m5stack.com>"
    echo "Section: APPLaunch"
    echo "Priority: optional"
    echo "Homepage: ${PKG_HOMEPAGE:-https://www.m5stack.com}"
    echo "Packaged-Date: $PACKAGED_DATE"
    if [[ -n "$PKG_DEPENDS" ]]; then echo "Depends: $PKG_DEPENDS"; fi
    echo "Description: $PKG_DESC"
} > "$STAGE/DEBIAN/control"

# 7. Optional maintainer scripts
for hook in postinst prerm postrm preinst; do
    if [[ -f "$PKG_DIR/$hook" ]]; then
        cp "$PKG_DIR/$hook" "$STAGE/DEBIAN/$hook"
        chmod 0755 "$STAGE/DEBIAN/$hook"
    fi
done

# 8. Fix perms on staged binaries
find "$STAGE$INSTALL_PREFIX/bin" -type f -exec chmod 0755 {} \; 2>/dev/null || true
find "$STAGE$APP_INSTALL_DIR" -type f -name "*.sh" -exec chmod 0755 {} \; 2>/dev/null || true

# 9. Build .deb (prefer --root-owner-group so uid/gid are root regardless of builder)
OUT_FILE="$OUTPUT_DIR/${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}_${ARCH}.deb"
DPKG_ARGS=(-b "$STAGE" "$OUT_FILE")
if dpkg-deb --help 2>/dev/null | grep -q -- '--root-owner-group'; then
    DPKG_ARGS=(--root-owner-group "${DPKG_ARGS[@]}")
fi
dpkg-deb "${DPKG_ARGS[@]}"

echo "-- built: $OUT_FILE"
dpkg-deb --info "$OUT_FILE"
