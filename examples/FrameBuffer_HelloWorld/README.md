# FrameBuffer_HelloWorld

最小依赖的 `/dev/fb0` + evdev 示例，目标设备 M5 CardputerZero（1.9" ST7789V，320x170）。
Minimal `/dev/fb0` + evdev Hello World for the M5 CardputerZero (1.9" ST7789V, 320x170).

## Features

- mmap `/dev/fb0`, read `FBIOGET_VSCREENINFO` / `FBIOGET_FSCREENINFO`.
- Supports 16bpp (RGB565) and 32bpp (XRGB8888) by branching on `bits_per_pixel`.
- Centered "Hello, CardputerZero!" with inline 8x8 bitmap font.
- 1px border.
- Scans `/dev/input/event*` for a keyboard; exits on ESC or `q`. Override via `INPUT_DEV=/dev/input/eventN`.

## Build

```sh
make
# or: gcc -O2 -Wall -Wextra -std=c11 main.c -o hello_fb
```

## Run (on device)

Needs write access to `/dev/fb0` and an input device. Run under tty (Ctrl+Alt+F1) or via sudo:

```sh
sudo ./hello_fb
```

If running from SSH without a free TTY, first switch VT:

```sh
sudo chvt 1
sudo ./hello_fb
```

## Deploy via SSH

```sh
make deploy
# equivalent to:
#   scp hello_fb pi@192.168.50.150:~/
#   ssh pi@192.168.50.150 ./hello_fb
```

## Notes

- The fb dimensions are always read from vinfo; the code only warns if it is not 320x170.
- Native aarch64 build on the device is the primary path. Cross compile from x86_64:
  ```sh
  aarch64-linux-gnu-gcc -O2 -Wall -std=c11 main.c -o hello_fb
  ```
