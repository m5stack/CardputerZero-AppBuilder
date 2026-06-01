#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 170
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define TICK_MS 16
#define TICK_SEC (TICK_MS / 1000.0f)
#define MAX_AST 32
#define MAX_BUL 6

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float x, y;
    float vx, vy;
    float angle;
    bool alive;
    float respawn;
    float invul;
} Ship;

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    bool alive;
} Bullet;

typedef struct {
    float x, y;
    float vx, vy;
    int size;
    bool alive;
} Asteroid;

typedef struct {
    Ship ship;
    Bullet bullets[MAX_BUL];
    Asteroid asts[MAX_AST];
    int score;
    int lives;
    bool key_left, key_right, key_up;
    bool game_over;
    bool wave_cleared_timer;
    float next_wave;
    int wave;
} Game;

static float frand(float a, float b) {
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

static void spawn_asteroid(Game *g, float x, float y, int size) {
    for (int i = 0; i < MAX_AST; i++) {
        if (!g->asts[i].alive) {
            g->asts[i].x = x;
            g->asts[i].y = y;
            float speed = (size == 3) ? 20.0f : (size == 2) ? 35.0f : 55.0f;
            float ang = frand(0, 2 * M_PI);
            g->asts[i].vx = cosf(ang) * speed;
            g->asts[i].vy = sinf(ang) * speed;
            g->asts[i].size = size;
            g->asts[i].alive = true;
            return;
        }
    }
}

static void start_wave(Game *g, int count) {
    for (int i = 0; i < count; i++) {
        float x = (i % 2) ? 20.0f : SCREEN_W - 20.0f;
        float y = frand(20, SCREEN_H - 20);
        spawn_asteroid(g, x, y, 3);
    }
}

static void game_reset(Game *g, bool full) {
    memset(g, 0, sizeof(*g));
    g->ship.x = SCREEN_W / 2.0f;
    g->ship.y = SCREEN_H / 2.0f;
    g->ship.angle = -M_PI / 2.0f;
    g->ship.alive = true;
    g->ship.invul = 1.5f;
    if (full) {
        g->score = 0;
        g->lives = 3;
        g->wave = 1;
    }
    start_wave(g, 3);
}

static void fire_bullet(Game *g) {
    if (!g->ship.alive) return;
    for (int i = 0; i < MAX_BUL; i++) {
        if (!g->bullets[i].alive) {
            g->bullets[i].x = g->ship.x + cosf(g->ship.angle) * 6;
            g->bullets[i].y = g->ship.y + sinf(g->ship.angle) * 6;
            g->bullets[i].vx = cosf(g->ship.angle) * 180.0f;
            g->bullets[i].vy = sinf(g->ship.angle) * 180.0f;
            g->bullets[i].life = 1.2f;
            g->bullets[i].alive = true;
            return;
        }
    }
}

static void wrap(float *x, float *y) {
    if (*x < 0) *x += SCREEN_W;
    if (*x >= SCREEN_W) *x -= SCREEN_W;
    if (*y < 0) *y += SCREEN_H;
    if (*y >= SCREEN_H) *y -= SCREEN_H;
}

static float ast_radius(int size) {
    return (size == 3) ? 14.0f : (size == 2) ? 8.0f : 4.0f;
}

static void hit_asteroid(Game *g, int idx) {
    Asteroid a = g->asts[idx];
    g->asts[idx].alive = false;
    if (a.size == 3) { g->score += 20; spawn_asteroid(g, a.x, a.y, 2); spawn_asteroid(g, a.x, a.y, 2); }
    else if (a.size == 2) { g->score += 50; spawn_asteroid(g, a.x, a.y, 1); spawn_asteroid(g, a.x, a.y, 1); }
    else g->score += 100;
}

static int count_asteroids(const Game *g) {
    int n = 0;
    for (int i = 0; i < MAX_AST; i++) if (g->asts[i].alive) n++;
    return n;
}

static void game_update(Game *g, float dt) {
    if (g->game_over) return;

    float rot = 3.5f;
    float thrust = 80.0f;
    if (g->ship.alive) {
        if (g->key_left)  g->ship.angle -= rot * dt;
        if (g->key_right) g->ship.angle += rot * dt;
        if (g->key_up) {
            g->ship.vx += cosf(g->ship.angle) * thrust * dt;
            g->ship.vy += sinf(g->ship.angle) * thrust * dt;
        }
        g->ship.vx *= 0.995f;
        g->ship.vy *= 0.995f;
        g->ship.x += g->ship.vx * dt;
        g->ship.y += g->ship.vy * dt;
        wrap(&g->ship.x, &g->ship.y);
        if (g->ship.invul > 0) g->ship.invul -= dt;
    } else {
        g->ship.respawn -= dt;
        if (g->ship.respawn <= 0 && g->lives > 0) {
            g->ship.x = SCREEN_W / 2.0f;
            g->ship.y = SCREEN_H / 2.0f;
            g->ship.vx = g->ship.vy = 0;
            g->ship.angle = -M_PI / 2.0f;
            g->ship.alive = true;
            g->ship.invul = 1.5f;
        }
    }

    for (int i = 0; i < MAX_BUL; i++) {
        if (!g->bullets[i].alive) continue;
        g->bullets[i].x += g->bullets[i].vx * dt;
        g->bullets[i].y += g->bullets[i].vy * dt;
        wrap(&g->bullets[i].x, &g->bullets[i].y);
        g->bullets[i].life -= dt;
        if (g->bullets[i].life <= 0) g->bullets[i].alive = false;
    }

    for (int i = 0; i < MAX_AST; i++) {
        if (!g->asts[i].alive) continue;
        g->asts[i].x += g->asts[i].vx * dt;
        g->asts[i].y += g->asts[i].vy * dt;
        wrap(&g->asts[i].x, &g->asts[i].y);
    }

    for (int b = 0; b < MAX_BUL; b++) {
        if (!g->bullets[b].alive) continue;
        for (int a = 0; a < MAX_AST; a++) {
            if (!g->asts[a].alive) continue;
            float dx = g->bullets[b].x - g->asts[a].x;
            float dy = g->bullets[b].y - g->asts[a].y;
            float rr = ast_radius(g->asts[a].size);
            if (dx*dx + dy*dy <= rr*rr) {
                g->bullets[b].alive = false;
                hit_asteroid(g, a);
                break;
            }
        }
    }

    if (g->ship.alive && g->ship.invul <= 0) {
        for (int a = 0; a < MAX_AST; a++) {
            if (!g->asts[a].alive) continue;
            float dx = g->ship.x - g->asts[a].x;
            float dy = g->ship.y - g->asts[a].y;
            float rr = ast_radius(g->asts[a].size) + 4;
            if (dx*dx + dy*dy <= rr*rr) {
                g->ship.alive = false;
                g->ship.respawn = 1.2f;
                g->lives--;
                if (g->lives <= 0) g->game_over = true;
                break;
            }
        }
    }

    if (count_asteroids(g) == 0) {
        if (!g->wave_cleared_timer) {
            g->next_wave = 1.2f;
            g->wave_cleared_timer = true;
        }
        g->next_wave -= dt;
        if (g->next_wave <= 0) {
            g->wave++;
            start_wave(g, 3 + (g->wave - 1));
            g->wave_cleared_timer = false;
        }
    }
}

static void draw_line_color(SDL_Renderer *ren, float x1, float y1, float x2, float y2, Uint8 r, Uint8 gg, Uint8 b) {
    SDL_SetRenderDrawColor(ren, r, gg, b, 255);
    SDL_RenderDrawLineF(ren, x1, y1, x2, y2);
}

static void draw_polyline(SDL_Renderer *ren, const SDL_FPoint *pts, int n, bool closed) {
    for (int i = 0; i < n - 1; i++)
        SDL_RenderDrawLineF(ren, pts[i].x, pts[i].y, pts[i+1].x, pts[i+1].y);
    if (closed && n > 2)
        SDL_RenderDrawLineF(ren, pts[n-1].x, pts[n-1].y, pts[0].x, pts[0].y);
}

static void draw_ship(SDL_Renderer *ren, const Ship *s) {
    float a = s->angle;
    SDL_FPoint pts[3];
    pts[0].x = s->x + cosf(a) * 8;       pts[0].y = s->y + sinf(a) * 8;
    pts[1].x = s->x + cosf(a + 2.4f) * 6; pts[1].y = s->y + sinf(a + 2.4f) * 6;
    pts[2].x = s->x + cosf(a - 2.4f) * 6; pts[2].y = s->y + sinf(a - 2.4f) * 6;
    Uint8 bright = (s->invul > 0 && ((int)(s->invul * 10) % 2)) ? 120 : 255;
    SDL_SetRenderDrawColor(ren, bright, bright, bright, 255);
    draw_polyline(ren, pts, 3, true);
}

static void draw_asteroid(SDL_Renderer *ren, const Asteroid *a) {
    int n = 8;
    float r = ast_radius(a->size);
    SDL_FPoint pts[8];
    for (int i = 0; i < n; i++) {
        float ang = (2 * M_PI * i) / n;
        float rr = r * (0.8f + 0.25f * ((i % 2) ? 1 : -1) * 0.5f);
        pts[i].x = a->x + cosf(ang) * rr;
        pts[i].y = a->y + sinf(ang) * rr;
    }
    SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
    draw_polyline(ren, pts, n, true);
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

    if (g->ship.alive) draw_ship(ren, &g->ship);

    for (int i = 0; i < MAX_BUL; i++) {
        if (!g->bullets[i].alive) continue;
        draw_line_color(ren, g->bullets[i].x, g->bullets[i].y,
                        g->bullets[i].x + 1, g->bullets[i].y + 1, 255, 255, 200);
    }

    for (int i = 0; i < MAX_AST; i++) {
        if (g->asts[i].alive) draw_asteroid(ren, &g->asts[i]);
    }

    SDL_Color white = { 230, 230, 230, 255 };
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", g->score);
    draw_text(ren, font, 2, 1, buf, white);
    snprintf(buf, sizeof(buf), "L%d W%d", g->lives, g->wave);
    draw_text(ren, font, SCREEN_W - 54, 1, buf, white);

    if (g->game_over) {
        SDL_Color rc = { 255, 80, 80, 255 };
        draw_text(ren, font, SCREEN_W / 2 - 40, SCREEN_H / 2 - 6, "GAME OVER - R", rc);
    }

    SDL_RenderPresent(ren);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned)SDL_GetTicks() ^ 0x1337);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    bool have_ttf = (TTF_Init() == 0);
    if (!have_ttf) fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());

    SDL_Window *win = SDL_CreateWindow("Asteroids",
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
                if (k == SDLK_UP)    g.key_up    = down;
                if (down && k == SDLK_SPACE) fire_bullet(&g);
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
