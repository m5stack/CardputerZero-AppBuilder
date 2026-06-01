#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "fb.h"
#include "input.h"

#define GW       10
#define GH       10
#define TILE     14
#define MAX_UNDO 256
#define FRAME_NS (33 * 1000 * 1000L)

/* Tile codes used internally. Walls/targets are static; player/box are dynamic. */
enum {
    T_FLOOR = 0,
    T_WALL  = 1,
    T_TARGET= 2
};

/* 3 hand-crafted 10x10 levels. Each line is 10 chars, 10 lines. */
static const char *LEVELS[3][GH] = {
    /* L1: single box, single target, open room — trivial warm-up. */
    {
        "##########",
        "#        #",
        "#        #",
        "#   .    #",
        "#        #",
        "#   $    #",
        "#        #",
        "#   @    #",
        "#        #",
        "##########",
    },
    /* L2: two boxes, two targets. Push east and south. */
    {
        "##########",
        "#        #",
        "#  .  .  #",
        "#        #",
        "#  $  $  #",
        "#        #",
        "#        #",
        "#   @    #",
        "#        #",
        "##########",
    },
    /* L3: three boxes in a row to push north onto three targets. */
    {
        "##########",
        "#        #",
        "# . . .  #",
        "#        #",
        "#        #",
        "# $ $ $  #",
        "#        #",
        "#        #",
        "#   @    #",
        "##########",
    },
};

typedef struct {
    uint8_t  wall[GH][GW];
    uint8_t  target[GH][GW];
    uint8_t  box[GH][GW];
    int      px, py;
    int      moves;
    int      pushes;
    int      solved;
    int      level;
    /* undo ring */
    struct { int px, py; int bx, by; int had_box; } hist[MAX_UNDO];
    int hist_n;
    time_t solve_time;
} game_t;

static int fix_second_player(char *row, int *px, int *py, int y) {
    /* level 1 above uses "@@" at center to mark player; pick leftmost @ as player,
       replace extra '@' with floor. */
    int found = -1;
    for (int x = 0; x < GW; x++) {
        if (row[x] == '@' || row[x] == '+') {
            if (found < 0) { found = x; }
            else { row[x] = ' '; }
        }
    }
    if (found >= 0) { *px = found; *py = y; }
    return found;
}

static void load_level(game_t *g, int idx) {
    memset(g->wall, 0, sizeof(g->wall));
    memset(g->target, 0, sizeof(g->target));
    memset(g->box, 0, sizeof(g->box));
    g->moves = 0;
    g->pushes = 0;
    g->solved = 0;
    g->hist_n = 0;
    g->level = idx;
    g->px = g->py = 0;
    char tmp[GH][GW + 1];
    for (int y = 0; y < GH; y++) {
        /* copy and clamp to GW chars */
        const char *src = LEVELS[idx][y];
        memset(tmp[y], ' ', GW);
        tmp[y][GW] = 0;
        for (int x = 0; x < GW && src[x]; x++) tmp[y][x] = src[x];
    }
    for (int y = 0; y < GH; y++) {
        fix_second_player(tmp[y], &g->px, &g->py, y);
    }
    for (int y = 0; y < GH; y++) {
        for (int x = 0; x < GW; x++) {
            char c = tmp[y][x];
            switch (c) {
                case '#': g->wall[y][x] = 1; break;
                case '.': g->target[y][x] = 1; break;
                case '$': g->box[y][x] = 1; break;
                case '*': g->box[y][x] = 1; g->target[y][x] = 1; break;
                case '@': g->px = x; g->py = y; break;
                case '+': g->px = x; g->py = y; g->target[y][x] = 1; break;
                default: break;
            }
        }
    }
}

static int check_solved(const game_t *g) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (g->target[y][x] && !g->box[y][x]) return 0;
    /* also ensure every box is on a target */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (g->box[y][x] && !g->target[y][x]) return 0;
    return 1;
}

