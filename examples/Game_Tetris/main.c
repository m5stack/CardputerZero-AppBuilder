#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "fb.h"
#include "input.h"

#define BW       10
#define BH       17
#define CELL     8
#define FRAME_NS (16 * 1000 * 1000L)

/* 7 tetrominos, 4 rotations, 4 blocks each as (x,y) offsets. */
static const int8_t TETRO[7][4][4][2] = {
    /* I */ {{{0,1},{1,1},{2,1},{3,1}},
             {{2,0},{2,1},{2,2},{2,3}},
             {{0,2},{1,2},{2,2},{3,2}},
             {{1,0},{1,1},{1,2},{1,3}}},
    /* O */ {{{1,0},{2,0},{1,1},{2,1}},
             {{1,0},{2,0},{1,1},{2,1}},
             {{1,0},{2,0},{1,1},{2,1}},
             {{1,0},{2,0},{1,1},{2,1}}},
    /* T */ {{{1,0},{0,1},{1,1},{2,1}},
             {{1,0},{1,1},{2,1},{1,2}},
             {{0,1},{1,1},{2,1},{1,2}},
             {{1,0},{0,1},{1,1},{1,2}}},
    /* S */ {{{1,0},{2,0},{0,1},{1,1}},
             {{1,0},{1,1},{2,1},{2,2}},
             {{1,1},{2,1},{0,2},{1,2}},
             {{0,0},{0,1},{1,1},{1,2}}},
    /* Z */ {{{0,0},{1,0},{1,1},{2,1}},
             {{2,0},{1,1},{2,1},{1,2}},
             {{0,1},{1,1},{1,2},{2,2}},
             {{1,0},{0,1},{1,1},{0,2}}},
    /* J */ {{{0,0},{0,1},{1,1},{2,1}},
             {{1,0},{2,0},{1,1},{1,2}},
             {{0,1},{1,1},{2,1},{2,2}},
             {{1,0},{1,1},{0,2},{1,2}}},
    /* L */ {{{2,0},{0,1},{1,1},{2,1}},
             {{1,0},{1,1},{1,2},{2,2}},
             {{0,1},{1,1},{2,1},{0,2}},
             {{0,0},{1,0},{1,1},{1,2}}},
};

typedef struct {
    int board[BH][BW];   /* 0 empty else color index 1..7 */
    int piece;           /* current piece 0..6 */
    int rot;
    int px, py;
    int nextp;
    int score;
    int level;
    int lines;
    int gameover;
    uint32_t rng;
    int drop_frames;
    int frame_ctr;
} game_t;

static uint32_t xorshift32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

static int piece_collides(const game_t *g, int piece, int rot, int px, int py) {
    for (int i = 0; i < 4; i++) {
        int x = px + TETRO[piece][rot][i][0];
        int y = py + TETRO[piece][rot][i][1];
        if (x < 0 || x >= BW || y >= BH) return 1;
        if (y < 0) continue;
        if (g->board[y][x]) return 1;
    }
    return 0;
}

static void spawn(game_t *g) {
    g->piece = g->nextp;
    g->nextp = (int)(xorshift32(&g->rng) % 7);
    g->rot = 0;
    g->px = BW / 2 - 2;
    g->py = -1;
    if (piece_collides(g, g->piece, g->rot, g->px, g->py)) g->gameover = 1;
}

static void lock_piece(game_t *g) {
    for (int i = 0; i < 4; i++) {
        int x = g->px + TETRO[g->piece][g->rot][i][0];
        int y = g->py + TETRO[g->piece][g->rot][i][1];
        if (y >= 0 && y < BH && x >= 0 && x < BW)
            g->board[y][x] = g->piece + 1;
    }
}

