#pragma once

#include <SDL2/SDL_rect.h>

namespace nesview {

// NES frame is always 256x240.
static constexpr int NES_W = 256;
static constexpr int NES_H = 240;

// Physical display on CardputerZero (ST7789V).
static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 170;

enum ViewMode {
    ViewScaleToHeight = 0, // letterbox left+right, NES 4:3 preserved
    ViewCropHeight    = 1, // 1.25x horizontal stretch, crop top/bottom
    ViewFit           = 2, // alias for ScaleToHeight
    ViewCount         = 3
};

// Returns the destination rect for SDL_RenderCopy.
// For ViewCropHeight the src sub-rect is returned via src_out.
SDL_Rect dst_rect(ViewMode mode);
SDL_Rect src_rect(ViewMode mode);

const char *mode_name(ViewMode mode);
ViewMode parse_mode(const char *s);
ViewMode cycle(ViewMode m);

} // namespace nesview
