#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

struct GameContext
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  // SDL_Texture *texture;
  TTF_Font *font;
  SDL_FPoint mouse_point;
  SDL_MouseButtonFlags mouse_flags;

  float zoom_scale;
  bool should_scale;

  // timer related variables
  Uint32 last_time;

  SDL_Texture *world_layer_texture = nullptr;

  bool dragging;
  int last_mouse_x, last_mouse_y;
  int camera_x, camera_y;
};
