// CardputerZero NES frontend. Provides the GUI/Joypad hooks LaiNES expects
// so the upstream core's main.cpp/gui.cpp/menu.cpp can be omitted.

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "viewport.hpp"
#include "input.hpp"

// LaiNES headers.
#include "cartridge.hpp"
#include "cpu.hpp"
#include "joypad.hpp"

using nesview::NES_W;
using nesview::NES_H;
using nesview::SCREEN_W;
using nesview::SCREEN_H;

namespace {

struct FrontendState {
    SDL_Window   *window   = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture  *texture  = nullptr;   // streaming RGB565 256x240
    TTF_Font     *font     = nullptr;
    nesview::ViewMode mode = nesview::ViewScaleToHeight;
    uint8_t pad1 = 0;
    uint8_t pad2 = 0;
    bool quit = false;
    bool paused = false;
    bool reset_pending = false;
    bool have_rom = false;
    std::string rom_path;
    uint16_t framebuf[NES_W * NES_H] = {};
};

FrontendState g;

static inline uint16_t rgb_to_565(uint32_t argb) {
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t gg = (argb >> 8) & 0xFF;
    uint8_t bb = argb & 0xFF;
    return uint16_t(((r & 0xF8) << 8) | ((gg & 0xFC) << 3) | (bb >> 3));
}

void draw_splash(const char *line1, const char *line2) {
    SDL_SetRenderDrawColor(g.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g.renderer);

    SDL_Rect bar{0, 70, SCREEN_W, 30};
    SDL_SetRenderDrawColor(g.renderer, 40, 40, 80, 255);
    SDL_RenderFillRect(g.renderer, &bar);

    if (g.font) {
        SDL_Color white{230, 230, 230, 255};
        const char *lines[2] = {line1, line2};
        int y = 50;
        for (int i = 0; i < 2; i++) {
            if (!lines[i] || !*lines[i]) { y += 24; continue; }
            SDL_Surface *s = TTF_RenderUTF8_Blended(g.font, lines[i], white);
            if (s) {
                SDL_Texture *t = SDL_CreateTextureFromSurface(g.renderer, s);
                if (t) {
                    SDL_Rect dst{(SCREEN_W - s->w) / 2, y, s->w, s->h};
                    SDL_RenderCopy(g.renderer, t, nullptr, &dst);
                    SDL_DestroyTexture(t);
                }
                SDL_FreeSurface(s);
            }
            y += 30;
        }
    }
    SDL_RenderPresent(g.renderer);
}

void pump_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { g.quit = true; continue; }
        if (ev.type == SDL_KEYDOWN) {
            switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE: g.quit = true; break;
            case SDLK_p:      g.paused = !g.paused; break;
            case SDLK_r:      g.reset_pending = true; break;
            case SDLK_v:      g.mode = nesview::cycle(g.mode); break;
            default: break;
            }
        }
    }
    auto b = nesinput::poll_keyboard();
    g.pad1 = b.to_byte();
    g.pad2 = 0;
}

} // namespace

// ---------- LaiNES GUI hooks ----------------------------------------------
// LaiNES PPU calls GUI::new_frame(u32 *pixels) with 256*240 ARGB8888 pixels,
// and APU calls GUI::new_samples(). Joypad reads via GUI::get_joypad_state.

namespace GUI {

constexpr unsigned WIDTH  = NES_W;
constexpr unsigned HEIGHT = NES_H;

void new_frame(u32 *pixels) {
    for (int i = 0; i < NES_W * NES_H; i++) {
        g.framebuf[i] = rgb_to_565(pixels[i]);
    }
}

void new_samples(const blip_sample_t *, size_t) {
    // Audio handled via SDL_OpenAudioDevice below; LaiNES also pushes through
    // its own SDL_QueueAudio path in its gui.cpp. We no-op here because audio
    // is wired directly inside the core's apu.cpp via Sound_Queue when built.
}

u8 get_joypad_state(int n) {
    return (n == 0) ? g.pad1 : g.pad2;
}

// Required by apu.cpp: we never fast-forward in this embedded frontend.
bool is_fast_forward() { return false; }

// Some forks reference these; provide weak-ish stubs.
void set_size(int) {}
void run() {}

} // namespace GUI

// Some LaiNES builds reference these helpers.
extern "C" void LaiNES_frontend_stub() {}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    }

    g.window = SDL_CreateWindow("NES",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_BORDERLESS);
    if (!g.window) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    g.renderer = SDL_CreateRenderer(g.window, -1, SDL_RENDERER_SOFTWARE);
    if (!g.renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }
    g.texture = SDL_CreateTexture(g.renderer,
        SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        NES_W, NES_H);

    // Load a readable font if available.
    const char *fonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    };
    for (const char *fp : fonts) {
        g.font = TTF_OpenFont(fp, 12);
        if (g.font) break;
    }

    if (const char *mv = std::getenv("NES_VIEW_MODE")) {
        g.mode = nesview::parse_mode(mv);
    }

    const char *rom = (argc > 1) ? argv[1] : std::getenv("NES_ROM");
    g.have_rom = (rom && *rom);
    if (g.have_rom) g.rom_path = rom;

    if (!g.have_rom) {
        // Wait on splash until ESC. Poll events so we can still quit cleanly.
        while (!g.quit) {
            pump_events();
            draw_splash("No ROM loaded", "Set NES_ROM=/path/to/file.nes");
            SDL_Delay(50);
        }
    } else {
        draw_splash("Loading ROM...", g.rom_path.c_str());
        Cartridge::load(g.rom_path.c_str());

        const uint32_t FRAME_MS = 16; // 60 fps
        uint32_t next = SDL_GetTicks();
        while (!g.quit) {
            pump_events();
            if (g.reset_pending) {
                CPU::power();
                g.reset_pending = false;
            }
            if (!g.paused) {
                CPU::run_frame();
            }

            SDL_UpdateTexture(g.texture, nullptr, g.framebuf, NES_W * 2);
            SDL_SetRenderDrawColor(g.renderer, 0, 0, 0, 255);
            SDL_RenderClear(g.renderer);
            SDL_Rect src = nesview::src_rect(g.mode);
            SDL_Rect dst = nesview::dst_rect(g.mode);
            SDL_RenderCopy(g.renderer, g.texture, &src, &dst);
            SDL_RenderPresent(g.renderer);

            next += FRAME_MS;
            uint32_t now = SDL_GetTicks();
            if (now < next) SDL_Delay(next - now);
            else            next = now;
        }
    }

    if (g.texture)  SDL_DestroyTexture(g.texture);
    if (g.font)     TTF_CloseFont(g.font);
    if (g.renderer) SDL_DestroyRenderer(g.renderer);
    if (g.window)   SDL_DestroyWindow(g.window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