static void push_hist(game_t *g, int bx, int by, int had_box) {
    int i = g->hist_n % MAX_UNDO;
    g->hist[i].px = g->px;
    g->hist[i].py = g->py;
    g->hist[i].bx = bx;
    g->hist[i].by = by;
    g->hist[i].had_box = had_box;
    g->hist_n++;
}

static void do_move(game_t *g, int dx, int dy) {
    if (g->solved) return;
    int nx = g->px + dx, ny = g->py + dy;
    if (nx < 0 || ny < 0 || nx >= GW || ny >= GH) return;
    if (g->wall[ny][nx]) return;
    if (g->box[ny][nx]) {
        int bx = nx + dx, by = ny + dy;
        if (bx < 0 || by < 0 || bx >= GW || by >= GH) return;
        if (g->wall[by][bx] || g->box[by][bx]) return;
        push_hist(g, nx, ny, 1);
        g->box[ny][nx] = 0;
        g->box[by][bx] = 1;
        g->px = nx; g->py = ny;
        g->moves++; g->pushes++;
    } else {
        push_hist(g, 0, 0, 0);
        g->px = nx; g->py = ny;
        g->moves++;
    }
    if (check_solved(g)) {
        g->solved = 1;
        g->solve_time = time(NULL);
    }
}

static void do_undo(game_t *g) {
    if (g->hist_n == 0 || g->solved) return;
    g->hist_n--;
    int i = g->hist_n % MAX_UNDO;
    int ppx = g->hist[i].px;
    int ppy = g->hist[i].py;
    if (g->hist[i].had_box) {
        int bx = g->hist[i].bx + (g->px - ppx);
        int by = g->hist[i].by + (g->py - ppy);
        if (bx >= 0 && by >= 0 && bx < GW && by < GH && g->box[by][bx]) {
            g->box[by][bx] = 0;
            g->box[g->hist[i].by][g->hist[i].bx] = 1;
        }
        g->pushes--;
    }
    g->px = ppx; g->py = ppy;
    g->moves--;
    if (g->moves < 0) g->moves = 0;
    if (g->pushes < 0) g->pushes = 0;
}

