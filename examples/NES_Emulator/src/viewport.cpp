#include "viewport.hpp"

#include <cstring>

namespace nesview {

SDL_Rect dst_rect(ViewMode mode) {
    SDL_Rect r{0, 0, SCREEN_W, SCREEN_H};
    switch (mode) {
    case ViewCropHeight:
        // Stretch horizontally 256 -> 320 (1.25x), height 170 maps exactly from
        // the 170 central source rows (see src_rect).
        r.x = 0; r.y = 0; r.w = SCREEN_W; r.h = SCREEN_H;
        break;
    case ViewScaleToHeight:
    case ViewFit:
    default: {
        // height=170, width = 170 * 256 / 240 = 181 (rounded).
        int h = SCREEN_H;
        int w = (h * NES_W) / NES_H;
        r.x = (SCREEN_W - w) / 2;
        r.y = 0;
        r.w = w;
        r.h = h;
        break;
    }
    }
    return r;
}

SDL_Rect src_rect(ViewMode mode) {
    SDL_Rect r{0, 0, NES_W, NES_H};
    if (mode == ViewCropHeight) {
        // Central 170 rows of the 240-row NES image.
        r.x = 0;
        r.y = (NES_H - SCREEN_H) / 2; // (240 - 170) / 2 = 35
        r.w = NES_W;
        r.h = SCREEN_H;
    }
    return r;
}

const char *mode_name(ViewMode mode) {
    switch (mode) {
    case ViewCropHeight: return "cropheight";
    case ViewFit:        return "fit";
    case ViewScaleToHeight:
    default:             return "scaletoheight";
    }
}

ViewMode parse_mode(const char *s) {
    if (!s || !*s) return ViewScaleToHeight;
    if (std::strcmp(s, "cropheight") == 0)    return ViewCropHeight;
    if (std::strcmp(s, "fit") == 0)           return ViewFit;
    if (std::strcmp(s, "scaletoheight") == 0) return ViewScaleToHeight;
    return ViewScaleToHeight;
}

ViewMode cycle(ViewMode m) {
    return (m == ViewScaleToHeight) ? ViewCropHeight : ViewScaleToHeight;
}

} // namespace nesview
