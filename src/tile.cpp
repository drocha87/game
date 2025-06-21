#include "tile.h"
#include "tileset.h"

std::array<int, 8> Tile::get_neighbors(int map_size) const
{
  std::array<int, 8> neighbors;
    
  constexpr std::array<SDL_Point, 8> offset = {{
      {0, -1},  // top
      {1, -1},  // top-right
      {1, 0},   // right
      {1, 1},   // bottom-right
      {0, 1},   // bottom
      {-1, 1},  // bottom-left
      {-1, 0},  // left
      {-1, -1}, // top-left
    }};

  for (size_t i = 0; i < 8; ++i)
    {
      const int nx = coord.x + offset[i].x;
      const int ny = coord.y + offset[i].y;

      if (nx >= 0 && ny >= 0 && nx < map_size && ny < map_size)
        {
          neighbors[i] = ny * map_size + nx;
        }
    }

  return neighbors;
}

SDL_FRect Tile::get_bitmask(Tiles tiles) const
{
  auto neighbors = get_neighbors(MAP_SIZE);
    
  int mask = 0;

  // Bit layout (matches bit index)
  //  7 0 1
  //  6   2
  //  5 4 3

  auto is_same = [&](int i) -> bool
  {
    return (neighbors[i] < 0 || kind == tiles[neighbors[i]].kind);
  };

  // Cardinal directions
  bool N = is_same(0);
  bool E = is_same(2);
  bool S = is_same(4);
  bool W = is_same(6);

  if (N)
    mask |= (1 << 0);
  if (E)
    mask |= (1 << 2);
  if (S)
    mask |= (1 << 4);
  if (W)
    mask |= (1 << 6);

  bool NE = is_same(1);
  bool SE = is_same(3);
  bool SW = is_same(5);
  bool NW = is_same(7);

  // Diagonals only if both adjacent cardinals are present
  if (NE && N && E)
    mask |= (1 << 1); // NE
  if (SE && E && S)
    mask |= (1 << 3); // SE
  if (SW && S && W)
    mask |= (1 << 5); // SW
  if (NW && W && N)
    mask |= (1 << 7); // NW

  // Lookup UV
  auto p = get_terrain_mask(mask);
  if (p.x == 0 && p.y == 0 && mask != 0)
    {
      SDL_Log("Missing UV for bitmask %d", mask);
    }

  return {(float)p.x * TILE_SIZE, (float)p.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
}

void Tile::render(SDL_Renderer *renderer, Tiles tiles, bool mouse_hover, const Asset& assets) const
{
  if (kind == TerrainKind::Grass)
    {
      SDL_SetRenderDrawColor(renderer, SDL_COLOR_RGBA(TERRAIN_COLORS[kind]));
      SDL_RenderFillRect(renderer, &rect);                                         
      // SDL_FRect bitmask = get_bitmask(tiles);
      // SDL_RenderTexture(renderer, grass, &bitmask, &rect);
    }
  // else
  //   {
  //     SDL_SetRenderDrawColor(renderer, SDL_COLOR_RGBA(TERRAIN_COLORS[tile.kind]));
  //     SDL_RenderFillRect(renderer, &tile.rect);
  //   }
    
  // if (tile.selected)
  //   {
  //     tile.kind = TerrainKind::Water;
  //     SDL_FRect bitmask = get_bitmask(tile);
  //     SDL_RenderTexture(renderer, grass, &bitmask, &tile.rect);
  //   }
  // else
  //   {
  //     tile.kind = TerrainKind::Crust;
  //   }

  if (mouse_hover)
    {
      SDL_Texture* tex = assets.get_texture("frame");
      if (tex)
        {
          SDL_RenderTexture(renderer, tex, nullptr, &rect);
        }
    }
  
  // if (SDL_PointInRectFloat(&point, &rect))
  //   {
  //     SDL_RenderTexture(renderer, frame, nullptr, &rect);
  //   }

  // SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 255);
  // SDL_RenderLine(renderer, point.x, 0, point.x, MAP_SIZE * TILE_SIZE);
  // SDL_RenderLine(renderer, 0, point.y, MAP_SIZE * TILE_SIZE, point.y);
}
