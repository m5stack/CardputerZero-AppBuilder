#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 170
#define ROWS 5
#define COLS 8
#define INV_W 16
#define INV_H 10
#define INV_GAP_X 4
#define INV_GAP_Y 4
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define TICK_MS 16
#define TICK_SEC (TICK_MS / 1000.0f)

typedef struct {
    SDL_FRect r;
    bool alive;
} Invader;

typedef struct {
    SDL_FRect r;
    bool alive;
    float vy;
} Bullet;

typedef struct {
    SDL_FRect ship;
    Invader invs[ROWS * COLS];
    Bullet pbullet;
    Bullet ebullet;
    float inv_vx;
    float inv_step_y;
    float inv_move_accum;
    float inv_move_interval;
    float fire_accum;
    int lives;
    int score;
    int alive_count;
    bool key_left;
    bool key_right;
    bool game_over;
    bool win;
} Game;

static void init_invaders(Game *g) {
    int ox = 16, oy = 18;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            Invader *v = &g->invs[r * COLS + c];
            v->r.x = ox + c * (INV_W + INV_GAP_X);
            v->r.y = oy + r * (INV_H + INV_GAP_Y);
            v->r.w = INV_W;
            v->r.h = INV_H;
            v->alive = true;
        }
    }
    g->alive_count = ROWS * COLS;
}

static void game_reset(Game *g, bool full) {
    g->ship.w = 18;
    g->ship.h = 6;
    g->ship.x = (SCREEN_W - g->ship.w) / 2.0f;
    g->ship.y = SCREEN_H - 14;
    g->pbullet.alive = false;
    g->ebullet.alive = false;
    g->inv_vx = 8.0f;
    g->inv_step_y = 4.0f;
    g->inv_move_accum = 0;
    g->inv_move_interval = 0.6f;
    g->fire_accum = 0;
    g->key_left = g->key_right = false;
    g->game_over = false;
    g->win = false;
    if (full) {
        g->lives = 3;
        g->score = 0;
    }
    init_invaders(g);
}

static void fire_player(Game *g) {
    if (g->pbullet.alive) return;
    g->pbullet.r.w = 2;
    g->pbullet.r.h = 6;
    g->pbullet.r.x = g->ship.x + g->ship.w / 2.0f - 1;
    g->pbullet.r.y = g->ship.y - 7;
    g->pbullet.vy = -180.0f;
    g->pbullet.alive = true;
}

static int rand_range(int a, int b) {
    return a + rand() % (b - a + 1);
}

static void maybe_enemy_fire(Game *g) {
    if (g->ebullet.alive) return;
    if (g->alive_count == 0) return;
    if (g->fire_accum < 0.7f) return;
    g->fire_accum = 0;
    if ((rand() % 3) != 0) return;
    int pick = rand_range(0, g->alive_count - 1);
    int idx = 0;
    for (int c = 0; c < COLS; c++) {
        for (int r = ROWS - 1; r >= 0; r--) {
            Invader *v = &g->invs[r * COLS + c];
            if (!v->alive) continue;
            if (idx == pick) {
                g->ebullet.r.w = 2;
                g->ebullet.r.h = 6;
                g->ebullet.r.x = v->r.x + v->r.w / 2.0f - 1;
                g->ebullet.r.y = v->r.y + v->r.h;
                g->ebullet.vy = 90.0f;
                g->ebullet.alive = true;
                return;
            }
            idx++;
            break;
        }
    }
}

static bool overlap(const SDL_FRect *a, const SDL_FRect *b) {
    return !(a->x + a->w <= b->x || b->x + b->w <= a->x ||
             a->y + a->h <= b->y || b->y + b->h <= a->y);
}

