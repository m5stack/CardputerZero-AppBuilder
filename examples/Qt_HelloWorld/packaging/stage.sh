#!/usr/bin/env bash
set -euo pipefail

install -D -m 0755 build/hello_qt "$STAGE$APP_INSTALL_DIR/hello_qt"

cat >"$STAGE$INSTALL_PREFIX/bin/$PKG_NAME" <<'EOF'
#!/bin/sh
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-linuxfb:fb=/dev/fb0,size=320x170}
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=${QT_QPA_EVDEV_KEYBOARD_PARAMETERS:-/dev/input/event0}
exec /usr/share/APPLaunch/apps/qt-hello/hello_qt "$@"
EOF
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
