#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "render.h"

#define FONT_PATH   "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define BOUNCE_WAV  "assets/bounce.wav"
#define BREAK_WAV   "assets/break.wav"

#define TICK_MS     16
#define TICK_SEC    (TICK_MS / 1000.0f)

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    int have_audio = 0;
    Mix_Chunk *snd_bounce = NULL;
    Mix_Chunk *snd_break  = NULL;
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 512) == 0) {
        have_audio = 1;
        snd_bounce = Mix_LoadWAV(BOUNCE_WAV);
        snd_break  = Mix_LoadWAV(BREAK_WAV);
        if (!snd_bounce) fprintf(stderr, "Mix_LoadWAV %s: %s\n", BOUNCE_WAV, Mix_GetError());
        if (!snd_break)  fprintf(stderr, "Mix_LoadWAV %s: %s\n", BREAK_WAV,  Mix_GetError());
    } else {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
    }

    SDL_Window *win = SDL_CreateWindow("Breakout",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_BORDERLESS);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont(FONT_PATH, 12);
    if (!font) fprintf(stderr, "TTF_OpenFont %s: %s\n", FONT_PATH, TTF_GetError());

    Renderer rctx = { ren, font };
    GameState game;
    game_init(&game);

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
                if (down && k == SDLK_r) { game_restart(&game); continue; }
                game_handle_key(&game, k, down);
            }
        }

        Uint32 now = SDL_GetTicks();
        float frame_dt = (now - last) / 1000.0f;
        last = now;
        if (frame_dt > 0.25f) frame_dt = 0.25f;
        accum += frame_dt;

        SoundEvents sfx = {0, 0};
        while (accum >= TICK_SEC) {
            game_update(&game, TICK_SEC, &sfx);
            accum -= TICK_SEC;
        }

        if (have_audio) {
            if (sfx.bounce && snd_bounce) Mix_PlayChannel(-1, snd_bounce, 0);
            if (sfx.brick  && snd_break)  Mix_PlayChannel(-1, snd_break,  0);
        }

        render_frame(&rctx, &game);

        Uint32 elapsed = SDL_GetTicks() - now;
        if (elapsed < TICK_MS) SDL_Delay(TICK_MS - elapsed);
    }

    if (font) TTF_CloseFont(font);
    if (snd_bounce) Mix_FreeChunk(snd_bounce);
    if (snd_break)  Mix_FreeChunk(snd_break);
    if (have_audio) Mix_CloseAudio();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
