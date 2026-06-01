#!/usr/bin/env bash
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-linuxfb:fb=/dev/fb0,size=320x170}
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=${QT_QPA_EVDEV_KEYBOARD_PARAMETERS:-/dev/input/event0}
exec python3 hello.py "$@"
