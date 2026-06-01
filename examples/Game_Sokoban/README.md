# Game_Sokoban

CardputerZero 320x170 framebuffer Sokoban in pure C11.

## Controls / 控制

| Key | Action / 动作 |
| --- | --- |
| Arrows / WASD | Move / 移动 |
| U            | Undo last move / 撤销 |
| R            | Restart level / 重试本关 |
| N            | Next level (after clear) / 下一关 |
| P            | Previous level / 上一关 |
| ESC / Q      | Quit / 退出 |

3 built-in levels on a 10 x 10 tile grid, 14 px tiles, HUD on the right.

## Build / 构建

```sh
make
```

Produces the binary `sokoban`.

## Run / 运行

```sh
./sokoban
```

Override input device:

```sh
INPUT_DEV=/dev/input/event1 ./sokoban
```

## Deploy / 部署

Packaged as the Debian package `game-sokoban`:

```sh
./scripts/pack-deb.sh Game_Sokoban
scp dist/game-sokoban_*_arm64.deb pi@<device>:/tmp/
ssh pi@<device> 'sudo dpkg -i /tmp/game-sokoban_*_arm64.deb \
    && sudo systemctl restart APPLaunch.service'
```

## Notes

- Levels use ASCII: `#` wall, `.` target, `$` box, `@` player, `*` box-on-target, `+` player-on-target, ` ` floor.
- Undo ring keeps up to 256 steps.
- On clear: "LEVEL CLEAR" flashes for ~1 s then auto-advances.
- 30 Hz loop via `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`.
- Self-contained `fb.[ch]`, `input.[ch]`, `font8x8.h`. No external libs.
