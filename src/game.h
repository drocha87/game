#pragma once

#include <algorithm>
#include <memory>
#include <array>
#include <random>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include <FastNoise/FastNoise.h>

#include "config.h"
#include "asset.h"
#include "tile.h"
#include "tileset.h"

class Game
{
public:  
  Game(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font)
    : window(window), renderer(renderer), font(font), frequency((double)SDL_GetPerformanceFrequency())
  {
  }

  ~Game()
  {
    // @fixme: cleanup
  }
  
  // Disable copy constructor and assignment operator for `Game`
  Game(const Game &) = delete;
  Game &operator=(const Game &) = delete;

    // this are the logical game width and height
  const int width  = TILE_SIZE * MAP_SIZE;
  const int height = TILE_SIZE * MAP_SIZE;
    
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  SDL_FPoint mouse_position = {0};
  SDL_Texture *world_texture = nullptr;
  SDL_FRect viewport; // Where the world is drawn on screen
  
  std::array<Tile, MAP_SIZE * MAP_SIZE> tiles;
  
  // timer related variables
  Uint64 prev_time, curr_time;
  double delta_time, frequency;
  
  float zoom = 1.0f, prev_zoom = 1.0f; // Stores zoom from *before* the current frame/input
  
  SDL_FPoint snap_offset = {0};
  bool snapping = false;
  
  bool render_grid = false;
  // SDL_Texture *tileset_grass;
  // @fixme: do not do this like this
  SDL_Texture *grass;
  SDL_Texture *frame;
  SDL_Texture *solid_base_tile;
  SDL_Texture *bg;
  
  Tile *tile_on_mouse;

  Asset assets;
  
  struct Text
  {
    SDL_FRect rect;
    SDL_Surface *surface;
  };

  bool create_world();
  SDL_FPoint screen_to_world(SDL_FPoint screen_point) const;
  void handle_mouse_wheel(int mouse_screen_x, int mouse_screen_y, float wheel_y);
  void handle_snapping(SDL_MouseMotionEvent &motion);
  void render_fps();
  bool prepare_text(const char *text, float size, SDL_Color color, Text *output);
  void destroy_text(Text *text);
  void render_text(Text *text);
  SDL_AppResult render();
  void initialize_map(int noise_seed = 12237861);
};
