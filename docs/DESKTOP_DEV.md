# Desktop Development for CardputerZero Apps

This document describes how the **desktop emulator** lets you develop 320×170
LVGL apps for M5 CardputerZero without having the physical device. It is the
contract between:

- App authors writing `.dylib` / `.so` / `.dll` modules
- The emulator (`cardputer-zero-emu`) that loads them
- The real device (APPLaunch on aarch64 Linux) that loads the same sources
  compiled natively

The goal: **one source tree → same `.dylib/.so/.dll` on desktop, same `.so`
inside the `.deb` on the device — zero per-platform `#ifdef` in app code.**

---

## 1. How LVGL is shared between emulator and app (all platforms)

The emulator hosts a single LVGL instance. Apps are **loaded into the same
process** via `dlopen` / `LoadLibrary` and call LVGL directly — they do **not**
link their own LVGL.

On macOS and Linux this works because:

- `emulator.exe` links LVGL with `-Wl,-force_load` (mac) / `--whole-archive`
  (Linux), so every LVGL symbol lives in the emulator's global symbol table.
- The app `.dylib/.so` is linked with `-undefined dynamic_lookup` (mac) /
  `-Wl,--unresolved-symbols=ignore-all` (Linux) — LVGL references are left
  unresolved at link time.
- `dlopen(RTLD_GLOBAL)` at runtime binds the app's LVGL references to the
  emulator's copy.

Windows needs a different shape (see §4) but the **result is the same**: one
LVGL instance, one display, one input group, one timer handler.

On the real device, APPLaunch does the same thing with `dlopen` against its own
LVGL — the app `.so` behaves identically.

### Rules for app authors

1. **Do not call `lv_init()` or `lv_display_create()`.** The host already did.
2. **Your `lv_conf.h` must byte-match the emulator's.** v9.5, color depth 16,
   same font set, same feature flags. Mismatch = segfault. Use the provided
   `sdk/include/lv_conf.h` verbatim.
3. **Entry point is `app_main(lv_obj_t* parent)`.** Hang your UI off `parent`.
4. **Clean up in `app_event(CZ_EV_EXIT_REQUEST)`.** Delete widgets you own.
5. **Do not export C++ symbols across the boundary.** Only `extern "C"`
   functions. C++ statics inside the library are fine.

---

## 2. App ABI contract (`cz_app.h`)

Every app exports exactly two C symbols:

```c
// sdk/include/cz_app.h
#pragma once
#include <lvgl.h>

#if defined(_WIN32)
  #define CZ_APP_EXPORT __declspec(dllexport)
#else
  #define CZ_APP_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

CZ_APP_EXPORT void app_main(lv_obj_t *parent);
CZ_APP_EXPORT void app_event(int type, void *data);

#ifdef __cplusplus
}
#endif

enum {
    CZ_EV_PAUSE         = 1,  // app is being backgrounded; persist state
    CZ_EV_RESUME        = 2,  // app is foregrounded again
    CZ_EV_EXIT_REQUEST  = 3,  // about to unload; free everything
    CZ_EV_SIDE_KEY      = 4,  // data: (int*) side-button id (ESC/HOME/...)
    // Future values are additive. Unknown types must be ignored by the app.
};
```

**Why this shape:**

- `app_main(parent)` — one UI entry point. `parent` is a full-screen container
  already sized 320×170. The app never touches display / screen APIs.
- `app_event(int, void*)` — system notifications. Integer type + opaque data
  keeps the ABI cheap to extend. Old apps that don't know a new type just
  ignore it.
- No string-keyed events, no C++ types, no callbacks with ownership semantics.
- `lvgl_version` is pinned in `app-builder.json` — CI refuses to build if the
  app's declared version and the host's version disagree on major/minor.

Apps that only need UI provide `app_main` and leave `app_event` as a one-line
no-op. It is mandatory so the host can always call it.

---

## 3. Why we use LVGL's API directly (no wrapper SDK)

APPLaunch and UserDemo on the real device are written against LVGL v9 directly.
If the desktop emulator introduced its own wrapper, the two surfaces would
drift and every app would need double maintenance. LVGL v9's API is stable
within the 9.x line (we pin 9.5); we version-lock so that 9.6 adoption is an
explicit coordinated bump, not a silent breakage.

