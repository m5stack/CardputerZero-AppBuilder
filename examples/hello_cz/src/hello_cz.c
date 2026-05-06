/* Hello CardputerZero — minimal LVGL app following the cz_app.h ABI. */
#include <cz_app.h>

static lv_obj_t *g_label;

void app_main(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    g_label = lv_label_create(parent);
    lv_label_set_text(g_label, "Hello, CardputerZeo!\n320x170 LVGL desktop emulator");
    lv_obj_set_style_text_color(g_label, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_align(g_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(g_label);
}

void app_event(int type, void *data)
{
    (void)data;
    if (type == CZ_EV_EXIT_REQUEST && g_label) {
        lv_obj_del(g_label);
        g_label = NULL;
    }
}
