#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "fb.h"
#include "input.h"

#define LAND_W        10
#define LAND_H        10
#define TERRAIN_MAX   320
#define NUM_PADS      2
#define TICK_NS       (16666667L)

#define GRAVITY       0.05f
#define THRUST_A      0.12f
#define ROT_DEG       2.0f
#define FUEL_INIT     500

typedef enum { ST_PLAY, ST_LANDED, ST_CRASH } lst_t;

typedef struct { int x0, x1, y; int bonus; } pad_t;

typedef struct {
    int W, H;
    int terrain[TERRAIN_MAX];
    pad_t pads[NUM_PADS];
    float x, y;
    float vx, vy;
    float angle_deg; /* 0 = up */
    int   fuel;
    int   score;
    lst_t st;
    int   crash_ticks; /* frames left for explosion */
    int   sparks_x[24], sparks_y[24];
    int   sparks_vx[24], sparks_vy[24];
    uint32_t rng;
    uint32_t seed;
    int   thrusting;
} lander_t;

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

static int iabs(int a) { return a < 0 ? -a : a; }
static int iclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void gen_terrain(lander_t *g) {
    int W = g->W, H = g->H;
    uint32_t s = g->seed ? g->seed : 1;

    /* jagged baseline */
    int base_lo = H - 20, base_hi = H - 50;
    if (base_hi < 60) base_hi = 60;
    int y = (base_lo + base_hi) / 2;
    for (int x = 0; x < W; x++) {
        int step = (int)(xs32(&s) % 5) - 2;
        y += step;
        if (y < base_hi) y = base_hi;
        if (y > base_lo) y = base_lo;
        g->terrain[x] = y;
    }

    /* two flat pads */
    int pad_w = 24;
    int gap = W / 3;
    int p0x = 20 + (int)(xs32(&s) % 30);
    int p1x = p0x + gap + (int)(xs32(&s) % 40);
    if (p1x + pad_w >= W) p1x = W - pad_w - 10;
    int bonuses[2] = { 2, 5 };
    int order = (int)(xs32(&s) & 1);
    g->pads[0].x0 = p0x; g->pads[0].x1 = p0x + pad_w;
    g->pads[0].y  = g->terrain[p0x];
    g->pads[0].bonus = bonuses[order];
    g->pads[1].x0 = p1x; g->pads[1].x1 = p1x + pad_w;
    g->pads[1].y  = g->terrain[p1x];
    g->pads[1].bonus = bonuses[1 - order];
    for (int p = 0; p < NUM_PADS; p++) {
        int py = g->pads[p].y;
        for (int x = g->pads[p].x0; x < g->pads[p].x1 && x < W; x++) g->terrain[x] = py;
    }
}

static void lander_reset(lander_t *g, int keep_score) {
    int score = keep_score ? g->score : 0;
    uint32_t seed = g->seed;
    int W = g->W, H = g->H;
    memset(g, 0, sizeof(*g));
    g->W = W; g->H = H;
    g->score = score;
    g->seed = seed;
    g->rng = seed ? seed : 1;
    gen_terrain(g);
    g->x = g->W / 2.0f;
    g->y = 16.0f;
    g->vx = 0.2f;
    g->vy = 0.0f;
    g->angle_deg = 0.0f;
    g->fuel = FUEL_INIT;
    g->st = ST_PLAY;
    g->crash_ticks = 0;
}

static void lander_init(lander_t *g, int W, int H) {
    memset(g, 0, sizeof(*g));
    g->W = W; g->H = H;
    g->seed = (uint32_t)time(NULL) ^ 0xABCD1234u;
    if (!g->seed) g->seed = 1;
    lander_reset(g, 0);
}

static void spawn_sparks(lander_t *g) {
    uint32_t *s = &g->rng;
    for (int i = 0; i < 24; i++) {
        g->sparks_x[i] = (int)g->x + LAND_W / 2;
        g->sparks_y[i] = (int)g->y + LAND_H / 2;
        g->sparks_vx[i] = (int)(xs32(s) % 7) - 3;
        g->sparks_vy[i] = (int)(xs32(s) % 7) - 4;
    }
}

static int pad_at(const lander_t *g, int x0, int x1) {
    for (int p = 0; p < NUM_PADS; p++) {
        if (x0 >= g->pads[p].x0 && x1 <= g->pads[p].x1) return p;
    }
    return -1;
}

