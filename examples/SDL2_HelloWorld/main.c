#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 320
#define SCREEN_H 170
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define BEEP_PATH "/usr/share/sounds/alsa/Front_Center.wav"

static SDL_Texture *render_text(SDL_Renderer *r, TTF_Font *f, const char *s, SDL_Color c) {
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, c);
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FreeSurface(surf);
    return tex;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    int have_audio = 0;
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 512) == 0) {
        have_audio = 1;
    } else {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
    }

    Mix_Chunk *beep = NULL;
    if (have_audio) {
        beep = Mix_LoadWAV(BEEP_PATH);
        if (!beep) fprintf(stderr, "Mix_LoadWAV(%s) failed: %s\n", BEEP_PATH, Mix_GetError());
    }

    SDL_Window *win = SDL_CreateWindow("CardputerZero",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_BORDERLESS);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont(FONT_PATH, 14);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont(%s) failed: %s\n", FONT_PATH, TTF_GetError());
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture *label = render_text(ren, font, "Hello CardputerZero", white);

    int running = 1;
    Uint32 last_fps_update = SDL_GetTicks();
    int frames = 0;
    int fps = 0;
    char fps_buf[32] = "FPS: 0";
    SDL_Texture *fps_tex = render_text(ren, font, fps_buf, white);

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
                if (beep) Mix_PlayChannel(-1, beep, 0);
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        if (label) {
            int w, h;
            SDL_QueryTexture(label, NULL, NULL, &w, &h);
            SDL_Rect dst = { (SCREEN_W - w) / 2, (SCREEN_H - h) / 2, w, h };
            SDL_RenderCopy(ren, label, NULL, &dst);
        }

        if (fps_tex) {
            int w, h;
            SDL_QueryTexture(fps_tex, NULL, NULL, &w, &h);
            SDL_Rect dst = { 4, 4, w, h };
            SDL_RenderCopy(ren, fps_tex, NULL, &dst);
        }

        SDL_RenderPresent(ren);

        frames++;
        Uint32 now = SDL_GetTicks();
        if (now - last_fps_update >= 1000) {
            fps = frames;
            frames = 0;
            last_fps_update = now;
            snprintf(fps_buf, sizeof(fps_buf), "FPS: %d", fps);
            if (fps_tex) SDL_DestroyTexture(fps_tex);
            fps_tex = render_text(ren, font, fps_buf, white);
        }
    }

    if (label) SDL_DestroyTexture(label);
    if (fps_tex) SDL_DestroyTexture(fps_tex);
    TTF_CloseFont(font);
    if (beep) Mix_FreeChunk(beep);
    if (have_audio) Mix_CloseAudio();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
