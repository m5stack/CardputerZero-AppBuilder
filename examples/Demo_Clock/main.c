/* Demo_Clock: big-font clock on /dev/fb0 (RGB565, 320x170).
 * Centered HH:MM with smaller seconds, date line at bottom.
 * Refreshes every 500 ms. Reuses style from ../FrameBuffer_HelloWorld.
 */

#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "font8x8_basic.h"

#define FB_DEV "/dev/fb0"
#define SCR_W 320
#define SCR_H 170

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void put_pixel(uint16_t *fb, int x, int y, uint16_t color) {
    if (x < 0 || y < 0 || x >= SCR_W || y >= SCR_H) return;
    fb[y * SCR_W + x] = color;
}

/* Draw a single char at scale (1 = 8x8, 4 = 32x32). */
static void draw_char(uint16_t *fb, int x, int y, char c, int scale, uint16_t fg) {
    if ((unsigned char)c >= 128) return;
    const uint8_t *g = font8x8_basic[(int)c];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++)
                        put_pixel(fb, x + col * scale + dx, y + row * scale + dy, fg);
            }
        }
    }
}

static void draw_str(uint16_t *fb, int x, int y, const char *s, int scale, uint16_t fg) {
    while (*s) {
        draw_char(fb, x, y, *s, scale, fg);
        x += 8 * scale;
        s++;
    }
}

static void fill(uint16_t *fb, uint16_t c) {
    for (int i = 0; i < SCR_W * SCR_H; i++) fb[i] = c;
}

int main(void) {
    int fd = open(FB_DEV, O_RDWR);
    if (fd < 0) { perror("open fb0"); return 1; }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) { perror("ioctl"); close(fd); return 1; }

    size_t bytes = (size_t)vinfo.xres * vinfo.yres * 2;
    uint16_t *fb = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    const uint16_t BG = rgb565(0, 0, 0);
    const uint16_t FG = rgb565(255, 255, 255);
    const uint16_t DIM = rgb565(120, 120, 120);

    for (;;) {
        time_t t = time(NULL);
        struct tm lt;
        localtime_r(&t, &lt);

        char hm[8], ss[4], date[16];
        snprintf(hm, sizeof(hm), "%02d:%02d", lt.tm_hour, lt.tm_min);
        snprintf(ss, sizeof(ss), "%02d", lt.tm_sec);
        snprintf(date, sizeof(date), "%04d-%02d-%02d",
                 lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);

        fill(fb, BG);
        /* HH:MM at scale 4 -> 32px tall, 5 chars wide = 160px */
        int hm_w = (int)strlen(hm) * 8 * 4;
        int hm_x = (SCR_W - hm_w) / 2;
        int hm_y = 30;
        draw_str(fb, hm_x, hm_y, hm, 4, FG);

        /* seconds smaller, scale 2, right-aligned near HH:MM baseline */
        int ss_x = hm_x + hm_w + 4;
        int ss_y = hm_y + 16;
        draw_str(fb, ss_x, ss_y, ss, 2, DIM);

        /* date bottom */
        int d_w = (int)strlen(date) * 8 * 2;
        draw_str(fb, (SCR_W - d_w) / 2, SCR_H - 20, date, 2, DIM);

        usleep(500 * 1000);
    }

    munmap(fb, bytes);
    close(fd);
    return 0;
}
