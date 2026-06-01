#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define SCREEN_W 320
#define SCREEN_H 170
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define TICK_MS 16

#define CELL 40
#define GRID_W (CELL * 3)
#define GRID_H (CELL * 3)
#define STATUS_H 20

#define EMPTY 0
#define P_X 1
#define P_O 2

typedef enum {
    PHASE_HUMAN,
    PHASE_CPU,
    PHASE_END
} Phase;

typedef struct {
    int board[9];
    int cursor;
    Phase phase;
    int winner;
    bool draw;
    float cpu_delay;
    int win_line[3];
} Game;

static int lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
};

static int check_winner(const int b[9], int line_out[3]) {
    for (int i = 0; i < 8; i++) {
        int a = b[lines[i][0]], c = b[lines[i][1]], d = b[lines[i][2]];
        if (a != EMPTY && a == c && c == d) {
            if (line_out) { line_out[0] = lines[i][0]; line_out[1] = lines[i][1]; line_out[2] = lines[i][2]; }
            return a;
        }
    }
    return 0;
}

static bool is_full(const int b[9]) {
    for (int i = 0; i < 9; i++) if (b[i] == EMPTY) return false;
    return true;
}

static int minimax(int b[9], int player, int depth) {
    int w = check_winner(b, NULL);
    if (w == P_O) return 10 - depth;
    if (w == P_X) return depth - 10;
    if (is_full(b)) return 0;

    if (player == P_O) {
        int best = -1000;
        for (int i = 0; i < 9; i++) {
            if (b[i] != EMPTY) continue;
            b[i] = P_O;
            int s = minimax(b, P_X, depth + 1);
            b[i] = EMPTY;
            if (s > best) best = s;
        }
        return best;
    } else {
        int best = 1000;
        for (int i = 0; i < 9; i++) {
            if (b[i] != EMPTY) continue;
            b[i] = P_X;
            int s = minimax(b, P_O, depth + 1);
            b[i] = EMPTY;
            if (s < best) best = s;
        }
        return best;
    }
}

static int cpu_choose(int b[9]) {
    int empties = 0;
    for (int i = 0; i < 9; i++) if (b[i] == EMPTY) empties++;
    if (empties == 9) return rand() % 9;

    int best_score = -1000;
    int best_idx = -1;
    for (int i = 0; i < 9; i++) {
        if (b[i] != EMPTY) continue;
        b[i] = P_O;
        int s = minimax(b, P_X, 0);
        b[i] = EMPTY;
        if (s > best_score) { best_score = s; best_idx = i; }
    }
    return best_idx;
}

static void game_reset(Game *g) {
    memset(g, 0, sizeof(*g));
    g->cursor = 4;
    g->phase = PHASE_HUMAN;
    g->winner = 0;
    g->draw = false;
    g->cpu_delay = 0;
    for (int i = 0; i < 3; i++) g->win_line[i] = -1;
}

static void check_end(Game *g) {
    int line[3];
    int w = check_winner(g->board, line);
    if (w) {
        g->winner = w;
        g->phase = PHASE_END;
        memcpy(g->win_line, line, sizeof(line));
        return;
    }
    if (is_full(g->board)) {
        g->draw = true;
        g->phase = PHASE_END;
    }
}

static void human_place(Game *g) {
    if (g->phase != PHASE_HUMAN) return;
    if (g->board[g->cursor] != EMPTY) return;
    g->board[g->cursor] = P_X;
    check_end(g);
    if (g->phase == PHASE_HUMAN) {
        g->phase = PHASE_CPU;
        g->cpu_delay = 0.5f;
    }
}

static void cpu_turn(Game *g) {
    int idx = cpu_choose(g->board);
    if (idx >= 0) g->board[idx] = P_O;
    check_end(g);
    if (g->phase == PHASE_CPU) g->phase = PHASE_HUMAN;
}

static void move_cursor(Game *g, int dx, int dy) {
    int cx = g->cursor % 3;
    int cy = g->cursor / 3;
    cx += dx; cy += dy;
    if (cx < 0) cx = 2; if (cx > 2) cx = 0;
    if (cy < 0) cy = 2; if (cy > 2) cy = 0;
    g->cursor = cy * 3 + cx;
}

static int grid_x0(void) { return (SCREEN_W - GRID_W) / 2; }
static int grid_y0(void) { return STATUS_H + (SCREEN_H - STATUS_H - GRID_H) / 2; }

static void draw_line(SDL_Renderer *ren, int x1, int y1, int x2, int y2, Uint8 r, Uint8 gg, Uint8 b) {
    SDL_SetRenderDrawColor(ren, r, gg, b, 255);
    SDL_RenderDrawLine(ren, x1, y1, x2, y2);
}

static void draw_rect_outline(SDL_Renderer *ren, int x, int y, int w, int h, Uint8 r, Uint8 gg, Uint8 b) {
    SDL_SetRenderDrawColor(ren, r, gg, b, 255);
    SDL_Rect rr = { x, y, w, h };
    SDL_RenderDrawRect(ren, &rr);
}

static void draw_x(SDL_Renderer *ren, int cx, int cy, int sz) {
    int p = 6;
    draw_line(ren, cx - sz/2 + p, cy - sz/2 + p, cx + sz/2 - p, cy + sz/2 - p, 220, 120, 120);
    draw_line(ren, cx + sz/2 - p, cy - sz/2 + p, cx - sz/2 + p, cy + sz/2 - p, 220, 120, 120);
    draw_line(ren, cx - sz/2 + p + 1, cy - sz/2 + p, cx + sz/2 - p + 1, cy + sz/2 - p, 220, 120, 120);
    draw_line(ren, cx + sz/2 - p - 1, cy - sz/2 + p, cx - sz/2 + p - 1, cy + sz/2 - p, 220, 120, 120);
}