static void step(lander_t *g, const input_state_t *in) {
    if (g->st == ST_CRASH) {
        if (g->crash_ticks > 0) {
            g->crash_ticks--;
            for (int i = 0; i < 24; i++) {
                g->sparks_x[i] += g->sparks_vx[i];
                g->sparks_y[i] += g->sparks_vy[i];
                g->sparks_vy[i] += 1;
            }
        }
        return;
    }
    if (g->st == ST_LANDED) return;

    /* rotation */
    if (in->held & KBIT_LEFT)  g->angle_deg -= ROT_DEG;
    if (in->held & KBIT_RIGHT) g->angle_deg += ROT_DEG;
    if (g->angle_deg > 180) g->angle_deg -= 360;
    if (g->angle_deg < -180) g->angle_deg += 360;

    /* thrust */
    g->thrusting = 0;
    if ((in->held & KBIT_UP) && g->fuel > 0) {
        float rad = g->angle_deg * (float)M_PI / 180.0f;
        /* 0 deg = up: direction (sin(a), -cos(a)) */
        g->vx += THRUST_A * sinf(rad);
        g->vy += -THRUST_A * cosf(rad);
        g->fuel -= 1;
        g->thrusting = 1;
    }

    /* gravity */
    g->vy += GRAVITY;
    g->x += g->vx;
    g->y += g->vy;

    /* wrap horizontally */
    if (g->x < 0) g->x += g->W;
    if (g->x >= g->W) g->x -= g->W;

    /* collision with terrain */
    int lx = (int)g->x;
    int ly = (int)g->y;
    int foot = ly + LAND_H;
    int x0 = iclamp(lx, 0, g->W - 1);
    int x1 = iclamp(lx + LAND_W - 1, 0, g->W - 1);
    int hit = 0;
    for (int x = x0; x <= x1; x++) {
        if (foot >= g->terrain[x]) { hit = 1; break; }
    }
    if (hit) {
        int pad = pad_at(g, x0, x1);
        int ang_ok = iabs((int)g->angle_deg) <= 8;
        int vy_ok  = g->vy <= 2.0f;
        int vx_ok  = fabsf(g->vx) <= 1.5f;
        if (pad >= 0 && ang_ok && vy_ok && vx_ok) {
            g->st = ST_LANDED;
            g->y = g->pads[pad].y - LAND_H;
            g->vx = g->vy = 0;
            g->score += 100 * g->pads[pad].bonus + g->fuel / 5;
        } else {
            g->st = ST_CRASH;
            g->crash_ticks = 60;
            spawn_sparks(g);
        }
    }

    /* ceiling clamp */
    if (g->y < 0) { g->y = 0; g->vy = 0; }
}

static void draw_lander(fb_t *fb, const lander_t *g, uint32_t body, uint32_t leg, uint32_t flame) {
    /* Draw a simple rotated-ish lander as a rect with legs; approximate rotation by shear not applied — use body rect + legs for clarity */
    int x = (int)g->x, y = (int)g->y;
    fb_fill(fb, x + 2, y + 1, LAND_W - 4, LAND_H - 4, body);
    fb_fill(fb, x, y + LAND_H - 3, 2, 3, leg);
    fb_fill(fb, x + LAND_W - 2, y + LAND_H - 3, 2, 3, leg);
    fb_fill(fb, x + 3, y, 4, 2, leg);
    if (g->thrusting) {
        fb_fill(fb, x + 3, y + LAND_H - 1, 4, 3, flame);
        fb_pixel(fb, x + 4, y + LAND_H + 2, flame);
    }
    /* crude angle indicator tick */
    int cx = x + LAND_W / 2;
    int cy = y + LAND_H / 2;
    float rad = g->angle_deg * (float)M_PI / 180.0f;
    int tx = cx + (int)(sinf(rad) * 6);
    int ty = cy - (int)(cosf(rad) * 6);
    fb_pixel(fb, tx, ty, leg);
    fb_pixel(fb, tx + 1, ty, leg);
    fb_pixel(fb, tx, ty + 1, leg);
}

