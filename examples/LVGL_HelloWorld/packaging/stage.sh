#!/usr/bin/env bash
set -euo pipefail
install -D -m 0755 build/hello_lvgl "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