static void clear_lines(game_t *g) {
    int cleared = 0;
    for (int y = BH - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BW; x++) if (!g->board[y][x]) { full = 0; break; }
        if (full) {
            cleared++;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < BW; x++)
                    g->board[yy][x] = g->board[yy-1][x];
            for (int x = 0; x < BW; x++) g->board[0][x] = 0;
            y++;
        }
    }
    if (cleared) {
        static const int pts[5] = {0, 40, 100, 300, 1200};
        g->score += pts[cleared] * (g->level + 1);
        g->lines += cleared;
        g->level = g->lines / 10;
        g->drop_frames = 48 - g->level * 4;
        if (g->drop_frames < 4) g->drop_frames = 4;
    }
}

static void game_init(game_t *g) {
    memset(g, 0, sizeof(*g));
    g->rng = (uint32_t)time(NULL) ^ 0xC0FFEE77u;
    if (!g->rng) g->rng = 1;
    g->nextp = (int)(xorshift32(&g->rng) % 7);
    g->level = 0;
    g->drop_frames = 48;
    spawn(g);
}

static void try_move(game_t *g, int dx, int dy) {
    if (g->gameover) return;
    if (!piece_collides(g, g->piece, g->rot, g->px + dx, g->py + dy)) {
        g->px += dx; g->py += dy;
    } else if (dy > 0) {
        lock_piece(g);
        clear_lines(g);
        spawn(g);
    }
}

static void try_rotate(game_t *g) {
    if (g->gameover) return;
    int nr = (g->rot + 1) & 3;
    if (!piece_collides(g, g->piece, nr, g->px, g->py)) {
        g->rot = nr;
        return;
    }
    /* try small kicks */
    for (int dx = -1; dx <= 1; dx += 2) {
        if (!piece_collides(g, g->piece, nr, g->px + dx, g->py)) {
            g->rot = nr; g->px += dx; return;
        }
    }
}

static void hard_drop(game_t *g) {
    if (g->gameover) return;
    while (!piece_collides(g, g->piece, g->rot, g->px, g->py + 1)) {
        g->py++; g->score += 2;
    }
    lock_piece(g);
    clear_lines(g);
    spawn(g);
}

static uint32_t piece_color(fb_t *fb, int idx) {
    /* idx 1..7 = I,O,T,S,Z,J,L */
    switch (idx) {
        case 1: return fb_rgb(fb, 0x00, 0xE0, 0xE0); /* I cyan */
        case 2: return fb_rgb(fb, 0xE0, 0xE0, 0x00); /* O yellow */
        case 3: return fb_rgb(fb, 0xB0, 0x30, 0xE0); /* T purple */
        case 4: return fb_rgb(fb, 0x30, 0xD0, 0x30); /* S green */
        case 5: return fb_rgb(fb, 0xE0, 0x30, 0x30); /* Z red */
        case 6: return fb_rgb(fb, 0x30, 0x60, 0xE0); /* J blue */
        case 7: return fb_rgb(fb, 0xE0, 0x90, 0x20); /* L orange */
    }
    return 0;
}

static void draw_cell(fb_t *fb, int x, int y, uint32_t c) {
    fb_fill(fb, x, y, CELL, CELL, c);
    /* 1 px inner highlight */
    uint32_t lite = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    fb_fill(fb, x + 1, y + 1, CELL - 2, 1, lite);
    fb_fill(fb, x + 1, y + 1, 1, CELL - 2, lite);
}

