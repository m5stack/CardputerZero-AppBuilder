#!/usr/bin/env bash
set -euo pipefail
install -D -m 0755 hello_fb "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
