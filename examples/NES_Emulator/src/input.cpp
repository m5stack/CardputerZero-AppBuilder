#include "input.hpp"

namespace nesinput {

Buttons poll_keyboard() {
    const Uint8 *k = SDL_GetKeyboardState(nullptr);
    Buttons b{};
    b.up     = k[SDL_SCANCODE_UP];
    b.down   = k[SDL_SCANCODE_DOWN];
    b.left   = k[SDL_SCANCODE_LEFT];
    b.right  = k[SDL_SCANCODE_RIGHT];
    b.a      = k[SDL_SCANCODE_K] || k[SDL_SCANCODE_Z];
    b.b      = k[SDL_SCANCODE_J] || k[SDL_SCANCODE_X];
    b.start  = k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER];
    b.select = k[SDL_SCANCODE_RSHIFT] || k[SDL_SCANCODE_BACKSPACE];
    return b;
}

} // namespace nesinput
