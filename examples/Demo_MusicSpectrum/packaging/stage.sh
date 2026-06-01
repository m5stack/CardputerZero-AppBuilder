#!/usr/bin/env bash
set -euo pipefail

# Install the raw binary as <pkg>-bin, then drop a /bin/sh wrapper at
# the name APPLaunch invokes. The wrapper redirects stdout+stderr to
# /tmp/demo-spectrum.log so ALSA enumeration diagnostics and any launch
# crash are recoverable (APPLaunch runs us with no TTY).
install -D -m 0755 spectrum "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME-bin"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
LOG=/tmp/demo-spectrum.log
exec /usr/share/APPLaunch/bin/demo-spectrum-bin "$@" >>"$LOG" 2>&1
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
rm -f "$tmp"
