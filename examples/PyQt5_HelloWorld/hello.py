#!/usr/bin/env python3
import atexit
import os
import struct
import sys
import fcntl

from PyQt5.QtCore import Qt, QTimer, QDateTime
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout
from PyQt5.QtGui import QFont

FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602


def _blank_fb(path="/dev/fb0"):
    """Write zeros over the framebuffer so APPLaunch's resumed UI is visible
    immediately after we exit. We never hardcode resolution: read vinfo/finfo
    and zero exactly smem_len bytes."""
    try:
        fd = os.open(path, os.O_RDWR)
    except OSError as exc:
        print("[hello_pyqt5] fb open failed: {}".format(exc), file=sys.stderr)
        return
    try:
        vbuf = bytearray(160)
        fcntl.ioctl(fd, FBIOGET_VSCREENINFO, vbuf)
        xres, yres, _xv, _yv, _xo, _yo, bpp, _gray = struct.unpack_from("8I", vbuf, 0)
        fbuf = bytearray(struct.calcsize("16sL" + "I" * 6 + "HHHI"))
        fcntl.ioctl(fd, FBIOGET_FSCREENINFO, fbuf)
        fields = struct.unpack("16sL" + "I" * 6 + "HHHI", bytes(fbuf))
        smem_len = fields[2]
        # Fall back to xres*yres*(bpp/8) if smem_len looks bogus.
        need = smem_len or (xres * yres * max(1, bpp // 8))
        os.lseek(fd, 0, os.SEEK_SET)
        zero = b"\x00" * 4096
        remaining = need
        while remaining > 0:
            n = os.write(fd, zero if remaining >= 4096 else zero[:remaining])
            if n <= 0:
                break
            remaining -= n
    except Exception as exc:
        print("[hello_pyqt5] fb blank failed: {}".format(exc), file=sys.stderr)
    finally:
        os.close(fd)


def ensure_platform():
    if "-platform" in sys.argv:
        return
    if any(k.startswith("QT_QPA_PLATFORM") for k in __import__("os").environ):
        return
    sys.argv += ["-platform", "linuxfb:fb=/dev/fb0,size=320x170"]


class HelloWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(320, 170)
        self.setWindowFlags(Qt.FramelessWindowHint)
        self.setStyleSheet("background-color: black;")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        title = QLabel("Hello CardputerZero", self)
        tf = QFont(title.font())
        tf.setPointSize(14)
        tf.setBold(True)
        title.setFont(tf)
        title.setStyleSheet("color: white;")
        title.setAlignment(Qt.AlignCenter)

        self.clock = QLabel(self)
        cf = QFont(self.clock.font())
        cf.setPointSize(10)
        self.clock.setFont(cf)
        self.clock.setStyleSheet("color: #00ff88;")
        self.clock.setAlignment(Qt.AlignCenter)

        hint = QLabel("ESC / Q to exit", self)
        hf = QFont(hint.font())
        hf.setPointSize(8)
        hint.setFont(hf)
        hint.setStyleSheet("color: #888;")
        hint.setAlignment(Qt.AlignCenter)

        layout.addStretch()
        layout.addWidget(title)
        layout.addWidget(self.clock)
        layout.addStretch()
        layout.addWidget(hint)

        timer = QTimer(self)
        timer.timeout.connect(self.tick)
        timer.start(1000)
        self.tick()

    def tick(self):
        self.clock.setText(QDateTime.currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))

    def keyPressEvent(self, e):
        if e.key() in (Qt.Key_Escape, Qt.Key_Q):
            QApplication.instance().quit()
            return
        super().keyPressEvent(e)


def main():
    print("[hello_pyqt5] launching on 320x170 linuxfb", file=sys.stderr)
    # Register BEFORE QApplication so it runs after Qt tears down.
    atexit.register(_blank_fb)
    ensure_platform()
    app = QApplication(sys.argv)
    w = HelloWindow()
    w.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
