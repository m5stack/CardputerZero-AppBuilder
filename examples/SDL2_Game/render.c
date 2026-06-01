#include "render.h"
#include <stdio.h>

static const SDL_Color BRICK_COLORS[5] = {
    {220,  50,  50, 255},
    {220, 140,  40, 255},
    {220, 210,  40, 255},
    { 80, 200,  80, 255},
    { 80, 140, 220, 255},
};

static void draw_text(Renderer *r, const char *s, int x, int y, SDL_Color c, bool right_align) {
    if (!r->font) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(r->font, s, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r->ren, surf);
    int w = surf->w, h = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_Rect dst = { right_align ? (x - w) : x, y, w, h };
    SDL_RenderCopy(r->ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void draw_frect(SDL_Renderer *ren, const SDL_FRect *fr) {
    SDL_Rect r = { (int)fr->x, (int)fr->y, (int)fr->w, (int)fr->h };
    SDL_RenderFillRect(ren, &r);
}

void render_frame(Renderer *r, const GameState *g) {
    SDL_SetRenderDrawColor(r->ren, 8, 8, 16, 255);
    SDL_RenderClear(r->ren);

    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
        const Brick *b = &g->bricks[i];
        if (!b->alive) continue;
        SDL_Color c = BRICK_COLORS[b->color % 5];
        SDL_SetRenderDrawColor(r->ren, c.r, c.g, c.b, 255);
        draw_frect(r->ren, &b->rect);
    }

    SDL_SetRenderDrawColor(r->ren, 230, 230, 230, 255);
    draw_frect(r->ren, &g->paddle);
    draw_frect(r->ren, &g->ball);

    SDL_Color white = {255, 255, 255, 255};
    char buf[32];
    snprintf(buf, sizeof(buf), "LIVES %d", g->lives);
    draw_text(r, buf, 4, 2, white, false);
    snprintf(buf, sizeof(buf), "SCORE %d", g->score);
    draw_text(r, buf, SCREEN_W - 4, 2, white, true);

    if (g->state == STATE_READY) {
        draw_text(r, "SPACE to launch", SCREEN_W / 2 - 55, SCREEN_H / 2 - 8, white, false);
    } else if (g->state == STATE_LOST) {
        SDL_Color red = {240, 80, 80, 255};
        draw_text(r, "GAME OVER - press R", SCREEN_W / 2 - 70, SCREEN_H / 2 - 8, red, false);
    } else if (g->state == STATE_WON) {
        SDL_Color grn = {120, 240, 120, 255};
        draw_text(r, "YOU WIN - press R", SCREEN_W / 2 - 64, SCREEN_H / 2 - 8, grn, false);
    }

    SDL_RenderPresent(r->ren);
}
