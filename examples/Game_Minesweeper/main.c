#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "fb.h"
#include "input.h"

#define GW       20
#define GH       14
#define MINES    16
#define CELL     10
#define FRAME_NS (33 * 1000 * 1000L)   /* ~30 fps, minesweeper is slow */

enum { HIDDEN = 0, REVEALED = 1, FLAGGED = 2 };

typedef struct {
    uint8_t mine[GH][GW];
    uint8_t state[GH][GW];
    uint8_t num[GH][GW];
    uint8_t dirty[GH][GW];
    int cx, cy;
    int flags;
    int revealed;
    int gameover;  /* 0 play, 1 lost, 2 won */
    int started;
    time_t t_start;
    time_t t_end;
    uint32_t rng;
    int need_full_redraw;
    int last_cx, last_cy;
} game_t;

static uint32_t xorshift32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

static void compute_numbers(game_t *g) {
    for (int y = 0; y < GH; y++) {
        for (int x = 0; x < GW; x++) {
            if (g->mine[y][x]) { g->num[y][x] = 0; continue; }
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    if (!dx && !dy) continue;
                    int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= GW || yy >= GH) continue;
                    if (g->mine[yy][xx]) n++;
                }
            g->num[y][x] = (uint8_t)n;
        }
    }
}

static void place_mines(game_t *g, int sx, int sy) {
    int placed = 0;
    while (placed < MINES) {
        int x = (int)(xorshift32(&g->rng) % GW);
        int y = (int)(xorshift32(&g->rng) % GH);
        if (g->mine[y][x]) continue;
        /* keep first click safe: no mine on first cell or neighbors */
        if (x >= sx - 1 && x <= sx + 1 && y >= sy - 1 && y <= sy + 1) continue;
        g->mine[y][x] = 1;
        placed++;
    }
    compute_numbers(g);
}

static void mark_dirty_all(game_t *g) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            g->dirty[y][x] = 1;
    g->need_full_redraw = 1;
}

static void game_init(game_t *g) {
    memset(g, 0, sizeof(*g));
    g->rng = (uint32_t)time(NULL) ^ 0xBEEFC001u;
    if (!g->rng) g->rng = 1;
    g->cx = GW / 2;
    g->cy = GH / 2;
    g->last_cx = -1;
    g->last_cy = -1;
    mark_dirty_all(g);
}

static void flood(game_t *g, int x, int y) {
    if (x < 0 || y < 0 || x >= GW || y >= GH) return;
    if (g->state[y][x] == REVEALED || g->state[y][x] == FLAGGED) return;
    g->state[y][x] = REVEALED;
    g->dirty[y][x] = 1;
    g->revealed++;
    if (g->num[y][x] == 0 && !g->mine[y][x]) {
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (!dx && !dy) continue;
                flood(g, x + dx, y + dy);
            }
    }
}

static void reveal(game_t *g) {
    if (g->gameover) return;
    int x = g->cx, y = g->cy;
    if (g->state[y][x] == FLAGGED) return;
    if (!g->started) {
        place_mines(g, x, y);
        g->started = 1;
        g->t_start = time(NULL);
    }
    if (g->mine[y][x]) {
        g->state[y][x] = REVEALED;
        g->dirty[y][x] = 1;
        g->gameover = 1;
        g->t_end = time(NULL);
        /* reveal all mines */
        for (int yy = 0; yy < GH; yy++)
            for (int xx = 0; xx < GW; xx++)
                if (g->mine[yy][xx]) { g->state[yy][xx] = REVEALED; g->dirty[yy][xx] = 1; }
        return;
    }
    flood(g, x, y);
    if (g->revealed == GW * GH - MINES) {
        g->gameover = 2;
        g->t_end = time(NULL);
    }
}

