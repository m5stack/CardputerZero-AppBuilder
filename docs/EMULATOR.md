# Emulator Usage

The `czdev` CLI includes a desktop emulator that renders the CardputerZero 320x170 LCD inside a keyboard skin using SDL2. You can run built-in apps or your own projects without a physical device.

## Prerequisites

See [QUICKSTART.md](QUICKSTART.md) for toolchain installation (cmake, SDL2, Rust).

## Running NC2000

NC2000 is a classic handheld PDA emulator (文曲星) ported to CardputerZero.

```bash
cd CardputerZero-AppBuilder
cargo run -p czdev --release -- run apps/nc2000
```

This will:
1. Configure and build the NC2000 shared library via CMake.
2. Stage `libnc2000.dylib` (macOS) or `libnc2000.so` (Linux) into the emulator's app directory.
3. Launch the emulator window at 640x420 (2x scaled).

The NC2000 home screen shows the classic PDA icons (address book, calculator, schedule, etc.). Use keyboard keys to navigate.

## Running APPLauncher

APPLauncher is the CardputerZero home screen / app launcher with a carousel UI.

```bash
cd CardputerZero-AppBuilder
cargo run -p czdev --release -- run apps/applaunch/
```

This launches the launcher interface showing installed apps (STORE, CLI, CLAW, etc.) in a horizontally scrollable carousel. Use left/right arrow keys to navigate between apps and Enter/OK to launch.

## Running Examples

```bash
cargo run -p czdev --release -- run examples/hello_cz
cargo run -p czdev --release -- run examples/key_echo
```

## Keyboard Mapping

| PC Key | CardputerZero |
|--------|---------------|
| A-Z, 0-9 | Direct mapping |
| F1-F12 | Function keys |
| Escape | ESC / HOME |
| Enter | OK |
| Arrow keys | Navigation |
| Shift | Aa (Caps) |
| Ctrl | ctrl |
| Alt | alt |
| Tab | tab / NEXT |

## Troubleshooting

- **Window doesn't appear** — ensure SDL2 is installed (`czdev doctor` checks this).
- **`inotify_init1 failed`** — benign warning on macOS, can be ignored.
- **`[Error] (0.000, +0)` lines** — LVGL debug output during init, harmless.
- **Build fails with missing NAND image** — NC2000 requires its ROM assets in the app directory; these are included in the `apps/nc2000` submodule.
