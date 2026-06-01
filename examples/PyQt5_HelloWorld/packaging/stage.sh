#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 hello.py "$STAGE$APP_INSTALL_DIR/hello.py"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-linuxfb:fb=/dev/fb0,size=320x170}
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=${QT_QPA_EVDEV_KEYBOARD_PARAMETERS:-/dev/input/event0}
# Do NOT exec the python process: we need to regain control after it exits
# so we can blank /dev/fb0. APPLaunch's home screen only repaints when the
# user presses an arrow key, so without this the last Qt frame lingers.
python3 /usr/share/APPLaunch/apps/py-qt-hello/hello.py "$@"
RC=$?
# 320x170x2 = 108800 bytes; 4096*27 = 110592 covers that with one extra block.
dd if=/dev/zero of=/dev/fb0 bs=4096 count=27 2>/dev/null || true
exit $RC
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/py-qt-hello"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/py-qt-hello"
