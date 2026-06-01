# FrameBuffer_Game — Snake

A tiny Snake game using only `/dev/fb0` + evdev. Targets the M5 CardputerZero
(1.9" ST7789V, 320x170). 10 fps fixed step via `clock_nanosleep(CLOCK_MONOTONIC)`.

贪吃蛇，直接写 `/dev/fb0`，从 `/dev/input/event*` 读键盘，10 fps 固定步进。

## Layout

- `main.c` — game loop, render, game state
- `fb.c` / `fb.h` — framebuffer open / pixel / fill / text
- `input.c` / `input.h` — evdev scan + non-blocking polling
- `font8x8.h` — inline 8x8 bitmap font
- `Makefile`

## Controls

| Key              | Action          |
| ---------------- | --------------- |
| Arrow keys / WASD| Turn snake      |
| SPACE / Enter    | Restart (on game over) |
| ESC / q          | Quit            |

## Build

```sh
make
```

## Run

```sh
sudo ./snake_fb
# or set an explicit keyboard device:
INPUT_DEV=/dev/input/event0 sudo -E ./snake_fb
```

## Deploy

```sh
make deploy
# scp snake_fb pi@192.168.50.150:~/
# ssh pi@192.168.50.150 ./snake_fb
```

## Optional: eat-food beep

Drop a small WAV at `assets/beep.wav` and it will play via `aplay` when the
snake eats food:

```sh
mkdir -p assets
cp /usr/share/sounds/alsa/Front_Center.wav assets/beep.wav
```

If the file does not exist, the game stays silent (no error).

## Notes

- Grid is 32x17 cells at 10 px each. Dimensions are still read from
  `FBIOGET_VSCREENINFO`; a warning is printed if the fb is not 320x170.
- Supports 16bpp (RGB565) and 32bpp framebuffers.
