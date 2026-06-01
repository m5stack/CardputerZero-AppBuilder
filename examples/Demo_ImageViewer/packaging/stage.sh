#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 viewer.py "$STAGE$APP_INSTALL_DIR/viewer.py"
install -d -m 0755 "$STAGE$APP_INSTALL_DIR/samples"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
# Run as the invoking user. The 'video' group grants /dev/fb0 access and
# the postinst adds pi to it; other fb demos (Clock, Matrix, 2048, Snake)
# follow the same pattern. Avoid sudo -- APPLaunch fork+exec has no TTY.
#
# Log to /tmp/demo-imgview.log so silent APPLaunch failures can be
# diagnosed after the fact (APPLaunch captures no stdout/stderr itself).
LOG=/tmp/demo-imgview.log
DIR="${1:-/usr/share/APPLaunch/apps/demo-imgview/samples}"
: >>"$LOG" 2>/dev/null || LOG=/dev/null
{
    echo "--- demo-imgview launch $(date -Iseconds) ---"
    echo "user=$(id -un) uid=$(id -u) groups=$(id -Gn)"
    echo "dir=$DIR"
} >>"$LOG" 2>&1
exec python3 /usr/share/APPLaunch/apps/demo-imgview/viewer.py "$DIR" >>"$LOG" 2>&1
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/demo-imgview"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/demo-imgview"
