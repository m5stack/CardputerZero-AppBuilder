#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 reader.py  "$STAGE$APP_INSTALL_DIR/reader.py"
install -D -m 0644 sample.md  "$STAGE$APP_INSTALL_DIR/sample.md"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
FILE="${1:-/usr/share/APPLaunch/apps/demo-mdread/sample.md}"
exec sudo -E python3 /usr/share/APPLaunch/apps/demo-mdread/reader.py "$FILE"
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/demo-mdread"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/demo-mdread"
