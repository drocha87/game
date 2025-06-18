#pragma once

#include <algorithm>
#include <memory>
#include <array>
#include <random>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include <FastNoise/FastNoise.h>

#include "terrain.h"

static SDL_Color WHITE = {255, 255, 255, SDL_ALPHA_OPAQUE};
static SDL_Color DEFAULT_BACKGROUND_COLOR = {0x18, 0x18, 0x18, SDL_ALPHA_OPAQUE};

struct Tile
{
  std::unique_ptr<Terrain> terrain;

  SDL_FRect rect;
  SDL_FRect sprite;
  SDL_Point coord;

  std::array<int, 8> neighbors;
};

class Game
{
  static const int tile_size = 32; // tile size in pixel
  static const int map_size = 128; // the number of tiles
  
  // this are the logical game width and height
  const int width  = tile_size * map_size;
  const int height = tile_size * map_size;
  
 public:
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  SDL_FPoint mouse_position = {0};
  SDL_Texture *world_texture = nullptr;
  
  std::array<Tile, map_size * map_size> tiles;
  
  // timer related variables
  Uint64 last_time;
  
  SDL_FRect viewport; // Where the world is drawn on screen
  
  float zoom = 1.0f;
  float prev_zoom = 1.0f; // Stores zoom from *before* the current frame/input
  
  bool snapping = false;
  SDL_FPoint snap_offset = {0};

  SDL_Texture *tileset_grass;
  bool render_grid = false;
    
 Game(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font)
   : window(window),
    renderer(renderer),
    font(font),
    last_time(SDL_GetTicks())
    {
    }

  ~Game()
    {
      // @fixme: cleanup
    }
  
  // Disable copy constructor and assignment operator for `Game`
  Game(const Game &) = delete;
  Game &operator=(const Game &) = delete;

  bool create_world()
  {
    world_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    // @note: set it for transparency
    // SDL_SetTextureBlendMode(ctx->world_texture, SDL_BLENDMODE_BLEND);
    if (!world_texture) return false;
    viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
    return true;      
  }
  
  // @fixme: do not do this like this
  SDL_Texture *grass;

  bool load_assets()
  {
    grass = IMG_LoadTexture(renderer, "assets/demo.png");
    return !!grass;
  }


  SDL_FPoint screen_to_world(SDL_FPoint screen_point) const
  {
    float x = (screen_point.x - viewport.x) / zoom;
    float y = (screen_point.y - viewport.y) / zoom;
    return {x, y};
  }

  void handle_mouse_wheel(int mouse_screen_x, int mouse_screen_y, float wheel_y)
  {
    float new_zoom;
    float zoom_step = 1.1f;

    if (wheel_y > 0)      new_zoom = zoom * zoom_step;
    else if (wheel_y < 0) new_zoom = zoom / zoom_step;

    new_zoom = std::clamp(new_zoom, 0.5f, 4.0f);
    if (new_zoom == zoom) return;

    // Compute current texture size on screen
    float world_screen_w = viewport.w * zoom;
    float world_screen_h = viewport.h * zoom;

    // Position inside texture, in screen space
    float rel_x = (mouse_screen_x - viewport.x) / world_screen_w;
    float rel_y = (mouse_screen_y - viewport.y) / world_screen_h;

    // Apply zoom
    zoom = new_zoom;

    // New texture size
    float new_world_screen_w = viewport.w * zoom;
    float new_world_screen_h = viewport.h * zoom;

    // Adjust world position so the cursor points to the same content
    viewport.x = mouse_screen_x - rel_x * new_world_screen_w;
    viewport.y = mouse_screen_y - rel_y * new_world_screen_h;
  }

  void handle_snapping(SDL_MouseMotionEvent &motion)
  {
    int dx = static_cast<int>(motion.x - snap_offset.x);
    int dy = static_cast<int>(motion.y - snap_offset.y);

    viewport.x += dx;
    viewport.y += dy;

    snap_offset.x = motion.x;
    snap_offset.y = motion.y;
  }

