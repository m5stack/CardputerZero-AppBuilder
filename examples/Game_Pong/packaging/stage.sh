#!/usr/bin/env bash
set -euo pipefail
install -D -m 0755 pong "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
