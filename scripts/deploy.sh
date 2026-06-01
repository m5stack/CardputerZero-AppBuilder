#!/usr/bin/env bash
#
# Usage:
#   scripts/deploy.sh <example-dir-or-deb> [options]
#
# If the arg is an example directory, this script first runs pack-deb.sh on it.
# If the arg is a .deb file, it is uploaded as-is.
#
# Options:
#   --host <user@ip>     default: pi@192.168.50.150 (env DEVICE_HOST)
#   --remote-dir <dir>   staging dir on device, default: /tmp
#   --no-restart         don't restart APPLaunch.service after install
#   --uninstall          remote dpkg -r <pkg> (doesn't need a .deb — just --uninstall <dir-or-pkg>)
#   --skip-build         pass through to pack-deb.sh
#   --dry-run            print commands, do not execute
#
# Env:
#   DEVICE_HOST          default remote host
#   DEVICE_SSH_OPTS      extra ssh/scp options
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

HOST="${DEVICE_HOST:-pi@192.168.50.150}"
REMOTE_DIR="/tmp"
RESTART=1
UNINSTALL=0
SKIP_BUILD=""
DRY_RUN=0
TARGET=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host) HOST="$2"; shift 2 ;;
        --remote-dir) REMOTE_DIR="$2"; shift 2 ;;
        --no-restart) RESTART=0; shift ;;
        --uninstall) UNINSTALL=1; shift ;;
        --skip-build) SKIP_BUILD="--skip-build"; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        -*) echo "unknown flag: $1" >&2; exit 2 ;;
        *)
            if [[ -z "$TARGET" ]]; then TARGET="$1"; else
                echo "unexpected arg: $1" >&2; exit 2
            fi
            shift
            ;;
    esac
done

if [[ -z "$TARGET" ]]; then
    echo "usage: $0 <example-dir-or-deb> [options]" >&2
    exit 2
fi

# shellcheck disable=SC2206
SSH_OPTS=(${DEVICE_SSH_OPTS:-})

run() {
    if [[ $DRY_RUN -eq 1 ]]; then
        printf '+ %s\n' "$*"
    else
        printf '+ %s\n' "$*" >&2
        "$@"
    fi
}

# ---- uninstall path ----
if [[ $UNINSTALL -eq 1 ]]; then
    PKG=""
    if [[ -f "$TARGET" && "$TARGET" == *.deb ]]; then
        PKG="$(dpkg-deb -f "$TARGET" Package 2>/dev/null || true)"
    elif [[ -d "$TARGET" && -f "$TARGET/packaging/meta.env" ]]; then
        # shellcheck disable=SC1091
        PKG="$( (source "$TARGET/packaging/meta.env"; echo "$PKG_NAME") )"
    else
        PKG="$TARGET"  # treat as literal package name
    fi
    [[ -n "$PKG" ]] || { echo "could not determine package name" >&2; exit 1; }
    echo "uninstalling $PKG on $HOST"
    run ssh "${SSH_OPTS[@]}" "$HOST" "sudo dpkg -r '$PKG' && sudo systemctl restart APPLaunch.service || true"
    exit 0
fi

# ---- resolve the .deb to upload ----
DEB=""
if [[ -f "$TARGET" && "$TARGET" == *.deb ]]; then
    DEB="$TARGET"
elif [[ -d "$TARGET" ]]; then
    echo "building .deb from $TARGET"
    run "$SCRIPT_DIR/pack-deb.sh" "$TARGET" $SKIP_BUILD --output-dir "$REPO_ROOT/dist"
    # shellcheck disable=SC1091
    source "$TARGET/packaging/meta.env"
    DEB="$REPO_ROOT/dist/${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION:-m5stack1}_${PKG_ARCH:-arm64}.deb"
    [[ -f "$DEB" ]] || { echo "expected deb not found: $DEB" >&2; exit 1; }
else
    echo "error: $TARGET is neither a .deb nor an example directory" >&2
    exit 1
fi

BASENAME="$(basename "$DEB")"
REMOTE_PATH="$REMOTE_DIR/$BASENAME"

echo "deploying $DEB -> $HOST:$REMOTE_PATH"

run scp "${SSH_OPTS[@]}" "$DEB" "$HOST:$REMOTE_PATH"

if [[ $RESTART -eq 1 ]]; then
    run ssh "${SSH_OPTS[@]}" "$HOST" \
        "sudo dpkg -i '$REMOTE_PATH' && sudo systemctl restart APPLaunch.service"
else
    run ssh "${SSH_OPTS[@]}" "$HOST" "sudo dpkg -i '$REMOTE_PATH'"
fi

echo "done. check status on device with: sudo journalctl -u APPLaunch.service -n 50"
