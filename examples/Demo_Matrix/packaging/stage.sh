#!/usr/bin/env bash
set -euo pipefail

# Install the raw binary as <pkg>-bin, then drop a /bin/sh wrapper at
# the name APPLaunch invokes. The wrapper redirects stdout+stderr to
# /tmp/<pkg>.log so launch-time crashes leave a trace on the device.
install -D -m 0755 matrix "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME-bin"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
LOG=/tmp/demo-matrix.log
exec /usr/share/APPLaunch/bin/demo-matrix-bin "$@" >>"$LOG" 2>&1
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
rm -f "$tmp"
