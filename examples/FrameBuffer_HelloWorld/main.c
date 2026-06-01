#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>

#include "font8x8_basic.h"

static uint32_t pack_color(const struct fb_var_screeninfo *v, uint8_t r, uint8_t g, uint8_t b) {
    if (v->bits_per_pixel == 16) {
        uint16_t c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        return c;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void put_pixel(uint8_t *fb, const struct fb_var_screeninfo *v,
                      const struct fb_fix_screeninfo *f,
                      int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= (int)v->xres || y >= (int)v->yres) return;
    size_t off = (size_t)y * f->line_length + (size_t)x * (v->bits_per_pixel / 8);
    if (v->bits_per_pixel == 16) {
        *(uint16_t *)(fb + off) = (uint16_t)color;
    } else {
        *(uint32_t *)(fb + off) = color;
    }
}

static void fill_rect(uint8_t *fb, const struct fb_var_screeninfo *v,
                      const struct fb_fix_screeninfo *f,
                      int x, int y, int w, int h, uint32_t color) {
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++)
            put_pixel(fb, v, f, x + xx, y + yy, color);
}

static void draw_border(uint8_t *fb, const struct fb_var_screeninfo *v,
                        const struct fb_fix_screeninfo *f, uint32_t color) {
    int W = v->xres, H = v->yres;
    for (int x = 0; x < W; x++) { put_pixel(fb, v, f, x, 0, color); put_pixel(fb, v, f, x, H - 1, color); }
    for (int y = 0; y < H; y++) { put_pixel(fb, v, f, 0, y, color); put_pixel(fb, v, f, W - 1, y, color); }
}

static void draw_char(uint8_t *fb, const struct fb_var_screeninfo *v,
                      const struct fb_fix_screeninfo *f,
                      int x, int y, char c, uint32_t color, int scale) {
    if ((unsigned char)c >= 128) return;
    const uint8_t *g = font8x8_basic[(unsigned char)c];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        put_pixel(fb, v, f, x + col * scale + sx, y + row * scale + sy, color);
            }
        }
    }
}

static void draw_string(uint8_t *fb, const struct fb_var_screeninfo *v,
                        const struct fb_fix_screeninfo *f,
                        int x, int y, const char *s, uint32_t color, int scale) {
    int cx = x;
    while (*s) {
        draw_char(fb, v, f, cx, y, *s, color, scale);
        cx += 8 * scale;
        s++;
    }
}

static int open_input_device(void) {
    const char *env = getenv("INPUT_DEV");
    if (env && *env) {
        int fd = open(env, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) return fd;
    }
    DIR *d = opendir("/dev/input");
    if (!d) return -1;
    struct dirent *e;
    int best = -1;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        unsigned long evbit = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0 && (evbit & (1UL << EV_KEY))) {
            best = fd;
            break;
        }
        close(fd);
    }
    closedir(d);
    return best;
}

int main(void) {
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd < 0) { perror("open /dev/fb0"); return 1; }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) { perror("FBIOGET_VSCREENINFO"); return 1; }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) < 0) { perror("FBIOGET_FSCREENINFO"); return 1; }

    if (vinfo.xres != 320 || vinfo.yres != 170) {
        fprintf(stderr, "warning: expected 320x170, got %ux%u\n", vinfo.xres, vinfo.yres);
    }
    if (vinfo.bits_per_pixel != 16 && vinfo.bits_per_pixel != 32) {
        fprintf(stderr, "unsupported bpp %u\n", vinfo.bits_per_pixel);
        return 1;
    }

    size_t screensize = (size_t)finfo.line_length * vinfo.yres;
    uint8_t *fb = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); return 1; }

    uint32_t bg = pack_color(&vinfo, 0x00, 0x00, 0x20);
    uint32_t fg = pack_color(&vinfo, 0xFF, 0xFF, 0xFF);
    uint32_t accent = pack_color(&vinfo, 0x40, 0xC0, 0xFF);

    fill_rect(fb, &vinfo, &finfo, 0, 0, vinfo.xres, vinfo.yres, bg);
    draw_border(fb, &vinfo, &finfo, accent);

    const char *msg = "Hello, CardputerZero!";
    int scale = 2;
    int msg_w = (int)strlen(msg) * 8 * scale;
    int msg_h = 8 * scale;
    int mx = ((int)vinfo.xres - msg_w) / 2;
    int my = ((int)vinfo.yres - msg_h) / 2;
    draw_string(fb, &vinfo, &finfo, mx, my, msg, fg, scale);

    const char *hint = "ESC or q to quit";
    int hw = (int)strlen(hint) * 8;
    draw_string(fb, &vinfo, &finfo, ((int)vinfo.xres - hw) / 2, (int)vinfo.yres - 14, hint, accent, 1);

    int kbd = open_input_device();
    if (kbd < 0) fprintf(stderr, "no input device; will run until killed\n");

    for (;;) {
        if (kbd < 0) { pause(); break; }
        struct pollfd pfd = { .fd = kbd, .events = POLLIN };
        int pr = poll(&pfd, 1, 1000);
        if (pr < 0) { if (errno == EINTR) continue; break; }
        if (pr == 0) continue;
        struct input_event ev;
        ssize_t n;
        while ((n = read(kbd, &ev, sizeof(ev))) == sizeof(ev)) {
            if (ev.type == EV_KEY && ev.value == 1) {
                if (ev.code == KEY_ESC || ev.code == KEY_Q) goto done;
            }
        }
        if (n < 0 && errno != EAGAIN) break;
    }
done:
    munmap(fb, screensize);
    close(fbfd);
    if (kbd >= 0) close(kbd);
    return 0;
}
