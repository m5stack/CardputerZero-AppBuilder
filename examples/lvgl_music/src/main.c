/* lvgl_music — run LVGL's built-in demo_music inside the emulator. */
#include <cz_app.h>

extern lv_obj_t *lv_demo_music(void);

void app_main(lv_obj_t *parent)
{
    (void)parent;
    lv_demo_music();
}

void app_event(int type, void *data) { (void)type; (void)data; }