We keep the host→app surface (§2) minimal precisely because LVGL already is
the SDK.

---

## 4. Windows LVGL + emulator — known issues and plan

Windows currently builds the emulator with `EMU_STATIC_APP=1` (the app is
static-linked into the exe). To reach the same "host + dlopen'd app" model as
mac/Linux we need to resolve these:

| # | Issue | Root cause | Plan |
|---|---|---|---|
| 1 | PE requires all symbols resolved at link time | No equivalent of ELF `--unresolved-symbols=ignore-all` or Mach-O `-undefined dynamic_lookup` | Produce `lvgl.dll` + `liblvgl.dll.a` import lib; app links the import lib |
| 2 | LVGL global data not marked dllexport | v9 headers use `LV_ATTRIBUTE_EXTERN_DATA` inconsistently for fonts/styles/builtin tables | Apply a one-shot Windows export header that blankets `__declspec(dllexport/dllimport)` over the public API. Revisit on each LVGL upgrade. |
| 3 | MinGW `__attribute__((weak))` doesn't link | Known MinGW limitation | Already handled via macro in `emu_compat_win.h:9` |
| 4 | C++ runtime duplicated across DLLs | MinGW defaults to `-static-libstdc++`; EXE and app.dll each bring a copy | App ABI is `extern "C"` only (§2). LVGL is C. No C++ objects cross the boundary. |
| 5 | App can't reach emulator symbols via `RTLD_DEFAULT` | Windows hides symbols unless exported | Emulator uses `__declspec(dllexport)` whitelist for the few host-provided helpers (or `--export-all-symbols` during bring-up) |
| 6 | Risk of two `lvgl.dll` copies loaded | If app.dll and emulator.exe resolve different copies, LVGL state splits | CI packs both into one directory; `czdev run` uses `SetDllDirectory` to pin lookup |
| 7 | SDL2 / freetype DLL bundling | MinGW runtime deps | Existing `ldd | awk` logic in `emulator-build.yml:116` stays |
| 8 | Freetype disabled on Windows | Present workaround; CJK fallback differs from mac/Linux | Accept for now; re-enable in a dedicated follow-up task |

The app source does **not** change per platform — all Windows-specific work
lives in the emulator + LVGL build.

---

## 5. Explicit non-goals (this development cycle)

These are good ideas but deliberately **not** in scope right now, to keep the
LVGL desktop-dev loop small and shippable:

- **Python / Qt / Tkinter / raw-framebuffer examples.** They require syscall
  interception (fb0 mmap, evdev, ALSA). Use the existing
  `CardputerZero-Examples/scripts/dev-on-mac/` Docker path for those.
- **Rust `framebuffer` + `evdev` examples.** Same reason.
- **Subprocess + shared-memory runtime channel.** The dlopen model covers
  100% of LVGL apps. Subprocess/SHM is reserved for a later pass when we
  want to cover fb/Qt/Python.
- **Hot `dlclose` / live reload.** `dlclose` is effectively a no-op on macOS
  and dangerous across C++ statics. `czdev watch` will rebuild and restart
  the emulator process instead.
- **Emscripten / WASM target for `czdev`.** The web playground continues to
  build via the emulator's own CI; `czdev` stays desktop-only.
- **GUI IDE.** CLI first. Tauri shell is parked until the CLI loop is solid.

---

## 6. Directory layout (target state)

```
CardputerZero-AppBuilder/
├── emulator/                    # git submodule → eggfly/M5CardputerZero-Emulator
├── crates/
│   └── czdev/                   # Rust CLI: list / build / run / watch / deploy / doctor
├── src-tauri/                   # existing Tauri shell (unchanged)
├── sdk/
│   ├── include/
│   │   ├── cz_app.h             # app ABI (§2)
│   │   └── lv_conf.h            # pinned LVGL config (must match emulator's)
│   └── cmake/
│       └── CZApp.cmake          # cz_add_lvgl_app() helper
└── docs/
    └── DESKTOP_DEV.md           # this file
```

App repos consume the SDK by pointing CMake at `sdk/cmake/CZApp.cmake`; they
do not vendor LVGL.