static void render(fb_t *fb, const game_t *g) {
    uint32_t bg     = fb_rgb(fb, 0x08, 0x0A, 0x14);
    uint32_t border = fb_rgb(fb, 0x50, 0x50, 0x60);
    uint32_t white  = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t dim    = fb_rgb(fb, 0xA0, 0xA0, 0xA0);

    int W = fb->vinfo.xres, H = fb->vinfo.yres;
    int boardw = BW * CELL, boardh = BH * CELL;
    int bx = (W - boardw) / 2 - 40;
    if (bx < 2) bx = 2;
    int by = (H - boardh) / 2;
    if (by < 1) by = 1;

    fb_clear(fb, bg);

    /* board frame */
    fb_fill(fb, bx - 1, by - 1, boardw + 2, 1, border);
    fb_fill(fb, bx - 1, by + boardh, boardw + 2, 1, border);
    fb_fill(fb, bx - 1, by - 1, 1, boardh + 2, border);
    fb_fill(fb, bx + boardw, by - 1, 1, boardh + 2, border);

    /* locked cells */
    for (int y = 0; y < BH; y++)
        for (int x = 0; x < BW; x++) {
            int v = g->board[y][x];
            if (v) draw_cell(fb, bx + x * CELL, by + y * CELL, piece_color(fb, v));
        }

    /* current piece */
    if (!g->gameover) {
        uint32_t pc = piece_color(fb, g->piece + 1);
        for (int i = 0; i < 4; i++) {
            int x = g->px + TETRO[g->piece][g->rot][i][0];
            int y = g->py + TETRO[g->piece][g->rot][i][1];
            if (y < 0) continue;
            draw_cell(fb, bx + x * CELL, by + y * CELL, pc);
        }
    }

    /* HUD */
    int hx = bx + boardw + 8;
    fb_text(fb, hx, by + 2, "SCORE", dim, 1);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", g->score);
    fb_text(fb, hx, by + 12, buf, white, 1);

    fb_text(fb, hx, by + 28, "LEVEL", dim, 1);
    snprintf(buf, sizeof(buf), "%d", g->level);
    fb_text(fb, hx, by + 38, buf, white, 1);

    fb_text(fb, hx, by + 54, "LINES", dim, 1);
    snprintf(buf, sizeof(buf), "%d", g->lines);
    fb_text(fb, hx, by + 64, buf, white, 1);

    fb_text(fb, hx, by + 80, "NEXT", dim, 1);
    /* draw next piece small */
    int nx = hx, ny = by + 90;
    uint32_t nc = piece_color(fb, g->nextp + 1);
    for (int i = 0; i < 4; i++) {
        int x = TETRO[g->nextp][0][i][0];
        int y = TETRO[g->nextp][0][i][1];
        fb_fill(fb, nx + x * 6, ny + y * 6, 5, 5, nc);
    }

    if (g->gameover) {
        const char *m1 = "GAME OVER";
        const char *m2 = "R restart";
        int w1 = (int)strlen(m1) * 8 * 2;
        int w2 = (int)strlen(m2) * 8;
        fb_text(fb, (W - w1) / 2, H / 2 - 10, m1, white, 2);
        fb_text(fb, (W - w2) / 2, H / 2 + 10, m2, dim, 1);
    }
}

int main(void) {
    fb_t fb;
    if (fb_open(&fb, "/dev/fb0") < 0) return 1;
    int kbd = input_open();
    if (kbd < 0) fprintf(stderr, "warning: no input device\n");

    game_t g;
    game_init(&g);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int running = 1;
    while (running) {
        input_key_t k = input_poll(kbd);
        switch (k) {
            case INPUT_LEFT:    try_move(&g, -1, 0); break;
            case INPUT_RIGHT:   try_move(&g,  1, 0); break;
            case INPUT_DOWN:    try_move(&g,  0, 1); g.frame_ctr = 0; break;
            case INPUT_ROTATE:  try_rotate(&g); break;
            case INPUT_DROP:    hard_drop(&g); break;
            case INPUT_QUIT:    running = 0; break;
            case INPUT_RESTART: if (g.gameover) game_init(&g); break;
            default: break;
        }

        if (!g.gameover) {
            g.frame_ctr++;
            if (g.frame_ctr >= g.drop_frames) {
                g.frame_ctr = 0;
                try_move(&g, 0, 1);
            }
        }

        render(&fb, &g);

        next.tv_nsec += FRAME_NS;
        while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    input_close(kbd);
    fb_close(&fb);
    return 0;
}