  void set_neighbors(Tile &tile)
  {
    tile.neighbors.fill(-1);
    const std::array<SDL_Point, 8> offset = {{
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
        const int nx = tile.coord.x + offset[i].x;
        const int ny = tile.coord.y + offset[i].y;

        if (nx >= 0 && ny >= 0 && nx < map_size && ny < map_size)
          {
            tile.neighbors[i] = ny * map_size + nx;
          }
      }
  }

  SDL_Point get_terrain_mask(int mask)
  {
    switch (mask)
      {
      case 0:
        return {0, 0};
      case 4:
        return {1, 0};
      case 84:
        return {2, 0};
      case 92:
        return {3, 0};
      case 124:
        return {4, 0};
      case 116:
        return {5, 0};
      case 80:
        return {6, 0};
      case 16:
        return {0, 1};
      case 28:
        return {1, 1};
      case 117:
        return {2, 1};
      case 95:
        return {3, 1};
      case 255:
        {
          static const std::array<SDL_Point, 3> options = {{{4, 2},
                                                            {1, 4},
                                                            {4, 1}}};

          static std::random_device rd;
          static std::mt19937 rng(rd());
          static std::uniform_int_distribution<std::size_t> dist(0, options.size() - 1);

          return options[dist(rng)];
        }
      case 253:
        return {5, 1};
      case 113:
        return {6, 1};
      case 21:
        return {0, 2};
      case 87:
        return {1, 2};
      case 221:
        return {2, 2};
      case 127:
        return {3, 2};
      case 247:
        return {5, 2};
      case 209:
        return {6, 2};
      case 29:
        return {0, 3};
      case 125:
        return {1, 3};
      case 119:
        return {2, 3};
      case 199:
        return {3, 3};
      case 215:
        return {4, 3};
      case 213:
        return {5, 3};
      case 81:
        return {6, 3};
      case 31:
        return {0, 4};
      case 241:
        return {2, 4};
      case 20:
        return {3, 4};
      case 65:
        return {4, 4};
      case 17:
        return {5, 4};
      case 1:
        return {6, 4};
      case 23:
        return {0, 5};
      case 223:
        return {1, 5};
      case 245:
        return {2, 5};
      case 85:
        return {3, 5};
      case 68:
        return {4, 5};
      case 93:
        return {5, 5};
      case 112:
        return {6, 5};
      case 5:
        return {0, 6};
      case 71:
        return {1, 6};
      case 197:
        return {2, 6};
      case 69:
        return {3, 6};
      case 64:
        return {4, 6};
      case 7:
        return {5, 6};
      case 193:
        return {6, 6};
      default:
        return {0, 0};
      }
  }

  SDL_FRect get_bitmask(Tile &tile)
  {
    int mask = 0;

    // Bit layout (matches bit index)
    //  7 0 1
    //  6   2
    //  5 4 3

    auto is_same = [&](int i) -> bool
      {
        return (tile.neighbors[i] < 0 || tile.terrain->kind == tiles[tile.neighbors[i]].terrain->kind);
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

    return {(float)p.x * tile_size, (float)p.y * tile_size, tile_size, tile_size};
  }

  void render_fps()
  {
    static Uint64 fps = 0, elapsed = 0;
    static char buffer[128] = {0};

    Uint64 current_time, frame_time;

    current_time = SDL_GetTicks();
    frame_time = current_time - last_time;
    last_time = current_time;

    elapsed += frame_time;

    if (elapsed >= 5000)
      {
        elapsed = 0;
        fps = (Uint32)(1000.0f / frame_time);
      }

    int rw, rh;
    SDL_GetCurrentRenderOutputSize(renderer, &rw, &rh);

    snprintf(buffer, sizeof buffer, "FPS: %zu, tile: (%d, %d)", fps, tile_on_mouse->coord.x, tile_on_mouse->coord.y);
    Text t = {0};
    if (prepare_text(buffer, 12, WHITE, &t))
      {
        t.rect.x = rw - t.rect.w - 10;
        t.rect.y = rh - t.rect.h - 10;
        render_text(&t);
        destroy_text(&t);
      }
  }
  
  struct Text
  {
    SDL_FRect rect;
    SDL_Surface *surface;
  };
  
  bool prepare_text(const char *text, float size, SDL_Color color, Text *output)
  {
    TTF_SetFontSize(font, size);

    int w, h;
    if (TTF_GetStringSize(font, text, 0, &w, &h))
      {
        output->rect = SDL_FRect{.w = (float)w, .h = (float)h};
        output->surface = TTF_RenderText_Blended(font, text, 0, color);
        return true;
      }

    SDL_Log("Couldn't prepare text SDL: %s\n", SDL_GetError());
    return false;
  }

  void destroy_text(Text *text)
  {
    SDL_DestroySurface(text->surface);
  }

  void render_text(Text *text)
  {
    if (text->surface)
      {
        auto texture = SDL_CreateTextureFromSurface(renderer, text->surface);
        if (!texture)
          {
            SDL_Log("Couldn't create text: %s\n", SDL_GetError());
          }
        else
          {
            SDL_FRect dst;
            SDL_GetTextureSize(texture, &dst.w, &dst.h);
            dst.x = text->rect.x;
            dst.y = text->rect.y;
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
          }
      }
  }

  SDL_AppResult render()
  {
    /* SDL_SetRenderDrawColor(renderer, DEFAULT_BACKGROUND_COLOR.r, DEFAULT_BACKGROUND_COLOR.g, DEFAULT_BACKGROUND_COLOR.b, DEFAULT_BACKGROUND_COLOR.a); */
    /* SDL_RenderClear(renderer); */

    SDL_SetRenderTarget(renderer, world_texture);
    SDL_SetRenderScale(renderer, zoom, zoom);

    // @fixme: I think we could avoid this RenderClear since we will render in the entire texture anyway
    SDL_SetRenderDrawColor(renderer, DEFAULT_BACKGROUND_COLOR.r, DEFAULT_BACKGROUND_COLOR.g, DEFAULT_BACKGROUND_COLOR.b, DEFAULT_BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);

    for (auto &tile : tiles)
      {
        render_tile(tile);
      }

    if (render_grid)
      {
        SDL_SetRenderDrawColor(renderer, 0xfa, 0xfa, 0xfa, 0xff);
        for (int i = 0; i < map_size; i++)
          {
            // render horizontal grid line
            float x = 0.0f;
            float y = static_cast<float>(i * tile_size);
            SDL_RenderLine(renderer, x, y, map_size * tile_size, y);

            // render vertical grid line
            x = static_cast<float>(i * tile_size);
            y = 0;
            SDL_RenderLine(renderer, x, y, x, map_size * tile_size);
          }
      }

    // 2. Reset Render Target to Window
    SDL_SetRenderTarget(renderer, NULL);

    // 3. Compose everything on the main renderer
    SDL_SetRenderDrawColor(renderer, DEFAULT_BACKGROUND_COLOR.r, DEFAULT_BACKGROUND_COLOR.g, DEFAULT_BACKGROUND_COLOR.b, DEFAULT_BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);
    
    SDL_RenderTexture(renderer, world_texture, NULL, &viewport);

    render_fps();
    
    // Present the final rendered frame
    SDL_RenderPresent(renderer);
    
    return SDL_APP_CONTINUE;
  }


  Tile *tile_on_mouse;
  
  // @fixme: make it const
  void render_tile(Tile &tile)
  {
    const SDL_FPoint point = screen_to_world(mouse_position);

    if (SDL_PointInRectFloat(&point, &tile.rect))
      {
        tile_on_mouse = &tile;
        SDL_Log("(screen x = %f, y = %f)", point.x, point.y);
        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 255);
      }
    else
      {
        SDL_SetRenderDrawColor(renderer, 0xED, 0x6A, 0xFF, 255);
      }
    SDL_RenderFillRect(renderer, &tile.rect);
  }

  FastNoise::SmartNode<FastNoise::FractalRidged> noise_generator;

  void initialize_noise_generator()
  {
    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    noise_generator = FastNoise::New<FastNoise::FractalRidged>();

    // fnSimplex->GenUniformGrid2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 0.09f, seed);
    noise_generator->SetSource(fnSimplex);
    noise_generator->SetGain(0.6f);
    noise_generator->SetLacunarity(2.0f);
    noise_generator->SetOctaveCount(4);

    // noise_generator->GenUniformGrid2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 1.0f, seed);
    // noise_generator->GenSingle2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 1.0f, seed);
  }

  void initialize_map(int noise_seed = 12237861)
  {
    initialize_noise_generator();

    for (size_t index = 0; index < tiles.size(); ++index)
      {
        auto &tile = tiles[index];

        float fx = static_cast<float>(index % map_size);
        float fy = static_cast<float>(index / map_size);

        tile.coord.x = static_cast<int>(fx);
        tile.coord.y = static_cast<int>(fy);

        tile.rect = {
          static_cast<float>(tile.coord.x) * tile_size,
          static_cast<float>(tile.coord.y) * tile_size,
          static_cast<float>(tile_size),
          static_cast<float>(tile_size)};

        float noise = 1.0f; // noise_generator->GenSingle2D(fx, fy, noise_seed);
        // noise = fabs(noise);

        // float noise = noise_map[y * MAP_SIZE + x]; // fbm_noise(fx * NOISE_SCALE, fy * NOISE_SCALE, noise_seed);
        // stb_perlin_noise3_seed(fx * NOISE_SCALE, fy * NOISE_SCALE, 0.0f, 0, 0, 0, noise_seed);
        //  noise = (noise + 1.0f) * 0.5f;

        if (noise < 0.5f)
          {
            tile.terrain = std::make_unique<Water>(Water::Depth::Shallow);
          }
        else
          {
            tile.terrain = std::make_unique<Land>(Land::Type::Dirt);
          }

        set_neighbors(tile);
      }

    // Second pass: assign bitmask sprite now that all tiles have types
    for (auto &tile : tiles)
      {
        if (tile.terrain->kind == TerrainKind::Liquid)
          {
            tile.sprite = get_bitmask(tile);
            // SDL_Log("sprite: {%f, %f, %f, %f}", tile.sprite.x, tile.sprite.y, tile.sprite.w, tile.sprite.h);
          }
        else
          {
            tile.sprite = {0, 0, tile_size, tile_size}; // fallback if needed
          }
      }
  }

};
