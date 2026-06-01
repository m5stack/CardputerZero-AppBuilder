/* Demo_Matrix: green "matrix rain" screensaver on /dev/fb0 320x170.
 * ~20 columns of 16px wide (8x8 font scaled 2x). Press any key on the
 * Cardputer keypad (or Esc) to quit.
 *
 * Input is read from an evdev keypad device (non-blocking). Reading
 * stdin is not reliable when launched under APPLaunch (no controlling
 * tty), so we probe /dev/input for a real keyboard.
 */

#define _POSIX_C_SOURCE 200809L
/* usleep(3) is a legacy BSD API removed from POSIX.1-2008 but still
 * provided by glibc under _DEFAULT_SOURCE / _XOPEN_SOURCE. Without it
 * a strict -std=c11 build (like the on-device Debian 13 gcc) fails
 * with "implicit declaration of 'usleep'". Use nanosleep() below. */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "font8x8_basic.h"

#define FB_DEV "/dev/fb0"
#define W 320
#define H 170
#define CHAR_W 16   /* 8x8 font, scale 2 */
#define CHAR_H 16
#define COLS (W / CHAR_W)     /* 20 */
#define ROWS (H / CHAR_H + 1) /* 11 */

#define BITS_PER_LONG (8 * (int)sizeof(long))
#define NBITS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define TEST_BIT(bit, arr) ((arr)[(bit) / BITS_PER_LONG] & (1UL << ((bit) % BITS_PER_LONG)))

static uint16_t *fb;
/* Actual on-device fb dimensions, read from FBIOGET_VSCREENINFO. Using
 * hardcoded W=320 as the stride segfaults when the real fb is padded or
 * sized differently (the mmap is v.xres*v.yres*2 bytes). */