static void draw_o(SDL_Renderer *ren, int cx, int cy, int sz) {
    int r = sz/2 - 6;
    SDL_SetRenderDrawColor(ren, 120, 180, 220, 255);
    int steps = 48;
    float prev_x = cx + r, prev_y = cy;
    for (int i = 1; i <= steps; i++) {
        float a = (2.0f * 3.14159265f * i) / steps;
        float nx = cx + r * cosf(a);
        float ny = cy + r * sinf(a);
        SDL_RenderDrawLine(ren, (int)prev_x, (int)prev_y, (int)nx, (int)ny);
        prev_x = nx; prev_y = ny;
    }
}

static void draw_text(SDL_Renderer *ren, TTF_Font *font, int x, int y, const char *text, SDL_Color c) {
    if (!font || !text || !*text) return;
    SDL_Surface *s = TTF_RenderUTF8_Solid(font, text, c);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    if (t) {
        SDL_Rect dst = { x, y, s->w, s->h };
        SDL_RenderCopy(ren, t, NULL, &dst);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

static void render(SDL_Renderer *ren, TTF_Font *font, Game *g) {
    SDL_SetRenderDrawColor(ren, 16, 16, 24, 255);
    SDL_RenderClear(ren);

    const char *status = "Your turn (X)";
    SDL_Color sc = { 230, 230, 230, 255 };
    if (g->phase == PHASE_CPU) { status = "CPU thinking..."; sc.r = 200; sc.g = 200; sc.b = 120; }
    else if (g->phase == PHASE_END) {
        if (g->winner == P_X) { status = "You won! (R)"; sc.r = 120; sc.g = 220; sc.b = 120; }
        else if (g->winner == P_O) { status = "CPU wins (R)"; sc.r = 220; sc.g = 120; sc.b = 120; }
        else if (g->draw) { status = "Draw (R)"; sc.r = 200; sc.g = 200; sc.b = 200; }
    }
    draw_text(ren, font, 4, 4, status, sc);

    int x0 = grid_x0(), y0 = grid_y0();
    for (int i = 0; i <= 3; i++) {
        draw_line(ren, x0 + i * CELL, y0, x0 + i * CELL, y0 + GRID_H, 200, 200, 200);
        draw_line(ren, x0, y0 + i * CELL, x0 + GRID_W, y0 + i * CELL, 200, 200, 200);
    }

    for (int i = 0; i < 9; i++) {
        int cx = x0 + (i % 3) * CELL + CELL / 2;
        int cy = y0 + (i / 3) * CELL + CELL / 2;
        if (g->board[i] == P_X) draw_x(ren, cx, cy, CELL);
        else if (g->board[i] == P_O) draw_o(ren, cx, cy, CELL);
    }

    if (g->phase == PHASE_HUMAN) {
        int cx = x0 + (g->cursor % 3) * CELL;
        int cy = y0 + (g->cursor / 3) * CELL;
        draw_rect_outline(ren, cx + 1, cy + 1, CELL - 2, CELL - 2, 255, 220, 100);
        draw_rect_outline(ren, cx + 2, cy + 2, CELL - 4, CELL - 4, 255, 220, 100);
    }

    if (g->phase == PHASE_END && g->winner) {
        int a = g->win_line[0], b = g->win_line[2];
        int ax = x0 + (a % 3) * CELL + CELL / 2;
        int ay = y0 + (a / 3) * CELL + CELL / 2;
        int bx = x0 + (b % 3) * CELL + CELL / 2;
        int by = y0 + (b / 3) * CELL + CELL / 2;
        draw_line(ren, ax, ay, bx, by, 255, 255, 80);
        draw_line(ren, ax + 1, ay, bx + 1, by, 255, 255, 80);
    }

    SDL_RenderPresent(ren);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)SDL_GetTicks() ^ 0xCAFE);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    bool have_ttf = (TTF_Init() == 0);
    if (!have_ttf) fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());

    SDL_Window *win = SDL_CreateWindow("TicTacToe",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_BORDERLESS);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    TTF_Font *font = NULL;
    if (have_ttf) {
        font = TTF_OpenFont(FONT_PATH, 10);
        if (!font) fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
    }

    Game g;
    game_reset(&g);

    int running = 1;
    Uint32 last = SDL_GetTicks();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) { running = 0; continue; }
                if (k == SDLK_r) { game_reset(&g); continue; }
                if (g.phase == PHASE_HUMAN) {
                    if (k == SDLK_LEFT)  move_cursor(&g, -1, 0);
                    if (k == SDLK_RIGHT) move_cursor(&g, 1, 0);
                    if (k == SDLK_UP)    move_cursor(&g, 0, -1);
                    if (k == SDLK_DOWN)  move_cursor(&g, 0, 1);
                    if (k == SDLK_SPACE || k == SDLK_RETURN || k == SDLK_KP_ENTER)
                        human_place(&g);
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;

        if (g.phase == PHASE_CPU) {
            g.cpu_delay -= dt;
            if (g.cpu_delay <= 0) cpu_turn(&g);
        }

        render(ren, font, &g);

        Uint32 elapsed = SDL_GetTicks() - now;
        if (elapsed < TICK_MS) SDL_Delay(TICK_MS - elapsed);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    if (have_ttf) TTF_Quit();
    SDL_Quit();
    return 0;
}
