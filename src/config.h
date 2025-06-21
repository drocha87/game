#pragma once

inline constexpr int TILE_SIZE = 32; // pixels
inline constexpr int MAP_SIZE = 128;
inline constexpr int TARGET_FPS = 30;
inline constexpr double TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

static SDL_Color WHITE = {255, 255, 255, SDL_ALPHA_OPAQUE};