static int fb_w = W;
static int fb_h = H;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void put_pixel(int x, int y, uint16_t c) {
    if (x < 0 || y < 0 || x >= fb_w || y >= fb_h) return;
    fb[y * fb_w + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            put_pixel(x + i, y + j, c);
}

static void draw_char_at(int gx, int gy, char ch, uint16_t fg) {
    if ((unsigned char)ch >= 128) return;
    const uint8_t *g = font8x8_basic[(int)ch];
    int px = gx * CHAR_W;
    int py = gy * CHAR_H;
    /* clear cell */
    fill_rect(px, py, CHAR_W, CHAR_H, 0);
    for (int r = 0; r < 8; r++) {
        uint8_t bits = g[r];
        for (int c = 0; c < 8; c++) {
            if (bits & (0x80 >> c))
                fill_rect(px + c * 2, py + r * 2, 2, 2, fg);
        }
    }
}

static int heads[COLS];
static int speeds[COLS];
static char glyphs[COLS][ROWS];

static char rand_glyph(void) {
    const char *set = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!*+:-<>";
    return set[rand() % (int)strlen(set)];
}

/* Open an evdev keypad device (non-blocking). Same discovery logic as
 * Demo_2048: env override, Cardputer by-path, then scan, rejecting
 * HDMI CEC (has EV_REL) and devices without KEY_ESC/KEY_ENTER. */
static int open_keypad(void) {
    const char *env = getenv("INPUT_DEV");
    if (env && *env) {
        int fd = open(env, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) return fd;
    }
    int fd = open("/dev/input/by-path/platform-3f804000.i2c-event",
                  O_RDONLY | O_NONBLOCK);
    if (fd >= 0) return fd;

    DIR *d = opendir("/dev/input");
    if (!d) return -1;
    struct dirent *e;
    int best = -1;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int cand = open(path, O_RDONLY | O_NONBLOCK);
        if (cand < 0) continue;

        unsigned long evbits[NBITS(EV_MAX)] = {0};
        if (ioctl(cand, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
            close(cand); continue;
        }
        if (!TEST_BIT(EV_KEY, evbits)) { close(cand); continue; }
        if (TEST_BIT(EV_REL, evbits)) { close(cand); continue; }

        unsigned long keybits[NBITS(KEY_MAX)] = {0};
        if (ioctl(cand, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0 &&
            (TEST_BIT(KEY_ENTER, keybits) || TEST_BIT(KEY_ESC, keybits))) {
            best = cand;
            break;
        }
        close(cand);
    }
    closedir(d);
    return best;
}

/* Non-blocking: return 1 if a quit key (ESC, Q, BACKSPACE) was pressed.
 * Previously this returned 1 on ANY key press, which made the demo exit
 * instantly under APPLaunch because the launcher's own KEY_ENTER press
 * (from selecting the app) is still queued on the evdev device when we
 * open it — resulting in "starts then immediately exits". */
static int key_pressed(int kfd) {
    if (kfd < 0) return 0;
    struct input_event ev;
    int got = 0;
    for (;;) {
        ssize_t n = read(kfd, &ev, sizeof(ev));
        if (n == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_KEY && ev.value == 1) {
                if (ev.code == KEY_ESC || ev.code == KEY_Q ||
                    ev.code == KEY_BACKSPACE) got = 1;
            }
            continue;
        }
        break;
    }
    return got;
}

int main(void) {
    int fd = open(FB_DEV, O_RDWR);
    if (fd < 0) { perror("open fb"); return 1; }
    struct fb_var_screeninfo v;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &v) < 0) { perror("ioctl"); close(fd); return 1; }
    /* Use real fb dimensions for the stride so put_pixel stays in-bounds. */
    fb_w = (int)v.xres;
    fb_h = (int)v.yres;
    if (fb_w <= 0 || fb_h <= 0) { fprintf(stderr, "bad vinfo %ux%u\n", v.xres, v.yres); close(fd); return 1; }
    size_t bytes = (size_t)v.xres * v.yres * 2;
    fb = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    int kfd = open_keypad();
    /* If no keypad, still animate — user can SIGINT / kill to exit. */

    /* Drain any stale events from the launcher (e.g. the KEY_ENTER press
     * that selected this app), otherwise we'd exit immediately on the
     * first key_pressed() poll. */
    if (kfd >= 0) {
        struct input_event ev;
        while (read(kfd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) { }
    }

    srand((unsigned)time(NULL));
    for (int i = 0; i < COLS; i++) {
        heads[i] = rand() % ROWS;
        speeds[i] = 1 + rand() % 3;
        for (int r = 0; r < ROWS; r++) glyphs[i][r] = rand_glyph();
    }
    memset(fb, 0, bytes);

    uint16_t GREEN = rgb565(0, 255, 70);
    uint16_t DARK = rgb565(0, 120, 40);
    uint16_t BRIGHT = rgb565(200, 255, 200);

    for (;;) {
        if (key_pressed(kfd)) break;

        for (int c = 0; c < COLS; c++) {
            /* dim trail: redraw column faded */
            for (int r = 0; r < ROWS; r++) {
                int dist = (heads[c] - r + ROWS) % ROWS;
                uint16_t col;
                if (dist == 0) col = BRIGHT;
                else if (dist < 4) col = GREEN;
                else if (dist < 8) col = DARK;
                else continue; /* leave black */
                draw_char_at(c, r, glyphs[c][r], col);
            }
            /* step */
            static int tick = 0;
            if (tick % speeds[c] == 0) {
                heads[c] = (heads[c] + 1) % ROWS;
                glyphs[c][heads[c]] = rand_glyph();
            }
            tick++;
        }
        /* ~80 ms frame delay using POSIX nanosleep (usleep was removed
         * from POSIX.1-2008 and won't compile under -std=c11 with only
         * _POSIX_C_SOURCE defined). */
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 80L * 1000 * 1000 };
        nanosleep(&ts, NULL);
    }

    if (kfd >= 0) close(kfd);
    munmap(fb, bytes);
    close(fd);
    return 0;
}
