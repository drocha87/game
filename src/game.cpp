#include "game.h"

bool Game::create_world()
{
  if (!assets.load_texture(renderer, "bg",    "assets/bg.png"))            return false;
  if (!assets.load_texture(renderer, "grass", "assets/tileset_grass.png")) return false;
  if (!assets.load_texture(renderer, "frame", "assets/frame.png"))         return false;
  
  world_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
  // @note: set it for transparency
  // SDL_SetTextureBlendMode(ctx->world_texture, SDL_BLENDMODE_BLEND);
  
  if (!world_texture) {
    SDL_Log("Failed to create world canvas: %s", SDL_GetError());
    return false;
  }
  
  viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
  prev_time = SDL_GetPerformanceCounter();
  curr_time = prev_time;

  return true;      
}

void Game::initialize_map(int noise_seed)
{
  auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
  auto noise_generator = FastNoise::New<FastNoise::DomainScale>();

  // fnSimplex->GenUniformGrid2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 0.09f, seed);
  noise_generator->SetSource(fnSimplex);
  // noise_generator->SetGain(0.6f);
  // noise_generator->SetLacunarity(2.0f);
  // noise_generator->SetOctaveCount(4);
  noise_generator->SetScale(0.2f);
    
  float noise_map[MAP_SIZE * MAP_SIZE] = {0};
  noise_generator->GenUniformGrid2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 1.0f, noise_seed);
  // noise_generator->GenSingle2D(noise_map, 0, 0, MAP_SIZE, MAP_SIZE, 1.0f, noise_seed);

  for (size_t index = 0; index < tiles.size(); ++index)
    {
      auto &tile = tiles[index];

      float fx = static_cast<float>(index % MAP_SIZE);
      float fy = static_cast<float>(index / MAP_SIZE);

      tile.coord.x = static_cast<int>(fx);
      tile.coord.y = static_cast<int>(fy);

      tile.rect = {
        static_cast<float>(tile.coord.x) * TILE_SIZE,
        static_cast<float>(tile.coord.y) * TILE_SIZE,
        static_cast<float>(TILE_SIZE),
        static_cast<float>(TILE_SIZE)};

      // float noise = noise_generator->GenSingle2D(fx, fy, noise_seed);
      float noise = noise_map[index];
      noise = fabs(noise);

      if (noise <= 0.2f)
        {
          tile.kind = TerrainKind::Crust;
        }
      // else if (noise < 0.6f)
      //   {
      //     tile.kind = TerrainKind::Dirt;
      //   }
      else
        {
          tile.kind = TerrainKind::Grass;
        }
      tile.selected = false;

      // set_neighbors(tile);
    }

  // Second pass: assign bitmask sprite now that all tiles have types
  // for (auto &tile : tiles)
  //   {
  //     if (tile.kind == TerrainKind::Water)
  //       {
  //         tile.sprite = get_bitmask(tile);
  //         // SDL_Log("sprite: {%f, %f, %f, %f}", tile.sprite.x, tile.sprite.y, tile.sprite.w, tile.sprite.h);
  //       }
  //     else
  //       {
  //         tile.sprite = {0, 0, TILE_SIZE, TILE_SIZE}; // fallback if needed
  //       }
  //   }
}

SDL_FPoint Game::screen_to_world(SDL_FPoint screen_point) const
{
  float x = (screen_point.x - viewport.x) / zoom;
  float y = (screen_point.y - viewport.y) / zoom;
  return {x+0.2f, y+0.2f};
}

void Game::handle_mouse_wheel(int mouse_screen_x, int mouse_screen_y, float wheel_y)
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

void Game::handle_snapping(SDL_MouseMotionEvent &motion)
{
  int dx = static_cast<int>(motion.x - snap_offset.x);
  int dy = static_cast<int>(motion.y - snap_offset.y);

  viewport.x += dx;
  viewport.y += dy;

  snap_offset.x = motion.x;
  snap_offset.y = motion.y;
}

SDL_AppResult Game::render()
{
  /* SDL_SetRenderDrawColor(renderer, DEFAULT_BACKGROUND_COLOR.r, DEFAULT_BACKGROUND_COLOR.g, DEFAULT_BACKGROUND_COLOR.b, DEFAULT_BACKGROUND_COLOR.a); */
  /* SDL_RenderClear(renderer); */

  SDL_SetRenderTarget(renderer, world_texture);
  SDL_SetRenderScale(renderer, zoom, zoom);

  // @fixme: I think we could avoid this RenderClear since we will render in the entire texture anyway
  SDL_SetRenderDrawColor(renderer, SDL_COLOR_RGBA(TERRAIN_COLORS[TerrainKind::Crust]));
  // SDL_RenderTexture(renderer, bg, nullptr, &viewport);
  SDL_RenderClear(renderer);

  const SDL_FPoint point = screen_to_world(mouse_position);

  for (auto &tile : tiles)
    {
      auto mouse_hover = SDL_PointInRectFloat(&point, &tile.rect);
      if (mouse_hover) {
        tile_on_mouse = &tile;
      }
      tile.render(renderer, tiles, mouse_hover, assets);
    }

  if (render_grid)
    {
      SDL_SetRenderDrawColor(renderer, 0xfa, 0xfa, 0xfa, 0xff);
      for (int i = 0; i < MAP_SIZE; i++)
        {
          // render horizontal grid line
          float x = 0.0f;
          float y = static_cast<float>(i * TILE_SIZE);
          SDL_RenderLine(renderer, x, y, MAP_SIZE * TILE_SIZE, y);

          // render vertical grid line
          x = static_cast<float>(i * TILE_SIZE);
          y = 0;
          SDL_RenderLine(renderer, x, y, x, MAP_SIZE * TILE_SIZE);
        }
    }

  // 2. Reset Render Target to Window
  SDL_SetRenderTarget(renderer, NULL);

  // 3. Compose everything on the main renderer
  SDL_SetRenderDrawColor(renderer, SDL_COLOR_RGBA(TERRAIN_COLORS[TerrainKind::Crust]));
  SDL_RenderClear(renderer);
    
  SDL_RenderTexture(renderer, world_texture, NULL, &viewport);

  render_fps();
    
  // Present the final rendered frame
  SDL_RenderPresent(renderer);
    
  return SDL_APP_CONTINUE;
}

void Game::render_fps()
{
  static char buffer[128] = {0};
  static Uint64 fps = 0;
  static double elapsed = 0;
    
  double frame_time = (curr_time - prev_time) / frequency;
  elapsed += frame_time;

  if (elapsed >= 2.0f)
    {
      elapsed = 0;
      fps = (Uint32)(1 / frame_time);
    }

  int rw, rh;
  SDL_GetCurrentRenderOutputSize(renderer, &rw, &rh);

  snprintf(buffer, sizeof buffer, "FFPS: %zu, tile: (%d, %d)", fps, tile_on_mouse->coord.x, tile_on_mouse->coord.y);
  Text t = {0};
  if (prepare_text(buffer, 12, WHITE, &t))
    {
      t.rect.x = rw - t.rect.w - 10;
      t.rect.y = rh - t.rect.h - 10;
      render_text(&t);
      destroy_text(&t);
    }
}
  
bool Game::prepare_text(const char *text, float size, SDL_Color color, Text *output)
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

void Game::destroy_text(Text *text)
{
  SDL_DestroySurface(text->surface);
}

void Game::render_text(Text *text)
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


