#!/usr/bin/env bash
set -euo pipefail

install -D -m 0755 snake_fb "$STAGE$APP_INSTALL_DIR/snake_fb"

if [[ -d assets ]]; then
    mkdir -p "$STAGE$APP_INSTALL_DIR/assets"
    cp -r assets/. "$STAGE$APP_INSTALL_DIR/assets/"
fi

cat >"$STAGE$INSTALL_PREFIX/bin/$PKG_NAME" <<'EOF'
#!/bin/sh
cd /usr/share/APPLaunch/apps/framebuffer-snake
exec /usr/share/APPLaunch/apps/framebuffer-snake/snake_fb "$@"
EOF
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
