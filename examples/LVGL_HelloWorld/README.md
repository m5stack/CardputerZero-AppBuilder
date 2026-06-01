# LVGL HelloWorld (CardputerZero)

A minimal LVGL v9.2 example for the M5 CardputerZero (RP3A0 aarch64,
1.9" ST7789V 320x170 TFT on `/dev/fb0`, keyboard via evdev). No touch.

## Build / 编译

On the device (or an aarch64 Linux host):

```sh
mkdir build && cd build
cmake ..
make -j2
```

The first build fetches LVGL v9.2.2 via CMake `FetchContent`. On the CM0
(single Cortex-A53 @ 1 GHz) expect a few minutes.

首次构建会通过 CMake `FetchContent` 拉取 LVGL v9.2.2 源码；CM0 性能有限，
初次编译需要几分钟。

## Deploy / 部署

Cross-compile or build on-device, then copy the binary over SSH:

```sh
scp build/hello_lvgl pi@192.168.50.150:~/
ssh pi@192.168.50.150 './hello_lvgl'
```

Default SSH credentials: `pi` / `pi`.

## Controls / 操作

- TAB / arrow keys: move focus in the input group.
- ENTER: activate the focused button (increments the click counter).
- There is no touch screen; all interaction is via the keyboard.

该机型无触摸屏，仅支持键盘交互：TAB / 方向键切换焦点，ENTER 触发按键。
