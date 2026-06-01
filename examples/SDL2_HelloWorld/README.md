# SDL2_HelloWorld

Minimal SDL2 sample for the M5 CardputerZero (Raspberry Pi CM Zero, RP3A0, 512 MB).
Opens a 320x170 borderless window, renders "Hello CardputerZero" with SDL2_ttf
(DejaVuSans 14pt), shows a 1 Hz FPS counter, and plays a short WAV via
SDL2_mixer on any keypress.

## Target hardware

- 1.9" LCD, 320x170, RGB565 on `/dev/fb0`
- No GPU on CM0 - Mesa swrast only. We use `SDL_RENDERER_SOFTWARE`.
- Keyboard input arrives as evdev -> SDL_KEYDOWN.
- ESC or close window to quit.

## Build (on the device)

```
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev \
                 fonts-dejavu-core alsa-utils
make
```

Or directly:

```
gcc $(pkg-config --cflags sdl2 SDL2_ttf SDL2_mixer) main.c -o hello_sdl2 \
    $(pkg-config --libs sdl2 SDL2_ttf SDL2_mixer)
```

## Run

Pick a video driver appropriate for the installed SDL2 build. KMSDRM is
preferred on modern Raspberry Pi OS images; `fbcon` works on older builds.

```
# Preferred - direct to /dev/dri
SDL_VIDEODRIVER=KMSDRM ./hello_sdl2

# Legacy framebuffer path
SDL_VIDEODRIVER=fbcon ./hello_sdl2

# Headless (CI / SSH without display)
SDL_VIDEODRIVER=dummy ./hello_sdl2

# Offscreen for debug
SDL_VIDEODRIVER=offscreen ./hello_sdl2
```

## SSH usage

```
ssh pi@192.168.50.150
cd ~/CardputerZero-Examples/SDL2_HelloWorld
make
sudo SDL_VIDEODRIVER=KMSDRM ./hello_sdl2
```

`sudo` is typically required to access `/dev/dri` or `/dev/fb0` and
`/dev/input/event*` unless the `pi` user is in the `video`, `render`,
and `input` groups.

## Notes

- CM0 has no GPU - everything is CPU-rendered. Keep draw calls light.
- Audio uses ALSA via SDL2_mixer and plays
  `/usr/share/sounds/alsa/Front_Center.wav`. The sample guards against
  the file being missing.
