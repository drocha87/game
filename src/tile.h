#pragma once

#include <array>

#include <SDL3/SDL.h>

#include "config.h"
#include "asset.h"

enum TerrainKind : uint8_t
{
  Crust,
  Water,
  Dirt,
  Grass,
  TERRAIN_KIND_COUNT,
};

struct Tile
{
  using Tiles = const std::array<Tile, MAP_SIZE * MAP_SIZE>&;

  TerrainKind kind;
  
  SDL_FRect rect;
  SDL_FRect sprite;
  SDL_Point coord; 
  bool selected;

  std::array<int, 8> get_neighbors(int map_size) const;
  SDL_FRect get_bitmask(Tiles tiles) const;
  void render(SDL_Renderer *renderer, Tiles tiles, bool mouse_hover, const Asset& assets) const;
};

const SDL_Color TERRAIN_COLORS[TERRAIN_KIND_COUNT] = {
    {150, 140, 130, SDL_ALPHA_OPAQUE}, // Crust
    {30, 90, 150, SDL_ALPHA_OPAQUE},   // Water
    {139, 69, 19, SDL_ALPHA_OPAQUE},   // Dirt
    {34, 139, 34, SDL_ALPHA_OPAQUE},   // Grass 
    /* {240, 220, 130, SDL_ALPHA_OPAQUE}, // Sand */
    /* {220, 230, 240, SDL_ALPHA_OPAQUE}, // Snow */
    /* {34, 94, 34, SDL_ALPHA_OPAQUE},    // Forest */
    /* {100, 110, 110, SDL_ALPHA_OPAQUE}, // Mountain */
    /* {80, 90, 60, SDL_ALPHA_OPAQUE},    // Swamp */
    /* {255, 69, 0, SDL_ALPHA_OPAQUE}     // Lava */
    // Make sure the order here matches the enum order exactly!
};

#define SDL_COLOR_RGBA(color) (color).r, (color).g, (color).b, (color).a
