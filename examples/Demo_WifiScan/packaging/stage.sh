#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 scan.py "$STAGE$APP_INSTALL_DIR/scan.py"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
exec sudo -E python3 /usr/share/APPLaunch/apps/demo-wifiscan/scan.py "$@"
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/demo-wifiscan"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/demo-wifiscan"