static void flag_toggle(game_t *g) {
    if (g->gameover) return;
    int x = g->cx, y = g->cy;
    if (g->state[y][x] == REVEALED) return;
    if (g->state[y][x] == FLAGGED) {
        g->state[y][x] = HIDDEN;
        g->flags--;
    } else {
        g->state[y][x] = FLAGGED;
        g->flags++;
    }
    g->dirty[y][x] = 1;
}

static void move_cursor(game_t *g, int dx, int dy) {
    int nx = g->cx + dx, ny = g->cy + dy;
    if (nx < 0 || ny < 0 || nx >= GW || ny >= GH) return;
    g->dirty[g->cy][g->cx] = 1;
    g->cx = nx; g->cy = ny;
    g->dirty[ny][nx] = 1;
}

static uint32_t num_color(fb_t *fb, int n) {
    switch (n) {
        case 1: return fb_rgb(fb, 0x30, 0x60, 0xFF);
        case 2: return fb_rgb(fb, 0x20, 0xC0, 0x40);
        case 3: return fb_rgb(fb, 0xE0, 0x30, 0x30);
        case 4: return fb_rgb(fb, 0x20, 0x20, 0x80);
        case 5: return fb_rgb(fb, 0x80, 0x20, 0x20);
        case 6: return fb_rgb(fb, 0x20, 0x80, 0x80);
        case 7: return fb_rgb(fb, 0x10, 0x10, 0x10);
        case 8: return fb_rgb(fb, 0x60, 0x60, 0x60);
    }
    return fb_rgb(fb, 0xFF, 0xFF, 0xFF);
}

static void draw_cell(fb_t *fb, const game_t *g, int gx0, int gy0, int x, int y, int cursor) {
    int px = gx0 + x * CELL;
    int py = gy0 + y * CELL;
    uint32_t hidden_hi = fb_rgb(fb, 0xC0, 0xC0, 0xC0);
    uint32_t hidden_lo = fb_rgb(fb, 0x80, 0x80, 0x80);
    uint32_t revealed  = fb_rgb(fb, 0x40, 0x40, 0x48);
    uint32_t edge      = fb_rgb(fb, 0x20, 0x20, 0x28);
    uint32_t flag_col  = fb_rgb(fb, 0xE0, 0x30, 0x30);
    uint32_t mine_col  = fb_rgb(fb, 0x10, 0x10, 0x10);
    uint32_t cursor_c  = fb_rgb(fb, 0xFF, 0xE0, 0x20);

    uint8_t st = g->state[y][x];
    if (st == REVEALED) {
        fb_fill(fb, px, py, CELL, CELL, revealed);
        fb_fill(fb, px, py, CELL, 1, edge);
        fb_fill(fb, px, py, 1, CELL, edge);
        if (g->mine[y][x]) {
            fb_fill(fb, px + 2, py + 2, CELL - 4, CELL - 4, mine_col);
        } else if (g->num[y][x]) {
            char c = '0' + g->num[y][x];
            fb_char(fb, px + 1, py + 1, c, num_color(fb, g->num[y][x]), 1);
        }
    } else {
        fb_fill(fb, px, py, CELL, CELL, hidden_lo);
        fb_fill(fb, px, py, CELL, 1, hidden_hi);
        fb_fill(fb, px, py, 1, CELL, hidden_hi);
        if (st == FLAGGED) {
            fb_fill(fb, px + 3, py + 2, 2, CELL - 4, flag_col);
            fb_fill(fb, px + 2, py + 2, 4, 3, flag_col);
        }
    }
    if (cursor) {
        /* 1 px inset border */
        fb_fill(fb, px, py, CELL, 1, cursor_c);
        fb_fill(fb, px, py + CELL - 1, CELL, 1, cursor_c);
        fb_fill(fb, px, py, 1, CELL, cursor_c);
        fb_fill(fb, px + CELL - 1, py, 1, CELL, cursor_c);
    }
}

