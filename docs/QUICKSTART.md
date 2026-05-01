# Quickstart — desktop dev for CardputerZero apps

Get a 320×170 LVGL app running on your Mac or Linux machine in ~3 minutes —
no CardputerZero device required.

## 1. Prerequisites

Install the native toolchain.

**macOS:**
```bash
brew install cmake pkg-config sdl2 sdl2_image sdl2_mixer freetype
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libfreetype-dev
```

**Windows:** MSYS2 MINGW64 shell. See
[DESKTOP_DEV.md §4](DESKTOP_DEV.md#4-windows-lvgl--emulator--known-issues-and-plan)
for the Windows-specific work still in progress — the mac/Linux loop below is
what's supported end-to-end today.

You also need a recent Rust toolchain (for `czdev`). If you don't have one:
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

## 2. Clone with submodules

```bash
git clone --recursive git@github.com:m5stack/CardputerZero-AppBuilder.git
cd CardputerZero-AppBuilder
```

If you already cloned without `--recursive`:
```bash
git submodule update --init --recursive
```

## 3. Verify the environment

```bash
cargo run -p czdev --release -- doctor
```

You should see all required rows green. If anything is MISSING, the output
shows the exact install command for your OS.

## 4. Run the hello app

```bash
cargo run -p czdev --release -- run examples/hello_cz
```

On first run this will:

1. Build the emulator (once, cached in `emulator/build/`).
2. Build `examples/hello_cz` into `examples/hello_cz/.czdev/build/`.
3. Stage the resulting `libhello_cz.dylib` (or `.so`) into the emulator's
   `apps/` directory.
4. Launch the emulator with the app loaded via `dlopen`.

You should see a 320×170 LCD inside a keyboard skin, showing
`Hello, CardputerZero!`. Close the emulator window to exit.

## 5. Edit-run loop

```bash
cargo run -p czdev --release -- watch examples/hello_cz
```

The watcher polls `src/`, `include/`, `assets/`, `CMakeLists.txt` and
`app-builder.json`. Any change triggers a rebuild and relaunches the emulator.

## 6. Writing your own app

Copy `examples/hello_cz/` and edit `src/hello_cz.c`. The ABI is documented in
`sdk/include/cz_app.h`:

```c
#include <cz_app.h>

void app_main(lv_obj_t *parent) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "your UI here");
    lv_obj_center(label);
}

void app_event(int type, void *data) {
    (void)type; (void)data;
}
```

The `CMakeLists.txt` is three lines:

```cmake
cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sdk/cmake")
include(CZApp)
cz_add_lvgl_app(my_app SOURCES src/my_app.c)
```

And the manifest (see `docs/APP_BUILDER_JSON.md`):

```json
{
  "package_name": "my_app",
  "bin_name": "my_app",
  "app_name": "My App",
  "runtime": "lvgl-dlopen",
  "lvgl_version": "9.5"
}
```

## 7. Shipping to a real device

Building the aarch64 `.deb` stays on CI — trigger the existing
`build-deb.yml` workflow (see the repo README). Then push to the device:

```bash
cargo run -p czdev --release -- deploy \
    --host pi@192.168.50.150 \
    --deb path/to/my_app_arm64.deb
```

## Troubleshooting

- **`emulator submodule not checked out`** — you forgot `--recursive`. Fix:
  `git submodule update --init --recursive`.
- **LVGL link errors about unresolved symbols** — expected in the app
  library; they're resolved at `dlopen` time by the emulator. If the linker
  *fails* instead of warns, see `DESKTOP_DEV.md` for the per-platform link
  flags (`CZApp.cmake` handles these automatically).
- **`indev_read_cb is not registered` warnings in the log** — benign; the
  emulator falls back to a default keypad indev when the app doesn't install
  its own `lv_sdl_keyboard_create`.
- **macOS: `Library not loaded: @rpath/SDL2.framework/...`** — `brew install
  sdl2` puts the library in a non-framework path; re-run `czdev doctor` and
  install what it reports.
