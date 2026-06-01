# ROMs

Put your **legally-obtained** `.nes` files in this directory, or point the
launcher at one via an env var:

```sh
export NES_ROM=/path/to/your-game.nes
nes-emulator
```

If `NES_ROM` is unset, the launcher picks the first `*.nes` it finds under
`$NES_ROM_DIR` (default `/usr/share/APPLaunch/apps/nes-emulator/roms/` on the
device, `NES_Emulator/roms/` during local dev).

No ROM files are committed to this repo.