static void render(fb_t *fb, const lander_t *g) {
    uint32_t sky    = fb_rgb(fb, 0x00, 0x00, 0x10);
    uint32_t ground = fb_rgb(fb, 0x60, 0x60, 0x70);
    uint32_t pad_c  = fb_rgb(fb, 0x40, 0xE0, 0x40);
    uint32_t body_c = fb_rgb(fb, 0xE0, 0xE0, 0xE0);
    uint32_t leg_c  = fb_rgb(fb, 0x80, 0x80, 0xFF);
    uint32_t flame  = fb_rgb(fb, 0xFF, 0xA0, 0x20);
    uint32_t white  = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t warn   = fb_rgb(fb, 0xFF, 0x60, 0x40);
    uint32_t star   = fb_rgb(fb, 0x80, 0x80, 0x90);

    int W = g->W, H = g->H;
    fb_clear(fb, sky);

    /* a few pseudo stars */
    uint32_t s = 0xCAFEBABEu ^ g->seed;
    for (int i = 0; i < 18; i++) {
        int sx = (int)(xs32(&s) % (uint32_t)W);
        int sy = (int)(xs32(&s) % 80);
        fb_pixel(fb, sx, sy, star);
    }

    /* terrain */
    for (int x = 0; x < W; x++) {
        int ty = g->terrain[x];
        fb_fill(fb, x, ty, 1, H - ty, ground);
    }
    /* pads + bonus text */
    for (int p = 0; p < NUM_PADS; p++) {
        fb_fill(fb, g->pads[p].x0, g->pads[p].y - 1, g->pads[p].x1 - g->pads[p].x0, 2, pad_c);
        char tb[8];
        snprintf(tb, sizeof(tb), "x%d", g->pads[p].bonus);
        int tx = (g->pads[p].x0 + g->pads[p].x1) / 2 - (int)strlen(tb) * 4;
        int ty = g->pads[p].y - 12;
        if (ty < 12) ty = 12;
        fb_text(fb, tx, ty, tb, pad_c, 1);
    }

    /* lander or explosion */
    if (g->st == ST_CRASH) {
        for (int i = 0; i < 24; i++) {
            fb_pixel(fb, g->sparks_x[i], g->sparks_y[i], flame);
            fb_pixel(fb, g->sparks_x[i] + 1, g->sparks_y[i], warn);
        }
    } else {
        draw_lander(fb, g, body_c, leg_c, flame);
    }

    /* HUD */
    char hud[64];
    float vmag = sqrtf(g->vx * g->vx + g->vy * g->vy);
    int ang_i = (int)g->angle_deg;
    uint32_t fuel_c = g->fuel < 50 ? warn : white;
    snprintf(hud, sizeof(hud), "VEL %.1f  FUEL %d  ANG %+d  SC %d", vmag, g->fuel, ang_i, g->score);
    fb_fill(fb, 0, 0, W, 10, fb_rgb(fb, 0x00, 0x00, 0x00));
    fb_text(fb, 2, 1, hud, fuel_c, 1);

    if (g->st == ST_LANDED) {
        const char *m1 = "LANDED!";
        const char *m2 = "SPACE to retry  ESC quit";
        int w1 = (int)strlen(m1) * 16;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 14, m1, pad_c, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 8, m2, white, 1);
    } else if (g->st == ST_CRASH && g->crash_ticks <= 0) {
        const char *m1 = "CRASHED";
        const char *m2 = "SPACE retry  R reseed  ESC quit";
        int w1 = (int)strlen(m1) * 16;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 14, m1, warn, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 8, m2, white, 1);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device found\n");

    lander_t g;
    lander_init(&g, (int)fb.vinfo.xres, (int)fb.vinfo.yres);

    input_state_t in;
    memset(&in, 0, sizeof(in));

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    while (running) {
        input_update(kbd, &in);
        if (in.pressed & KBIT_ESC) running = 0;
        if (in.pressed & KBIT_R) {
            g.seed = g.seed * 1664525u + 1013904223u;
            lander_reset(&g, 0);
        }
        if (in.pressed & KBIT_SPACE) {
            if (g.st != ST_PLAY) lander_reset(&g, g.st == ST_LANDED);
        }

        step(&g, &in);
        render(&fb, &g);

        next.tv_nsec += TICK_NS;
        while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    input_close(kbd);
    fb_close(&fb);
    return 0;
}
