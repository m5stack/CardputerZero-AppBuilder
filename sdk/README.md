# CardputerZero App SDK

Headers and CMake helpers for building LVGL apps that load into the emulator
and, unchanged, into APPLaunch on the device.

## Contents

```
sdk/
├── include/
│   ├── cz_app.h        # App ABI: app_main / app_event / CZ_EV_*
│   └── lv_conf.h       # Pinned LVGL v9.5 config (must byte-match host's)
└── cmake/
    └── CZApp.cmake     # cz_add_lvgl_app(target SOURCES ...)
```

## Writing an app

```c
#include <cz_app.h>

static lv_obj_t *label;

void app_main(lv_obj_t *parent) {
    label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, CardputerZero!");
    lv_obj_center(label);
}

void app_event(int type, void *data) {
    (void)data;
    if (type == CZ_EV_EXIT_REQUEST && label) {
        lv_obj_del(label);
        label = NULL;
    }
}
```

And a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(hello_cz C)

# Point at the SDK (usually the CardputerZero-AppBuilder sdk/ dir).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sdk/cmake")
include(CZApp)

cz_add_lvgl_app(hello_cz SOURCES src/hello_cz.c)
```

Build with plain CMake; `czdev build` / `czdev run` wrap this for you.

## Non-negotiable rules

See `docs/DESKTOP_DEV.md` for the full rationale.

1. Don't call `lv_init()` or `lv_display_create()`.
2. Use this SDK's `lv_conf.h` — don't ship your own.
3. Export only `extern "C"` symbols (`app_main`, `app_event`).
4. Delete widgets you own in `CZ_EV_EXIT_REQUEST`.
