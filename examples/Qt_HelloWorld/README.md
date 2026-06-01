# Qt HelloWorld (C++ / Qt5 Widgets)

Minimal Qt5 Widgets demo for M5 CardputerZero (CM Zero, aarch64 Linux, 320x170 /dev/fb0).

基于 Qt5 Widgets 的最小示例，直接渲染到 linuxfb（CM0 无 GPU，避免使用 eglfs）。

## Install / 安装

```sh
sudo apt update
sudo apt install qtbase5-dev libqt5widgets5
```

## Build / 构建

Option A (qmake):

```sh
qmake && make -j2
```

Option B (cmake):

```sh
cmake -B build && cmake --build build -j2
```

## Run / 运行

```sh
sudo ./hello_qt -platform linuxfb:fb=/dev/fb0,size=320x170
```

Or via env:

```sh
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0,size=320x170
export QT_QPA_FB_NO_LIBINPUT=1
export QT_QPA_EVDEV_KEYBOARD_PARAMETERS=/dev/input/event0
sudo -E ./hello_qt
```

SSH deploy target: `pi@192.168.50.150`.

## Notes / 说明

- 320x170 非常紧凑，字体使用 8-14 pt；Qt Widgets 元素默认偏大。
- CM0 无 GPU，务必使用 `linuxfb`，不要使用 `eglfs`。
- 需要 `sudo` 以访问 `/dev/fb0` 与 `/dev/input/event*`。
- ESC / Q 退出。

Qt Widgets at 320x170 is tight; keep fonts small (8-14 pt). CM0 has no GPU - stick to linuxfb. Root access required for framebuffer and evdev.
