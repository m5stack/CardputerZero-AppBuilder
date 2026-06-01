#!/usr/bin/env bash
set -euo pipefail

install -D -m 0644 hello_tk.py "$STAGE$APP_INSTALL_DIR/hello_tk.py"
install -D -m 0644 fb_blit.py  "$STAGE$APP_INSTALL_DIR/fb_blit.py"
install -D -m 0755 run_xvfb.sh "$STAGE$APP_INSTALL_DIR/run_xvfb.sh"

tmp=$(mktemp)
cat >"$tmp" <<'EOF'
#!/bin/sh
cd /usr/share/APPLaunch/apps/py-tk-hello
# Do NOT exec run_xvfb.sh: we want to regain control after it exits so we
# can blank /dev/fb0. Otherwise APPLaunch's home screen stays hidden
# behind the last Tk frame until the user presses an arrow key.
# Avoid `set -e` so a non-zero RC from the app still reaches the blank step.
sh ./run_xvfb.sh "$@"
RC=$?
# 320x170x2 = 108800 bytes; 4096*27 = 110592 covers that with one extra block.
dd if=/dev/zero of=/dev/fb0 bs=4096 count=27 2>/dev/null || true
exit $RC
EOF
install -D -m 0755 "$tmp" "$STAGE$INSTALL_PREFIX/bin/py-tk-hello"
rm -f "$tmp"
chmod 0755 "$STAGE$INSTALL_PREFIX/bin/py-tk-hello"
