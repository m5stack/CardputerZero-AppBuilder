#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "fb.h"
#include "input.h"

#define PADDLE_W    4
#define PADDLE_H    24
#define BALL_R      2
#define WIN_SCORE   10
#define TICK_NS     (16666667L) /* ~60 Hz */

typedef enum { ST_SERVE, ST_PLAY, ST_OVER } state_t;

typedef struct {
    int W, H;
    float bx, by;
    float vx, vy;
    int   lpy, rpy;
    int   ls, rs;
    state_t st;
    int   winner; /* 0 left, 1 right */
    uint32_t rng;
    int   serve_dir;
} pong_t;

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

static void reset_ball(pong_t *g) {
    g->bx = g->W / 2.0f;
    g->by = g->H / 2.0f;
    float ang = ((xs32(&g->rng) % 60) - 30) * 0.03f; /* ~±0.9 rad */
    float spd = 2.2f;
    g->vx = g->serve_dir * spd;
    g->vy = spd * ang;
    g->st = ST_SERVE;
}

static void pong_init(pong_t *g, int W, int H) {
    memset(g, 0, sizeof(*g));
    g->W = W; g->H = H;
    g->lpy = (H - PADDLE_H) / 2;
    g->rpy = (H - PADDLE_H) / 2;
    g->ls = 0; g->rs = 0;
    g->rng = (uint32_t)time(NULL) ^ 0xDEADBEEFu;
    if (!g->rng) g->rng = 1;
    g->serve_dir = (xs32(&g->rng) & 1) ? 1 : -1;
    reset_ball(g);
}

static int iclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void step(pong_t *g, const input_state_t *in) {
    if (g->st == ST_OVER) return;

    /* Left paddle: human */
    int speed = 3;
    if (in->held & KBIT_UP)   g->lpy -= speed;
    if (in->held & KBIT_DOWN) g->lpy += speed;
    g->lpy = iclamp(g->lpy, 1, g->H - 1 - PADDLE_H);

    /* Right paddle: simple AI */
    int center = g->rpy + PADDLE_H / 2;
    int target = (int)g->by;
    int aispd = 2;
    /* add a small lag: only pursue if ball moves toward AI */
    if (g->vx > 0 && target < center - 2) g->rpy -= aispd;
    else if (g->vx > 0 && target > center + 2) g->rpy += aispd;
    else if (g->vx <= 0) {
        /* drift toward center when idle */
        int mid = (g->H - PADDLE_H) / 2;
        if (g->rpy < mid) g->rpy += 1;
        else if (g->rpy > mid) g->rpy -= 1;
    }
    g->rpy = iclamp(g->rpy, 1, g->H - 1 - PADDLE_H);

    if (g->st == ST_SERVE) {
        if (in->pressed & KBIT_SPACE) g->st = ST_PLAY;
        return;
    }

    /* Move ball */
    g->bx += g->vx;
    g->by += g->vy;

    /* Top/bottom walls */
    if (g->by - BALL_R < 1) { g->by = 1 + BALL_R; g->vy = -g->vy; }
    if (g->by + BALL_R > g->H - 2) { g->by = g->H - 2 - BALL_R; g->vy = -g->vy; }

    /* Left paddle collision */
    if (g->bx - BALL_R <= 2 + PADDLE_W && g->vx < 0) {
        if (g->by >= g->lpy && g->by <= g->lpy + PADDLE_H) {
            g->vx = -g->vx;
            float hit = (g->by - (g->lpy + PADDLE_H / 2.0f)) / (PADDLE_H / 2.0f);
            g->vy += hit * 1.2f;
            g->bx = 2 + PADDLE_W + BALL_R + 1;
        }
    }
    /* Right paddle collision */
    if (g->bx + BALL_R >= g->W - 3 - PADDLE_W && g->vx > 0) {
        if (g->by >= g->rpy && g->by <= g->rpy + PADDLE_H) {
            g->vx = -g->vx;
            float hit = (g->by - (g->rpy + PADDLE_H / 2.0f)) / (PADDLE_H / 2.0f);
            g->vy += hit * 1.2f;
            g->bx = g->W - 3 - PADDLE_W - BALL_R - 1;
        }
    }

    /* Scoring */
    if (g->bx < 0) {
        g->rs++;
        g->serve_dir = -1;
        if (g->rs >= WIN_SCORE) { g->st = ST_OVER; g->winner = 1; }
        else reset_ball(g);
    } else if (g->bx > g->W) {
        g->ls++;
        g->serve_dir = 1;
        if (g->ls >= WIN_SCORE) { g->st = ST_OVER; g->winner = 0; }
        else reset_ball(g);
    }

    /* Clamp speed */
    if (g->vy > 3.5f) g->vy = 3.5f;
    if (g->vy < -3.5f) g->vy = -3.5f;
}

static void render(fb_t *fb, const pong_t *g) {
    uint32_t bg    = fb_rgb(fb, 0x08, 0x08, 0x10);
    uint32_t bord  = fb_rgb(fb, 0x50, 0x50, 0x60);
    uint32_t net   = fb_rgb(fb, 0x40, 0x40, 0x40);
    uint32_t white = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t ball  = fb_rgb(fb, 0xFF, 0xE0, 0x30);

    int W = g->W, H = g->H;
    fb_clear(fb, bg);
    for (int x = 0; x < W; x++) { fb_pixel(fb, x, 0, bord); fb_pixel(fb, x, H - 1, bord); }
    for (int y = 0; y < H; y++) { fb_pixel(fb, 0, y, bord); fb_pixel(fb, W - 1, y, bord); }

    for (int y = 4; y < H - 4; y += 6) fb_fill(fb, W / 2 - 1, y, 2, 3, net);

    fb_fill(fb, 2, g->lpy, PADDLE_W, PADDLE_H, white);
    fb_fill(fb, W - 2 - PADDLE_W, g->rpy, PADDLE_W, PADDLE_H, white);

    int bx = (int)g->bx, by = (int)g->by;
    fb_fill(fb, bx - BALL_R, by - BALL_R, BALL_R * 2 + 1, BALL_R * 2 + 1, ball);

    char ls[8], rs[8];
    snprintf(ls, sizeof(ls), "%d", g->ls);
    snprintf(rs, sizeof(rs), "%d", g->rs);
    fb_text(fb, 8, 4, ls, white, 2);
    int rw = (int)strlen(rs) * 16;
    fb_text(fb, W - rw - 8, 4, rs, white, 2);

    if (g->st == ST_SERVE) {
        const char *m = "SPACE TO SERVE";
        int w = (int)strlen(m) * 8;
        fb_text(fb, (W - w) / 2, H - 14, m, white, 1);
    } else if (g->st == ST_OVER) {
        const char *m1 = g->winner == 0 ? "PLAYER WINS" : "CPU WINS";
        const char *m2 = "SPACE restart  ESC quit";
        int w1 = (int)strlen(m1) * 16;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 14, m1, white, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 10, m2, white, 1);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device found\n");

    pong_t g;
    pong_init(&g, (int)fb.vinfo.xres, (int)fb.vinfo.yres);

    input_state_t in;
    memset(&in, 0, sizeof(in));

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    while (running) {
        input_update(kbd, &in);
        if (in.pressed & KBIT_ESC) { running = 0; }
        if (in.pressed & KBIT_R) pong_init(&g, g.W, g.H);
        if (g.st == ST_OVER && (in.pressed & KBIT_SPACE)) pong_init(&g, g.W, g.H);

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
