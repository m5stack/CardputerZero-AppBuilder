/**
 * @file cz_app.h
 * @brief CardputerZero application ABI.
 *
 * Contract between a CardputerZero app and its host (desktop emulator or
 * on-device APPLaunch). See docs/DESKTOP_DEV.md §2 for the rationale.
 *
 * An app is a shared library that exports exactly two C symbols:
 *   - void app_main (lv_obj_t *parent)
 *   - void app_event(int type, void *data)
 *
 * The host owns the LVGL instance. Do NOT call lv_init() or
 * lv_display_create(); build your UI by hanging LVGL objects off `parent`.
 *
 * The ABI is intentionally minimal (two extern "C" entry points, int + opaque
 * data for events) to stay cheap across dlopen/LoadLibrary boundaries and to
 * make additions source-compatible — unknown event types must be ignored.
 */
#ifndef CZ_APP_H
#define CZ_APP_H

#include <lvgl.h>

#if defined(_WIN32)
#define CZ_APP_EXPORT __declspec(dllexport)
#else
#define CZ_APP_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the app's UI.
 *
 * Called exactly once, after the host has initialised LVGL, a 320×170 display
 * and the keypad input device. The app attaches its widgets under `parent`,
 * which fills the screen. The app must not call lv_init().
 */
CZ_APP_EXPORT void app_main(lv_obj_t *parent);

/**
 * @brief Receive a system event from the host.
 *
 * Called on the LVGL thread. `type` is one of the CZ_EV_* enum values. Apps
 * that do not care about events still export a no-op body — the host always
 * calls this symbol if it exists. Unknown types must be silently ignored.
 *
 * @param type   event kind (CZ_EV_PAUSE, CZ_EV_RESUME, …)
 * @param data   optional type-specific payload, may be NULL
 */
CZ_APP_EXPORT void app_event(int type, void *data);

#ifdef __cplusplus
}
#endif

/** Event codes. Values are stable across versions; additions are append-only. */
enum {
    CZ_EV_PAUSE        = 1,  /**< app is about to background; persist state */
    CZ_EV_RESUME       = 2,  /**< app returned to foreground */
    CZ_EV_EXIT_REQUEST = 3,  /**< host is unloading; free owned widgets */
    CZ_EV_SIDE_KEY     = 4,  /**< data: (int*) side-button id (ESC/HOME/…) */
};

#endif /* CZ_APP_H */