static void game_update(Game *g, float dt) {
    if (g->game_over || g->win) return;

    float sp = 120.0f;
    if (g->key_left) g->ship.x -= sp * dt;
    if (g->key_right) g->ship.x += sp * dt;
    if (g->ship.x < 2) g->ship.x = 2;
    if (g->ship.x + g->ship.w > SCREEN_W - 2) g->ship.x = SCREEN_W - 2 - g->ship.w;

    if (g->pbullet.alive) {
        g->pbullet.r.y += g->pbullet.vy * dt;
        if (g->pbullet.r.y + g->pbullet.r.h < 0) g->pbullet.alive = false;
    }
    if (g->ebullet.alive) {
        g->ebullet.r.y += g->ebullet.vy * dt;
        if (g->ebullet.r.y > SCREEN_H) g->ebullet.alive = false;
    }

    g->inv_move_accum += dt;
    g->fire_accum += dt;
    float interval = g->inv_move_interval;
    if (g->alive_count < 10) interval *= 0.5f;
    else if (g->alive_count < 20) interval *= 0.7f;

    if (g->inv_move_accum >= interval) {
        g->inv_move_accum = 0;
        float min_x = 1e9, max_x = -1e9, max_y = -1e9;
        for (int i = 0; i < ROWS * COLS; i++) {
            if (!g->invs[i].alive) continue;
            if (g->invs[i].r.x < min_x) min_x = g->invs[i].r.x;
            if (g->invs[i].r.x + g->invs[i].r.w > max_x) max_x = g->invs[i].r.x + g->invs[i].r.w;
            if (g->invs[i].r.y + g->invs[i].r.h > max_y) max_y = g->invs[i].r.y + g->invs[i].r.h;
        }
        bool flip = false;
        float dx = g->inv_vx;
        if (dx > 0 && max_x + dx > SCREEN_W - 2) flip = true;
        if (dx < 0 && min_x + dx < 2) flip = true;
        if (flip) {
            g->inv_vx = -g->inv_vx;
            for (int i = 0; i < ROWS * COLS; i++) {
                if (g->invs[i].alive) g->invs[i].r.y += g->inv_step_y;
            }
        } else {
            for (int i = 0; i < ROWS * COLS; i++) {
                if (g->invs[i].alive) g->invs[i].r.x += dx;
            }
        }
        if (max_y + g->inv_step_y >= g->ship.y) g->game_over = true;
    }

    if (g->pbullet.alive) {
        for (int i = 0; i < ROWS * COLS; i++) {
            Invader *v = &g->invs[i];
            if (!v->alive) continue;
            if (overlap(&g->pbullet.r, &v->r)) {
                v->alive = false;
                g->pbullet.alive = false;
                g->score += 10;
                g->alive_count--;
                break;
            }
        }
    }

    if (g->ebullet.alive && overlap(&g->ebullet.r, &g->ship)) {
        g->ebullet.alive = false;
        g->lives--;
        if (g->lives <= 0) g->game_over = true;
    }

    if (g->alive_count == 0) g->win = true;

    maybe_enemy_fire(g);
}

static void draw_rect(SDL_Renderer *ren, float x, float y, float w, float h, Uint8 r, Uint8 gg, Uint8 b) {
    SDL_SetRenderDrawColor(ren, r, gg, b, 255);
    SDL_FRect fr = { x, y, w, h };
    SDL_RenderFillRectF(ren, &fr);
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
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    for (int i = 0; i < ROWS * COLS; i++) {
        if (!g->invs[i].alive) continue;
        int row = i / COLS;
        Uint8 r = 80 + row * 20, gg = 200 - row * 15, b = 80;
        draw_rect(ren, g->invs[i].r.x, g->invs[i].r.y, g->invs[i].r.w, g->invs[i].r.h, r, gg, b);
    }

    draw_rect(ren, g->ship.x, g->ship.y, g->ship.w, g->ship.h, 120, 220, 120);
    draw_rect(ren, g->ship.x + g->ship.w / 2 - 2, g->ship.y - 3, 4, 3, 120, 220, 120);

    if (g->pbullet.alive)
        draw_rect(ren, g->pbullet.r.x, g->pbullet.r.y, g->pbullet.r.w, g->pbullet.r.h, 255, 255, 255);
    if (g->ebullet.alive)
        draw_rect(ren, g->ebullet.r.x, g->ebullet.r.y, g->ebullet.r.w, g->ebullet.r.h, 255, 120, 120);

    SDL_Color white = { 230, 230, 230, 255 };
    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE %d", g->score);
    draw_text(ren, font, 2, 1, buf, white);
    snprintf(buf, sizeof(buf), "LIVES %d", g->lives);
    draw_text(ren, font, SCREEN_W - 64, 1, buf, white);

    if (g->game_over) {
        SDL_Color rc = { 255, 80, 80, 255 };
        draw_text(ren, font, SCREEN_W / 2 - 40, SCREEN_H / 2 - 10, "GAME OVER - R", rc);
    } else if (g->win) {
        SDL_Color gc = { 120, 255, 120, 255 };
        draw_text(ren, font, SCREEN_W / 2 - 24, SCREEN_H / 2 - 10, "YOU WIN - R", gc);
    }

    SDL_RenderPresent(ren);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)SDL_GetTicks() ^ 0xA5A5);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    bool have_ttf = (TTF_Init() == 0);
    if (!have_ttf) fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());

    SDL_Window *win = SDL_CreateWindow("Invaders",
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
    game_reset(&g, true);

    int running = 1;
    Uint32 last = SDL_GetTicks();
    float accum = 0.0f;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                bool down = (ev.type == SDL_KEYDOWN);
                SDL_Keycode k = ev.key.keysym.sym;
                if (down && k == SDLK_ESCAPE) { running = 0; continue; }
                if (down && k == SDLK_r) { game_reset(&g, true); continue; }
                if (k == SDLK_LEFT)  g.key_left  = down;
                if (k == SDLK_RIGHT) g.key_right = down;
                if (down && k == SDLK_SPACE) fire_player(&g);
            }
        }

        Uint32 now = SDL_GetTicks();
        float frame_dt = (now - last) / 1000.0f;
        last = now;
        if (frame_dt > 0.25f) frame_dt = 0.25f;
        accum += frame_dt;
        while (accum >= TICK_SEC) {
            game_update(&g, TICK_SEC);
            accum -= TICK_SEC;
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
