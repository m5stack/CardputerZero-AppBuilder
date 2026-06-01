#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 dashboard.py "$STAGE$APP_INSTALL_DIR/dashboard.py"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
exec python3 /usr/share/APPLaunch/apps/demo-sysdash/dashboard.py "$@"
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/demo-sysdash"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/demo-sysdash"
