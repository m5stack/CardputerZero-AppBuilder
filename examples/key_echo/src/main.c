/* key_echo — navigate LVGL's built-in keypad_encoder demo with the keyboard.
 *
 * Uses LVGL's built-in demo_keypad_encoder (force-loaded from the emulator)
 * together with the SDK's default weak keyboard stubs. Pressing keys on the
 * emulator skin moves focus through the widgets.
 */
#include <cz_app.h>

extern void lv_demo_keypad_encoder(void);

void app_main(lv_obj_t *parent)
{
    (void)parent;
    lv_demo_keypad_encoder();
}

void app_event(int type, void *data) { (void)type; (void)data; }
