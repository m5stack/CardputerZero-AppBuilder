#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>
#include <linux/fb.h>

typedef struct {
    int fd;
    uint8_t *mem;
    size_t size;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
} fb_t;

int  fb_open(fb_t *fb, const char *path);
void fb_close(fb_t *fb);

uint32_t fb_rgb(const fb_t *fb, uint8_t r, uint8_t g, uint8_t b);
void fb_pixel(fb_t *fb, int x, int y, uint32_t color);
void fb_fill(fb_t *fb, int x, int y, int w, int h, uint32_t color);
void fb_clear(fb_t *fb, uint32_t color);
void fb_char(fb_t *fb, int x, int y, char c, uint32_t color, int scale);
void fb_text(fb_t *fb, int x, int y, const char *s, uint32_t color, int scale);

#endif
