# SDL2_Game - Breakout

Small Brick Breaker for the M5 CardputerZero (CM Zero, RP3A0, 512 MB).
320x170 borderless window, software renderer, SDL2_ttf HUD, SDL2_mixer
sound effects. No OpenGL - CM0 has no GPU, everything is CPU.

## Controls

- Left / Right arrows - move paddle
- SPACE - launch ball
- R - restart
- ESC - quit

## Layout

- 5 rows x 8 columns of bricks (~38x8 px each)
- Lives (start 3) top-left, score top-right
- 60 Hz fixed timestep (accumulator + SDL_Delay)

## Build (on device)

```
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev \
                 fonts-dejavu-core sox
make
```

## Assets

The game loads `assets/bounce.wav` and `assets/break.wav`. Binary audio
is not checked in - generate them with `sox`. See
[`assets/README_ASSETS.md`](assets/README_ASSETS.md). The game still
runs if the files are missing.

## Run

```
# Preferred
SDL_VIDEODRIVER=KMSDRM ./breakout

# Legacy framebuffer
SDL_VIDEODRIVER=fbcon ./breakout

# Headless / CI
SDL_VIDEODRIVER=dummy ./breakout
```

## SSH

```
ssh pi@192.168.50.150
cd ~/CardputerZero-Examples/SDL2_Game
make
sudo SDL_VIDEODRIVER=KMSDRM ./breakout
```

`sudo` is usually needed for `/dev/dri`, `/dev/fb0`, and
`/dev/input/event*` unless the user is in the `video`, `render`, and
`input` groups.

## Files

- `main.c`   - SDL setup, event loop, 60 Hz ticker, audio dispatch
- `game.c/h` - paddle / ball / brick state and collision
- `render.c/h` - all drawing (rects + text)
- `assets/README_ASSETS.md` - how to generate the sound effects
