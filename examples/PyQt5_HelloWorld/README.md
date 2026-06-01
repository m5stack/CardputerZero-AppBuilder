# PyQt5 HelloWorld (Python)

Python/PyQt5 equivalent of `Qt_HelloWorld`, rendering directly to /dev/fb0 on M5 CardputerZero (CM Zero, aarch64, 320x170).

与 `Qt_HelloWorld` 等效的 Python 版本，使用 PyQt5 通过 linuxfb 直接渲染到 /dev/fb0。

## Install / 安装

Preferred (system package, avoids building wheels on CM0):

```sh
sudo apt update
sudo apt install python3-pyqt5
```

pip fallback (heavy; PyQt5 wheels or apt provide roughly 80 MB of libs on aarch64):

```sh
pip3 install -r requirements.txt
```

## Run / 运行

```sh
./run.sh
```

Or manually:

```sh
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0,size=320x170
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/event0
sudo -E python3 hello.py
```

SSH deploy target: `pi@192.168.50.150`.

## Notes / 说明

- CM0 性能较弱，PyQt5 启动约需数秒（加载 ~80MB 库），首帧后交互可用。
- 320x170 空间紧张，字体 8-14 pt。
- CM0 无 GPU，固定使用 `linuxfb`，避免 `eglfs`。
- `sudo` 用于访问 `/dev/fb0` 与 `/dev/input/event*`。
- ESC / Q 退出。

PyQt5 on CM0 is heavy (~80 MB of shared libs, multi-second cold start). Use `python3-pyqt5` from apt. Stick to linuxfb; CM0 has no GPU. Root needed for framebuffer + evdev.
