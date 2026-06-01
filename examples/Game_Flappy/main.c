#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "fb.h"
#include "input.h"

#define BIRD_W      8
#define BIRD_H      6
#define PIPE_W      32
#define GAP_H       40
#define PIPE_DX     80
#define SCROLL      1.5f
#define GRAVITY     0.35f
#define JUMP_IMP    (-3.8f)
#define MAX_PIPES   8
#define TICK_NS     (16666667L)

typedef struct {
    float x;
    int   gap_y;  /* top-edge y of gap */
    int   scored;
    int   active;
} pipe_t;

typedef struct {
    int W, H;
    float by, bv;
    int   bx;
    pipe_t pipes[MAX_PIPES];
    float spawn_x; /* x coord where next pipe appears (relative to world) */
    int   score;
    int   best;
    int   alive;
    uint32_t rng;
} flap_t;

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

static int rand_gap(flap_t *g) {
    int margin = 10;
    int lo = margin;
    int hi = g->H - margin - GAP_H;
    if (hi <= lo) return lo;
    return lo + (int)(xs32(&g->rng) % (uint32_t)(hi - lo));
}

static void spawn_pipe(flap_t *g, float x) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!g->pipes[i].active) {
            g->pipes[i].active = 1;
            g->pipes[i].x = x;
            g->pipes[i].gap_y = rand_gap(g);
            g->pipes[i].scored = 0;
            return;
        }
    }
}

static void flap_init(flap_t *g, int W, int H, int keep_best) {
    int best = keep_best ? g->best : 0;
    memset(g, 0, sizeof(*g));
    g->W = W; g->H = H;
    g->best = best;
    g->rng = (uint32_t)time(NULL) ^ 0xC0FFEE11u;
    if (!g->rng) g->rng = 1;
    g->bx = W / 4;
    g->by = H / 2.0f;
    g->bv = 0;
    g->alive = 1;
    g->score = 0;
    /* prime a pipe off-screen right */
    spawn_pipe(g, (float)W + 20);
    spawn_pipe(g, (float)W + 20 + PIPE_DX);
    spawn_pipe(g, (float)W + 20 + PIPE_DX * 2);
    g->spawn_x = (float)W + 20 + PIPE_DX * 2;
}

static int rect_overlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void step(flap_t *g, const input_state_t *in) {
    if (!g->alive) return;

    if ((in->pressed & KBIT_SPACE) || (in->pressed & KBIT_UP)) {
        g->bv = JUMP_IMP;
    }
    g->bv += GRAVITY;
    if (g->bv > 6.0f) g->bv = 6.0f;
    g->by += g->bv;

    /* scroll pipes left */
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!g->pipes[i].active) continue;
        g->pipes[i].x -= SCROLL;
        if (g->pipes[i].x + PIPE_W < 0) g->pipes[i].active = 0;
    }
    g->spawn_x -= SCROLL;
    if (g->spawn_x < g->W) {
        spawn_pipe(g, g->spawn_x + PIPE_DX);
        g->spawn_x += PIPE_DX;
    }

    /* collisions & scoring */
    int bx = g->bx, by = (int)g->by;
    if (by < 0 || by + BIRD_H >= g->H) { g->alive = 0; }
    for (int i = 0; i < MAX_PIPES && g->alive; i++) {
        if (!g->pipes[i].active) continue;
        int px = (int)g->pipes[i].x;
        int gy = g->pipes[i].gap_y;
        /* top pipe */
        if (rect_overlap(bx, by, BIRD_W, BIRD_H, px, 0, PIPE_W, gy)) { g->alive = 0; break; }
        /* bottom pipe */
        if (rect_overlap(bx, by, BIRD_W, BIRD_H, px, gy + GAP_H, PIPE_W, g->H - gy - GAP_H)) {
            g->alive = 0; break;
        }
        /* scoring */
        if (!g->pipes[i].scored && px + PIPE_W < bx) {
            g->pipes[i].scored = 1;
            g->score++;
            if (g->score > g->best) g->best = g->score;
        }
    }
}

static void render(fb_t *fb, const flap_t *g) {
    uint32_t sky   = fb_rgb(fb, 0x3A, 0xA8, 0xDC);
    uint32_t ground= fb_rgb(fb, 0x80, 0x50, 0x20);
    uint32_t pipec = fb_rgb(fb, 0x30, 0xB0, 0x40);
    uint32_t pipe2 = fb_rgb(fb, 0x20, 0x80, 0x30);
    uint32_t bird  = fb_rgb(fb, 0xFF, 0xE0, 0x30);
    uint32_t beak  = fb_rgb(fb, 0xE0, 0x60, 0x20);
    uint32_t white = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t black = fb_rgb(fb, 0x10, 0x10, 0x10);

    int W = g->W, H = g->H;
    fb_clear(fb, sky);
    fb_fill(fb, 0, H - 4, W, 4, ground);

    for (int i = 0; i < MAX_PIPES; i++) {
        if (!g->pipes[i].active) continue;
        int px = (int)g->pipes[i].x;
        int gy = g->pipes[i].gap_y;
        fb_fill(fb, px + 1, 0, PIPE_W - 2, gy, pipec);
        fb_fill(fb, px, gy - 4, PIPE_W, 4, pipe2);
        fb_fill(fb, px + 1, gy + GAP_H, PIPE_W - 2, H - gy - GAP_H - 4, pipec);
        fb_fill(fb, px, gy + GAP_H, PIPE_W, 4, pipe2);
    }

    int bx = g->bx, by = (int)g->by;
    fb_fill(fb, bx, by, BIRD_W, BIRD_H, bird);
    fb_fill(fb, bx + BIRD_W - 1, by + 1, 2, 2, beak);
    fb_pixel(fb, bx + BIRD_W - 3, by + 1, black);

    char s[16], b[16];
    snprintf(s, sizeof(s), "%d", g->score);
    snprintf(b, sizeof(b), "BEST %d", g->best);
    int sw = (int)strlen(s) * 16;
    fb_text(fb, (W - sw) / 2, 4, s, white, 2);
    int bw = (int)strlen(b) * 8;
    fb_text(fb, W - bw - 4, 4, b, white, 1);

    if (!g->alive) {
        const char *m1 = "GAME OVER";
        const char *m2 = "R restart  ESC quit";
        int w1 = (int)strlen(m1) * 16;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 14, m1, white, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 8, m2, white, 1);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device found\n");

    flap_t g;
    memset(&g, 0, sizeof(g));
    flap_init(&g, (int)fb.vinfo.xres, (int)fb.vinfo.yres, 0);

    input_state_t in;
    memset(&in, 0, sizeof(in));

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    while (running) {
        input_update(kbd, &in);
        if (in.pressed & KBIT_ESC) running = 0;
        if (in.pressed & KBIT_R) flap_init(&g, g.W, g.H, 1);

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