static void render(fb_t *fb, const game_t *g, int flash_clear) {
    uint32_t bg     = fb_rgb(fb, 0x10, 0x10, 0x18);
    uint32_t wall   = fb_rgb(fb, 0x60, 0x60, 0x70);
    uint32_t wall_lo= fb_rgb(fb, 0x30, 0x30, 0x38);
    uint32_t floor  = fb_rgb(fb, 0x20, 0x20, 0x28);
    uint32_t tgt    = fb_rgb(fb, 0x80, 0x20, 0x20);
    uint32_t box    = fb_rgb(fb, 0xB0, 0x70, 0x20);
    uint32_t box_ok = fb_rgb(fb, 0x40, 0xC0, 0x40);
    uint32_t player = fb_rgb(fb, 0x40, 0xA0, 0xFF);
    uint32_t white  = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t dim    = fb_rgb(fb, 0xA0, 0xA0, 0xA0);

    int W = fb->vinfo.xres, H = fb->vinfo.yres;
    int boardw = GW * TILE, boardh = GH * TILE;
    int bx = 4;
    int by = (H - boardh) / 2;
    if (by < 1) by = 1;

    fb_clear(fb, bg);

    for (int y = 0; y < GH; y++) {
        for (int x = 0; x < GW; x++) {
            int px = bx + x * TILE;
            int py = by + y * TILE;
            if (g->wall[y][x]) {
                fb_fill(fb, px, py, TILE, TILE, wall);
                fb_fill(fb, px, py + TILE - 2, TILE, 2, wall_lo);
                fb_fill(fb, px + TILE - 2, py, 2, TILE, wall_lo);
            } else {
                fb_fill(fb, px, py, TILE, TILE, floor);
                if (g->target[y][x])
                    fb_fill(fb, px + TILE/2 - 2, py + TILE/2 - 2, 4, 4, tgt);
            }
        }
    }
    for (int y = 0; y < GH; y++) {
        for (int x = 0; x < GW; x++) {
            if (!g->box[y][x]) continue;
            int px = bx + x * TILE;
            int py = by + y * TILE;
            uint32_t c = g->target[y][x] ? box_ok : box;
            fb_fill(fb, px + 2, py + 2, TILE - 4, TILE - 4, c);
            fb_fill(fb, px + 3, py + 3, TILE - 6, 1, white);
        }
    }
    /* player */
    {
        int px = bx + g->px * TILE;
        int py = by + g->py * TILE;
        fb_fill(fb, px + 3, py + 3, TILE - 6, TILE - 6, player);
        fb_fill(fb, px + 5, py + 5, 2, 2, white);
        fb_fill(fb, px + TILE - 7, py + 5, 2, 2, white);
    }

    /* HUD */
    int hx = bx + boardw + 8;
    char buf[32];
    snprintf(buf, sizeof(buf), "LVL %d/%d", g->level + 1, 3);
    fb_text(fb, hx, by + 2, buf, white, 1);
    fb_text(fb, hx, by + 18, "MOVES", dim, 1);
    snprintf(buf, sizeof(buf), "%d", g->moves);
    fb_text(fb, hx, by + 28, buf, white, 1);
    fb_text(fb, hx, by + 42, "PUSH", dim, 1);
    snprintf(buf, sizeof(buf), "%d", g->pushes);
    fb_text(fb, hx, by + 52, buf, white, 1);

    fb_text(fb, hx, by + 70, "U UNDO", dim, 1);
    fb_text(fb, hx, by + 80, "R RESET", dim, 1);
    fb_text(fb, hx, by + 90, "N NEXT", dim, 1);
    fb_text(fb, hx, by + 100, "P PREV", dim, 1);
    fb_text(fb, hx, by + 110, "Q QUIT", dim, 1);

    if (flash_clear) {
        const char *m = "LEVEL CLEAR";
        int w = (int)strlen(m) * 8 * 2;
        fb_fill(fb, (W - w) / 2 - 6, H / 2 - 12, w + 12, 24, fb_rgb(fb, 0x20, 0x40, 0x20));
        fb_text(fb, (W - w) / 2, H / 2 - 8, m, white, 2);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device\n");

    game_t g;
    memset(&g, 0, sizeof(g));
    load_level(&g, 0);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    int flash_left = 0;  /* frames remaining of "LEVEL CLEAR" banner */
    int auto_advance = 0;

    while (running) {
        input_key_t k = input_poll(kbd);
        if (!flash_left) {
            switch (k) {
                case INPUT_UP:    do_move(&g,  0, -1); break;
                case INPUT_DOWN:  do_move(&g,  0,  1); break;
                case INPUT_LEFT:  do_move(&g, -1,  0); break;
                case INPUT_RIGHT: do_move(&g,  1,  0); break;
                case INPUT_UNDO:  do_undo(&g); break;
                case INPUT_RESTART: load_level(&g, g.level); break;
                case INPUT_NEXT:  if (g.solved) { int n = (g.level + 1) % 3; load_level(&g, n); } break;
                case INPUT_PREV:  { int n = (g.level + 3 - 1) % 3; load_level(&g, n); } break;
                case INPUT_QUIT:  running = 0; break;
                default: break;
            }
            if (g.solved && !flash_left && !auto_advance) {
                flash_left = 30;  /* ~1 s at 30 fps */
                auto_advance = 1;
            }
        } else {
            if (k == INPUT_QUIT) running = 0;
        }

        render(&fb, &g, flash_left > 0);

        if (flash_left > 0) {
            flash_left--;
            if (flash_left == 0 && auto_advance) {
                int n = (g.level + 1) % 3;
                load_level(&g, n);
                auto_advance = 0;
            }
        }

        next.tv_nsec += FRAME_NS;
        while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    input_close(kbd);
    fb_close(&fb);
    return 0;
}
