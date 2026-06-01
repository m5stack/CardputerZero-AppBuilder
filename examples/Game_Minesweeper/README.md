# Game_Minesweeper

CardputerZero 320x170 framebuffer Minesweeper in pure C11.

## Controls / 控制

| Key | Action / 动作 |
| --- | --- |
| Arrows / WASD | Move cursor / 移动光标 |
| SPACE / ENTER | Reveal cell / 翻开 |
| F            | Toggle flag / 插/拔旗 |
| R            | Restart board / 重开 |
| ESC / Q      | Quit / 退出 |

Board: 20 x 14 cells, 16 mines. First click is always safe.

## Build / 构建

```sh
make
```

Produces the binary `minesweeper`.

## Run / 运行

```sh
./minesweeper
```

Override input device if needed:

```sh
INPUT_DEV=/dev/input/event1 ./minesweeper
```

## Deploy / 部署

Packaged as the Debian package `game-minesweeper`:

```sh
./scripts/pack-deb.sh Game_Minesweeper
scp dist/game-minesweeper_*_arm64.deb pi@<device>:/tmp/
ssh pi@<device> 'sudo dpkg -i /tmp/game-minesweeper_*_arm64.deb \
    && sudo systemctl restart APPLaunch.service'
```

## Notes

- Only dirty cells are repainted each frame.
- Face shows :) playing, :D won, :( lost.
- 30 Hz loop via `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`.
- Self-contained `fb.[ch]`, `input.[ch]`, `font8x8.h`. No external libs.
