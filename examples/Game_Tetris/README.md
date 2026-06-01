# Game_Tetris

CardputerZero 320x170 framebuffer Tetris in pure C11.

## Controls / 控制

| Key | Action / 动作 |
| --- | --- |
| LEFT / A  | Move left / 左移 |
| RIGHT / D | Move right / 右移 |
| DOWN / S  | Soft drop / 软降 |
| UP / W / ENTER | Rotate CW / 顺时针旋转 |
| SPACE     | Hard drop / 硬降 |
| R         | Restart (after game over) / 重开 |
| ESC / Q   | Quit / 退出 |

## Build / 构建

```sh
make
```

Produces the binary `tetris`.

## Run / 运行

On the device with `/dev/fb0` and an evdev keyboard:

```sh
./tetris
```

Override the input device if auto-detection picks the wrong one:

```sh
INPUT_DEV=/dev/input/event1 ./tetris
```

## Deploy / 部署

Packaged as the Debian package `game-tetris`:

```sh
./scripts/pack-deb.sh Game_Tetris
scp dist/game-tetris_*_arm64.deb pi@<device>:/tmp/
ssh pi@<device> 'sudo dpkg -i /tmp/game-tetris_*_arm64.deb \
    && sudo systemctl restart APPLaunch.service'
```

## Notes

- Board is 10 x 17 cells of 8 px each, centered with HUD on the right.
- Standard 7 tetrominos, simple CW rotation with small wall kicks.
- Gravity speeds up every 10 lines cleared.
- 60 Hz loop via `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`.
- No external libs. Self-contained `fb.[ch]`, `input.[ch]`, `font8x8.h`.
