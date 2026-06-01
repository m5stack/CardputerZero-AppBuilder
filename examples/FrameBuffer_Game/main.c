#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "fb.h"
#include "input.h"

#define CELL      10
#define GRID_W    32
#define GRID_H    17
#define MAX_LEN   (GRID_W * GRID_H)
#define TICK_NS   (100 * 1000 * 1000L)

typedef struct { int x, y; } pt_t;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } dir_t;

typedef struct {
    pt_t body[MAX_LEN];
    int  length;
    dir_t dir;
    dir_t next_dir;
    pt_t food;
    int  score;
    int  alive;
    uint32_t rng;
} game_t;

static uint32_t xorshift32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static int occupied(const game_t *g, int x, int y) {
    for (int i = 0; i < g->length; i++)
        if (g->body[i].x == x && g->body[i].y == y) return 1;
    return 0;
}

static void spawn_food(game_t *g) {
    for (int tries = 0; tries < 256; tries++) {
        int x = xorshift32(&g->rng) % GRID_W;
        int y = xorshift32(&g->rng) % GRID_H;
        if (!occupied(g, x, y)) { g->food.x = x; g->food.y = y; return; }
    }
    g->food.x = 0; g->food.y = 0;
}

static void game_init(game_t *g) {
    memset(g, 0, sizeof(*g));
    g->rng = (uint32_t)time(NULL) ^ 0xA5A5A5A5u;
    if (!g->rng) g->rng = 1;
    g->length = 4;
    int cx = GRID_W / 2, cy = GRID_H / 2;
    for (int i = 0; i < g->length; i++) { g->body[i].x = cx - i; g->body[i].y = cy; }
    g->dir = DIR_RIGHT;
    g->next_dir = DIR_RIGHT;
    g->alive = 1;
    g->score = 0;
    spawn_food(g);
}

static void try_turn(game_t *g, dir_t nd) {
    if ((g->dir == DIR_UP    && nd == DIR_DOWN)  ||
        (g->dir == DIR_DOWN  && nd == DIR_UP)    ||
        (g->dir == DIR_LEFT  && nd == DIR_RIGHT) ||
        (g->dir == DIR_RIGHT && nd == DIR_LEFT)) return;
    g->next_dir = nd;
}

static void maybe_beep(void) {
    if (access("assets/beep.wav", F_OK) == 0) {
        int r = system("aplay -q assets/beep.wav &");
        (void)r;
    }
}

static void step(game_t *g) {
    if (!g->alive) return;
    g->dir = g->next_dir;
    int dx = 0, dy = 0;
    switch (g->dir) {
        case DIR_UP:    dy = -1; break;
        case DIR_DOWN:  dy =  1; break;
        case DIR_LEFT:  dx = -1; break;
        case DIR_RIGHT: dx =  1; break;
    }
    int nx = g->body[0].x + dx;
    int ny = g->body[0].y + dy;
    if (nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H) { g->alive = 0; return; }
    int ate = (nx == g->food.x && ny == g->food.y);
    int check_len = ate ? g->length : g->length - 1;
    for (int i = 0; i < check_len; i++)
        if (g->body[i].x == nx && g->body[i].y == ny) { g->alive = 0; return; }
    if (ate) {
        if (g->length < MAX_LEN) g->length++;
        g->score++;
        maybe_beep();
    }
    for (int i = g->length - 1; i > 0; i--) g->body[i] = g->body[i - 1];
    g->body[0].x = nx;
    g->body[0].y = ny;
    if (ate) spawn_food(g);
}

static void render(fb_t *fb, const game_t *g) {
    uint32_t bg     = fb_rgb(fb, 0x10, 0x10, 0x18);
    uint32_t wall   = fb_rgb(fb, 0x60, 0x60, 0x60);
    uint32_t snake  = fb_rgb(fb, 0x30, 0xD0, 0x50);
    uint32_t head   = fb_rgb(fb, 0x80, 0xFF, 0x80);
    uint32_t food   = fb_rgb(fb, 0xE0, 0x30, 0x30);
    uint32_t white  = fb_rgb(fb, 0xFF, 0xFF, 0xFF);

    fb_clear(fb, bg);
    int W = fb->vinfo.xres, H = fb->vinfo.yres;
    for (int x = 0; x < W; x++) { fb_pixel(fb, x, 0, wall); fb_pixel(fb, x, H - 1, wall); }
    for (int y = 0; y < H; y++) { fb_pixel(fb, 0, y, wall); fb_pixel(fb, W - 1, y, wall); }

    fb_fill(fb, g->food.x * CELL, g->food.y * CELL, CELL, CELL, food);
    for (int i = 0; i < g->length; i++) {
        uint32_t c = (i == 0) ? head : snake;
        fb_fill(fb, g->body[i].x * CELL + 1, g->body[i].y * CELL + 1, CELL - 2, CELL - 2, c);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", g->score);
    int w = (int)strlen(buf) * 8;
    fb_text(fb, W - w - 4, 3, buf, white, 1);

    if (!g->alive) {
        const char *m1 = "GAME OVER";
        const char *m2 = "SPACE restart  ESC quit";
        int w1 = (int)strlen(m1) * 8 * 2;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 12, m1, white, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 10, m2, white, 1);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    if (fb.vinfo.xres != 320 || fb.vinfo.yres != 170) {
        fprintf(stderr, "warning: expected 320x170, got %ux%u\n", fb.vinfo.xres, fb.vinfo.yres);
    }
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device found\n");

    game_t g;
    game_init(&g);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    while (running) {
        input_key_t k = input_poll(kbd);
        switch (k) {
            case INPUT_UP:    try_turn(&g, DIR_UP); break;
            case INPUT_DOWN:  try_turn(&g, DIR_DOWN); break;
            case INPUT_LEFT:  try_turn(&g, DIR_LEFT); break;
            case INPUT_RIGHT: try_turn(&g, DIR_RIGHT); break;
            case INPUT_QUIT:  running = 0; break;
            case INPUT_RESTART: if (!g.alive) game_init(&g); break;
            default: break;
        }

        step(&g);
        render(&fb, &g);

        next.tv_nsec += TICK_NS;
        while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    input_close(kbd);
    fb_close(&fb);
    return 0;
}
