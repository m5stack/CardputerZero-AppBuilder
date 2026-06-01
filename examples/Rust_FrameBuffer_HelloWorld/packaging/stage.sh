#!/usr/bin/env bash
set -euo pipefail
install -D -m 0755 target/release/rust_fb_hello "$STAGE$INSTALL_PREFIX/bin/$PKG_NAME"
