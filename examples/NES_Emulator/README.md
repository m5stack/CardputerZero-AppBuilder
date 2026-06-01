# NES_Emulator — NES on CardputerZero (SDL2, 320×170)

A small NES emulator for M5 CardputerZero that targets the 320×170 ST7789V
display directly through SDL2's software renderer. The core is **LaiNES**
(C++11, GPL-3.0), fetched at build time via CMake `FetchContent` — no emulator
source is vendored here.

## 目的 / Purpose

- 在 CardputerZero 上提供一个可用的 NES 模拟器。
- 演示如何把 256×240 的游戏画面适配到 320×170 的 4:3 非典型屏幕。
- A reference for 2D framebuffer scaling decisions when the host display has
  an unusual aspect ratio.

## Core choice / 选型

**Primary: LaiNES** (<https://github.com/AndreaOrru/LaiNES>).
Pinned commit: `6b057f9b08a27bce00fa13ac9f77e64f99b65be5` (default branch HEAD
at write time — edit `LAINES_GIT_TAG` in `CMakeLists.txt` to repin).

Rationale:

- Already ships an SDL2 frontend and works with `SDL_RENDERER_SOFTWARE` — a
  perfect match for the CM0 (no GPU, ALSA audio, evdev input).
- Clean C++11, compiles with GCC on aarch64.
- Cycle-accurate-ish PPU + Blargg APU → good audio quality on default mapper 0/1/2/4 games.

Alternate considered: **InfoNES** (portable C, permissive). Smaller and faster
but the audio/mapper support is noticeably thinner, and we'd need to write the
SDL glue from scratch. If LaiNES performance on CM0 is insufficient for a
given game, InfoNES is a viable drop-in replacement — swap the `FetchContent`
block in `CMakeLists.txt`.

## Viewport modes — 256×240 → 320×170

Selected via environment variable **`NES_VIEW_MODE`** (default
`scaletoheight`). Toggle at runtime with **V**.

### `scaletoheight` (default, aka `fit`)

Preserve NES 4:3 aspect, scale so `height = 170`. Width becomes
`170 × 256 / 240 ≈ 181 px`. Left/right bars are letterboxed black.

```
+--------+-----------------+--------+
|  69px  |                 |  70px  |
| black  |   181 × 170     | black  |
|  bar   |   NES picture   |  bar   |
|        |   (4:3, nearest)|        |
+--------+-----------------+--------+
0       69                 250     320
```

### `cropheight` (fills the screen)

Keep a 1:1 horizontal scaling logic but actually stretch 256 → 320 (×1.25) and
drop the top/bottom 35 rows of the 240-row NES image. Heights line up
perfectly; the HUD row that sits in the very top of most NES games may clip.

```
+----------------------------------+
|                                  |
|         320 × 170 stretched      |   source: rows 35..204
|        (horizontal 1.25× NN)     |   top 35 + bottom 35 hidden
|                                  |
+----------------------------------+
0                                320
```

### `fit`

Alias for `scaletoheight` (since 240>170 the vertical constraint wins either
way).

## Controls

| Key            | Action          |
| -------------- | --------------- |
| Arrow keys     | D-Pad           |
| K (or Z)       | A button        |
| J (or X)       | B button        |
| Enter          | Start           |
| Right Shift / Backspace | Select |
| P              | Pause / Resume  |
| R              | Reset console   |
| V              | Cycle view mode |
| ESC            | Quit            |

Keyboard reads go through `SDL_GetKeyboardState`, which on Linux maps to
`/dev/input/event*` via SDL's evdev backend — matches CardputerZero's
keyboard.

## Dependencies / 依赖

Build-time (CI installs via `packaging/ci-deps.sh`):

```
cmake build-essential pkg-config git libsdl2-dev libsdl2-ttf-dev
```

Runtime (declared in `meta.env` → `PKG_DEPENDS`):

```
libsdl2-2.0-0, libsdl2-ttf-2.0-0
```

ALSA is the default audio path on the device — no extra config needed.

## Build / 构建

```sh
cd NES_Emulator
./packaging/ci-deps.sh     # first time only, installs build deps
./packaging/build.sh       # runs cmake -B build && cmake --build build
```

The first build pulls down LaiNES via `FetchContent` into `build/_deps/`.
That fetch is the slowest step.

## Run / 运行

```sh
# natively on the device or laptop with SDL2 installed
NES_ROM=/path/to/your-game.nes ./build/nes-emulator

# or pass as argv
./build/nes-emulator /path/to/your-game.nes

# choose a viewport mode
NES_VIEW_MODE=cropheight ./build/nes-emulator rom.nes

# on the device under KMSDRM (no X)
SDL_VIDEODRIVER=KMSDRM ./build/nes-emulator rom.nes
```

If neither `$NES_ROM` nor `argv[1]` is a valid file, the binary still starts
and shows a **"No ROM loaded — set NES_ROM=..."** splash instead of crashing.
ESC quits.

## ROM placement / ROM 放置

**Do not commit ROM files** — they're copyrighted. Put legally-obtained
`.nes` files in one of:

- `NES_Emulator/roms/*.nes` during local development.
- `/usr/share/APPLaunch/apps/nes-emulator/roms/*.nes` on the device.
- Any path via `NES_ROM=/abs/path/to/rom.nes`.

The launcher wrapper (installed at `/usr/share/APPLaunch/bin/nes-emulator`)
picks the first `*.nes` in the default directory when `NES_ROM` is unset.

Copy a ROM to the device:

```sh
scp my-game.nes pi@192.168.50.150:/usr/share/APPLaunch/apps/nes-emulator/roms/
```

## Deploy / 部署

```sh
# from repo root
./scripts/pack-deb.sh NES_Emulator
scp dist/nes-emulator_0.1-m5stack1_arm64.deb pi@192.168.50.150:/tmp/
ssh pi@192.168.50.150 'sudo dpkg -i /tmp/nes-emulator_0.1-m5stack1_arm64.deb \
    && sudo systemctl restart APPLaunch.service'
```

## Performance notes

- CardputerZero is an RP3A0 @ 1 GHz (Cortex-A53 quad). On a single core,
  LaiNES runs mapper 0/1/2/3 games (SMB, Contra, Mega Man 1/2, Castlevania)
  comfortably at 60 fps with the software renderer + `RGB565` streaming
  texture + nearest-neighbor scaling.
- Heavier mappers (MMC5, VRC6/VRC7, FME7) may occasionally dip below 60 fps.
  If that happens:
  - Try `NES_VIEW_MODE=cropheight` (fewer pixels transformed; wider=more but
    no letterbox bars to fill).
  - Reduce audio sample rate in LaiNES' `apu.cpp` if adventurous.
- The frontend is borderless, single-window, and uses
  `SDL_RENDERER_SOFTWARE` — no Mesa/GL dependency on the device.

## License

- Our frontend (`src/`, `CMakeLists.txt`, `packaging/`): MIT, same as the rest
  of this examples repo.
- **LaiNES (fetched at build time): GPL-3.0.** Because we link against it,
  any redistribution of the final binary falls under GPL-3.0. The sources we
  add to the resulting program are MIT-compatible, so the combined binary is
  distributable under GPL-3.0. No NES BIOS or ROMs are included.
