#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

namespace nesinput {

// NES pad bit layout used by LaiNES Joypad: A, B, Select, Start, Up, Down, Left, Right.
struct Buttons {
    bool a, b, select, start, up, down, left, right;
    uint8_t to_byte() const {
        uint8_t v = 0;
        if (a)      v |= 0x01;
        if (b)      v |= 0x02;
        if (select) v |= 0x04;
        if (start)  v |= 0x08;
        if (up)     v |= 0x10;
        if (down)   v |= 0x20;
        if (left)   v |= 0x40;
        if (right)  v |= 0x80;
        return v;
    }
};

Buttons poll_keyboard();

} // namespace nesinput
