# Dev on macOS — simulate the 320×170 screen without flashing

Developing directly on the CM0 is slow (ssh + dpkg + restart APPLaunch). This
directory gives you three ways to iterate on your host laptop first, then flash
only when the UI looks right.

All scripts are idempotent and self-contained. They cd into the repo root
(`$REPO_ROOT`) so you can run them from anywhere.

## 1. SDL2 examples — native run on macOS

macOS has SDL2 via Homebrew, so `SDL2_HelloWorld` / `SDL2_Game` build and run
natively in a real 320×170 window. No framebuffer needed.

```bash
brew install sdl2 sdl2_ttf sdl2_mixer
./scripts/dev-on-mac/run-sdl.sh SDL2_HelloWorld
```

The script patches the DejaVuSans path to one that exists on macOS
(`/System/Library/Fonts/Supplemental/Arial.ttf` fallback) at runtime via env
var, without modifying the source.

## 2. LVGL / Slint — fbsim shim (Linux only)

LVGL's built-in simulator uses SDL2 too; these will be runnable on Mac later
but `run-sdl.sh` does not cover them yet. See TODO in that script.

## 3. Everything else — Linux-in-Docker with a virtual 320×170 framebuffer

For Framebuffer, LVGL, Rust/embedded-graphics, Python+Pillow, Qt linuxfb, PyQt,
Tkinter — we run the example inside an ARM64 Linux container that has a
virtual 320×170 framebuffer via `Xvfb` + `ffmpeg -f fbdev` trick, and mirror
the buffer to a GUI window on your Mac via `x11grab`-equivalent to an XQuartz
display.

```bash
# one-time
./scripts/dev-on-mac/setup-docker.sh

# pick any example that has packaging/
./scripts/dev-on-mac/run-in-docker.sh FrameBuffer_HelloWorld
./scripts/dev-on-mac/run-in-docker.sh Python_FrameBuffer_HelloWorld
./scripts/dev-on-mac/run-in-docker.sh LVGL_HelloWorld
```

The container:
- Is `ubuntu:24.04` (aarch64 — matches CM0 arch exactly on Apple Silicon).
- Mounts the repo read-write at `/work`.
- Runs `Xvfb :99 -screen 0 320x170x16` and pipes that to a fake `/dev/fb0` via
  a small mmap bridge (`xvfb-to-fb.py`).
- Starts XQuartz display forwarding so the Mac sees a live mirror.
- Runs the example's `packaging/build.sh` then launches the binary with
  `DISPLAY=:99`.

Performance is **not** representative of the real device (your Mac is 100×
faster than CM0), but layout and UX verify fine.

## 4. Quick "does my .deb install?" smoke test

```bash
./scripts/dev-on-mac/smoke-deb.sh FrameBuffer_HelloWorld
```

Builds the `.deb`, spins up the ubuntu aarch64 container, `dpkg -i`s it,
verifies `/usr/share/APPLaunch/bin/<pkg>` is executable, and dumps the rendered
`.desktop` file. Fast feedback that packaging is sane without touching the
device.

---

## Files

| File | Purpose |
| --- | --- |
| `run-sdl.sh` | build + run any SDL2 example natively on macOS |
| `setup-docker.sh` | first-time XQuartz + docker prep |
| `run-in-docker.sh` | run an example inside ubuntu aarch64, UI mirrored to XQuartz |
| `smoke-deb.sh` | `pack-deb.sh` + `dpkg -i` inside container; sanity only |
| `Dockerfile.ubuntu` | base image for 2 and 3 |
| `entrypoint.sh` | starts Xvfb + runs the example |

## Known limitations

- **No real /dev/fb0**: the container exposes a regular file + mmap helper
  pretending to be fb0. Anything that uses `FBIOGET_VSCREENINFO` with the
  actual Linux ioctl will fail — our pure-C Framebuffer examples do, so they
  only run under 3.
- **Keyboard**: Xvfb doesn't forward your Mac keystrokes by default;
  use `xdotool` inside the container (installed by Dockerfile) to simulate
  keys, or open the XQuartz window and type when it's focused.
- **Audio**: not wired up on macOS; `aplay`/SDL2_mixer are stubbed to no-op.

These limitations are fine because *the point of dev-on-mac is to iterate on
UI layout/logic without touching the device*. Once you're happy, `scripts/deploy.sh`
pushes the real `.deb`.
