#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 fb_hello.py "$STAGE$APP_INSTALL_DIR/fb_hello.py"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
exec python3 /usr/share/APPLaunch/apps/py-fb-hello/fb_hello.py "$@"
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/py-fb-hello"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/py-fb-hello"
