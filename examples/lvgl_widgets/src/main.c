/* lvgl_widgets — run LVGL's built-in demo_widgets inside the emulator.
 *
 * Proves the lvgl_demos force-load path: this app only declares the demo
 * entry point (the symbol is resolved from the emulator at dlopen time).
 */
#include <cz_app.h>

extern void lv_demo_widgets(void);

void app_main(lv_obj_t *parent)
{
    (void)parent;  // demo builds on the active screen itself
    lv_demo_widgets();
}

void app_event(int type, void *data) { (void)type; (void)data; }
