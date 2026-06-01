#ifndef CPZ_RENDER_H
#define CPZ_RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"

typedef struct {
    SDL_Renderer *ren;
    TTF_Font *font;
} Renderer;

void render_frame(Renderer *r, const GameState *g);

#endif
