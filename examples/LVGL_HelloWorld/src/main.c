/* LVGL Hello World for M5 CardputerZero (RP3A0 aarch64, 320x170 ST7789V). */

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lvgl.h"
#include "src/drivers/display/fb/lv_linux_fbdev.h"
#include "src/drivers/evdev/lv_evdev.h"

static uint32_t g_click_count = 0;
static lv_obj_t *g_count_label = NULL;

static void bump_click(void)
{
    g_click_count++;
    if (g_count_label) {
        lv_label_set_text_fmt(g_count_label, "Clicks: %u", g_click_count);
    }
}

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
        bump_click();
        return;
    }
    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ENTER) {
            bump_click();
        }
    }
}

/* Group-level fallback: some evdev/indev setups route LV_EVENT_KEY to the
 * group's focused obj only when the obj is in a specific editable state.
 * Catch ENTER on any focused obj in our single-button group so the click
 * counter always advances, even if the button's own LV_EVENT_CLICKED
 * handler is not triggered by the default group action. */
static lv_group_t *g_group = NULL;

static void indev_read_cb_unused(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    (void)data;
}

#include <linux/input-event-codes.h>

#define BITS_PER_LONG (8 * (int)sizeof(long))
#define NBITS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define TEST_BIT(bit, arr) ((arr)[(bit) / BITS_PER_LONG] & (1UL << ((bit) % BITS_PER_LONG)))

/* Scan /dev/input/event* for a real keyboard-style device.
 *
 * We reject anything that also reports EV_REL, which filters out the
 * vc4-hdmi CEC pseudo-keyboard at event0 (reports EV_KEY + EV_REL but
 * never actually emits key events from the Cardputer keypad). We also
 * require KEY_ENTER or KEY_ESC in the supported key bitmap so we only
 * pick a device that can actually dispatch the keys this app cares
 * about.
 */
static int find_keypad_event_path(char *out, size_t out_sz)
{
    DIR *d = opendir("/dev/input");
    if (!d) return -1;

    struct dirent *ent;
    int found = -1;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        unsigned long evbits[NBITS(EV_MAX)] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
            close(fd); continue;
        }
        if (!TEST_BIT(EV_KEY, evbits)) { close(fd); continue; }
        /* Reject mouse/CEC remote (event0 on Pi has EV_REL). */
        if (TEST_BIT(EV_REL, evbits)) { close(fd); continue; }

        unsigned long keybits[NBITS(KEY_MAX)] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0 &&
            (TEST_BIT(KEY_ENTER, keybits) || TEST_BIT(KEY_ESC, keybits))) {
            snprintf(out, out_sz, "%s", path);
            found = 0;
            close(fd);
            break;
        }
        close(fd);
    }
    closedir(d);
    return found;
}

static void build_ui(lv_group_t *group)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101018), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Hello, CardputerZero!");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 40);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_KEY, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Press ENTER");
    lv_obj_center(btn_label);

    g_count_label = lv_label_create(scr);
    lv_label_set_text(g_count_label, "Clicks: 0");
    lv_obj_set_style_text_color(g_count_label, lv_color_hex(0xAAD4FF), LV_PART_MAIN);
    lv_obj_align(g_count_label, LV_ALIGN_BOTTOM_MID, 0, -16);

    lv_group_add_obj(group, btn);
    lv_group_focus_obj(btn);
}

int main(void)
{
    lv_init();

    /* Framebuffer display. */
    lv_display_t *disp = lv_linux_fbdev_create();
    if (!disp) {
        fprintf(stderr, "lv_linux_fbdev_create failed\n");
        return 1;
    }
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_display_set_resolution(disp, 320, 170);

    /* Keypad input (evdev). Probe order:
     *   1. $INPUT_DEV env override
     *   2. /dev/input/by-path/platform-3f804000.i2c-event (Cardputer keypad)
     *   3. Scan /dev/input/event*, skipping HDMI CEC / mouse-like devices.
     *
     * Do NOT blindly try /dev/input/event0 first: on the Pi CM0 that is
     * vc4-hdmi (CEC) which advertises EV_KEY but never emits the
     * Cardputer key events, leaving focus unreachable.
     */
    const char *kp_env = getenv("INPUT_DEV");
    char kp_path[64] = {0};
    if (kp_env && *kp_env) {
        snprintf(kp_path, sizeof(kp_path), "%s", kp_env);
    } else {
        const char *stable = "/dev/input/by-path/platform-3f804000.i2c-event";
        if (access(stable, R_OK) == 0) {
            snprintf(kp_path, sizeof(kp_path), "%s", stable);
        } else {
            find_keypad_event_path(kp_path, sizeof(kp_path));
        }
    }
    lv_indev_t *indev = NULL;
    if (kp_path[0]) {
        indev = lv_evdev_create(LV_INDEV_TYPE_KEYPAD, kp_path);
        if (indev) fprintf(stderr, "evdev: using %s\n", kp_path);
        else fprintf(stderr, "evdev: failed to open %s\n", kp_path);
    } else {
        fprintf(stderr, "evdev: no keypad device found\n");
    }

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    if (indev) lv_indev_set_group(indev, group);

    build_ui(group);

    for (;;) {
        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms > 50) sleep_ms = 50;
        usleep(sleep_ms * 1000);
    }

    return 0;
}
