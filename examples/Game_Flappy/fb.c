#define _POSIX_C_SOURCE 200809L
#include "fb.h"
#include "font8x8.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int fb_open(fb_t *fb, const char *path) {
    memset(fb, 0, sizeof(*fb));
    fb->fd = open(path, O_RDWR);
    if (fb->fd < 0) { perror("open fb"); return -1; }
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo) < 0) { perror("FBIOGET_VSCREENINFO"); return -1; }
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->finfo) < 0) { perror("FBIOGET_FSCREENINFO"); return -1; }
    if (fb->vinfo.bits_per_pixel != 16 && fb->vinfo.bits_per_pixel != 32) {
        fprintf(stderr, "unsupported bpp %u\n", fb->vinfo.bits_per_pixel);
        return -1;
    }
    fb->size = (size_t)fb->finfo.line_length * fb->vinfo.yres;
    fb->mem = mmap(NULL, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if (fb->mem == MAP_FAILED) { perror("mmap"); fb->mem = NULL; return -1; }
    return 0;
}

void fb_close(fb_t *fb) {
    if (fb->mem) munmap(fb->mem, fb->size);
    if (fb->fd >= 0) close(fb->fd);
    fb->mem = NULL;
    fb->fd = -1;
}

uint32_t fb_rgb(const fb_t *fb, uint8_t r, uint8_t g, uint8_t b) {
    if (fb->vinfo.bits_per_pixel == 16) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void fb_pixel(fb_t *fb, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= (int)fb->vinfo.xres || y >= (int)fb->vinfo.yres) return;
    size_t off = (size_t)y * fb->finfo.line_length + (size_t)x * (fb->vinfo.bits_per_pixel / 8);
    if (fb->vinfo.bits_per_pixel == 16) {
        *(uint16_t *)(fb->mem + off) = (uint16_t)color;
    } else {
        *(uint32_t *)(fb->mem + off) = color;
    }
}

void fb_fill(fb_t *fb, int x, int y, int w, int h, uint32_t color) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w; if (x1 > (int)fb->vinfo.xres) x1 = fb->vinfo.xres;
    int y1 = y + h; if (y1 > (int)fb->vinfo.yres) y1 = fb->vinfo.yres;
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            fb_pixel(fb, xx, yy, color);
}

void fb_clear(fb_t *fb, uint32_t color) {
    fb_fill(fb, 0, 0, fb->vinfo.xres, fb->vinfo.yres, color);
}

void fb_char(fb_t *fb, int x, int y, char c, uint32_t color, int scale) {
    if ((unsigned char)c >= 128) return;
    const uint8_t *g = font8x8[(unsigned char)c];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        fb_pixel(fb, x + col * scale + sx, y + row * scale + sy, color);
            }
        }
    }
}

void fb_text(fb_t *fb, int x, int y, const char *s, uint32_t color, int scale) {
    int cx = x;
    while (*s) {
        fb_char(fb, cx, y, *s, color, scale);
        cx += 8 * scale;
        s++;
    }
}