static void render(fb_t *fb, game_t *g) {
    int W = fb->vinfo.xres, H = fb->vinfo.yres;
    int gw = GW * CELL, gh = GH * CELL;
    int gx = 4;
    int gy = (H - gh) / 2;
    if (gy < 1) gy = 1;

    uint32_t bg    = fb_rgb(fb, 0x10, 0x14, 0x1C);
    uint32_t white = fb_rgb(fb, 0xFF, 0xFF, 0xFF);
    uint32_t dim   = fb_rgb(fb, 0xA0, 0xA0, 0xA0);
    uint32_t border= fb_rgb(fb, 0x50, 0x50, 0x60);

    if (g->need_full_redraw) {
        fb_clear(fb, bg);
        fb_fill(fb, gx - 1, gy - 1, gw + 2, 1, border);
        fb_fill(fb, gx - 1, gy + gh, gw + 2, 1, border);
        fb_fill(fb, gx - 1, gy - 1, 1, gh + 2, border);
        fb_fill(fb, gx + gw, gy - 1, 1, gh + 2, border);
        g->need_full_redraw = 0;
    }

    /* ensure prior cursor cell clears its highlight */
    if (g->last_cx >= 0 && (g->last_cx != g->cx || g->last_cy != g->cy))
        g->dirty[g->last_cy][g->last_cx] = 1;
    g->dirty[g->cy][g->cx] = 1;

    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (!g->dirty[y][x]) continue;
            int cur = (x == g->cx && y == g->cy);
            draw_cell(fb, g, gx, gy, x, y, cur);
            g->dirty[y][x] = 0;
        }
    g->last_cx = g->cx; g->last_cy = g->cy;

    /* HUD (right side) repainted each frame */
    int hx = gx + gw + 8;
    int hw = W - hx - 2;
    if (hw > 0)
        fb_fill(fb, hx, 0, hw, H, bg);

    char buf[32];
    time_t now = g->t_end ? g->t_end : (g->started ? time(NULL) : g->t_start);
    int elapsed = g->started ? (int)(now - g->t_start) : 0;
    if (elapsed > 999) elapsed = 999;

    fb_text(fb, hx, gy + 2, "MINES", dim, 1);
    snprintf(buf, sizeof(buf), "%d", MINES - g->flags);
    fb_text(fb, hx, gy + 12, buf, white, 1);

    fb_text(fb, hx, gy + 28, "TIME", dim, 1);
    snprintf(buf, sizeof(buf), "%d", elapsed);
    fb_text(fb, hx, gy + 38, buf, white, 1);

    const char *face;
    if (g->gameover == 1)      face = ":(";
    else if (g->gameover == 2) face = ":D";
    else                        face = ":)";
    fb_text(fb, hx, gy + 56, "FACE", dim, 1);
    fb_text(fb, hx, gy + 66, face, white, 2);

    fb_text(fb, hx, gy + 92, "SPACE OPEN", dim, 1);
    fb_text(fb, hx, gy + 102, "F FLAG", dim, 1);
    fb_text(fb, hx, gy + 112, "R RESET", dim, 1);

    if (g->gameover) {
        const char *m = (g->gameover == 2) ? "YOU WIN" : "BOOM";
        int w = (int)strlen(m) * 8 * 2;
        fb_fill(fb, (W - w) / 2 - 4, H - 22, w + 8, 20, fb_rgb(fb, 0x20, 0x20, 0x30));
        fb_text(fb, (W - w) / 2, H - 20, m, white, 2);
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
            case INPUT_UP:      move_cursor(&g, 0, -1); break;
            case INPUT_DOWN:    move_cursor(&g, 0,  1); break;
            case INPUT_LEFT:    move_cursor(&g, -1, 0); break;
            case INPUT_RIGHT:   move_cursor(&g,  1, 0); break;
            case INPUT_REVEAL:  reveal(&g); break;
            case INPUT_FLAG:    flag_toggle(&g); break;
            case INPUT_RESTART: game_init(&g); break;
            case INPUT_QUIT:    running = 0; break;
            default: break;
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
