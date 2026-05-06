# APPLaunch (prebuilt by emulator)

This directory contains **only a manifest** — no sources. APPLaunch lives in
`M5CardputerZero-UserDemo/projects/APPLaunch/` and is built by the emulator's
own CMake as `libAPPLaunch.dylib` (on mac) / `libAPPLaunch.so` (Linux). We
deliberately do not vendor, copy, or otherwise modify the UserDemo repository.

## How it works

When `czdev run apps/applaunch` runs:

1. If the emulator binary isn't built yet, czdev builds it. That build pulls
   in the UserDemo submodule inside the emulator and produces
   `emulator/build/apps/libAPPLaunch.dylib`.
2. The `prebuilt-emulator-app` runtime tells czdev to skip cmake and use that
   file directly.
3. czdev launches the emulator with APPLaunch loaded.

To get updates from UserDemo, run `git submodule update --remote` inside
`emulator/vendor/M5CardputerZero-UserDemo/` and rebuild the emulator.
