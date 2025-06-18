#include <string>
#include <algorithm>
#include <format>
#include <array>

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <FastNoise/FastNoise.h>

// #define STB_PERLIN_IMPLEMENTATION
// #include "stb_perlin.h"

#include "game.h"
#include "events.h"
#include "terrain.h"
#include <random>

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

constexpr int TILE_SIZE = 32;
constexpr int MAP_SIZE = 10;
constexpr int WORLD_WIDTH = MAP_SIZE * TILE_SIZE;
constexpr int WORLD_HEIGHT = MAP_SIZE * TILE_SIZE;

static std::array<Tile, MAP_SIZE * MAP_SIZE> tiles_data;

SDL_Texture *tileset_grass;

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

        if (nx >= 0 && ny >= 0 && nx < MAP_SIZE && ny < MAP_SIZE)
        {
            tile.neighbors[i] = ny * MAP_SIZE + nx;
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
        return (tile.neighbors[i] < 0 || tile.terrain->kind == tiles_data[tile.neighbors[i]].terrain->kind);
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

// static float noise_map[MAP_SIZE * MAP_SIZE] = {};

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

    for (size_t index = 0; index < tiles_data.size(); ++index)
    {
        auto &tile = tiles_data[index];

        float fx = index % MAP_SIZE;
        float fy = index / MAP_SIZE;

        tile.coord.x = fx;
        tile.coord.y = fy;

        tile.rect = {
            static_cast<float>(tile.coord.x) * TILE_SIZE,
            static_cast<float>(tile.coord.y) * TILE_SIZE,
            static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)};

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
    for (auto &tile : tiles_data)
    {
        if (tile.terrain->kind == TerrainKind::Liquid)
        {
            tile.sprite = get_bitmask(tile);
            // SDL_Log("sprite: {%f, %f, %f, %f}", tile.sprite.x, tile.sprite.y, tile.sprite.w, tile.sprite.h);
        }
        else
        {
            tile.sprite = {0, 0, TILE_SIZE, TILE_SIZE}; // fallback if needed
        }
    }

    // for (size_t index = 0; index < tiles_data.size(); ++index)
    // {
    //     auto &tile = tiles_data[index];
    //     if (tile.type == TileType::Water)
    //     {
    //         bool dw = true;
    //         for (auto n : tile.neighbors)
    //         {
    //             if (n >= 0 && tiles_data[n].type != TileType::Water)
    //             {
    //                 dw = false;
    //                 break;
    //             }
    //         }
    //         if (dw)
    //         {
    //             tile.type = TileType::DeepWater;
    //         }
    //     }
    // }
}

void update_mouse(GameContext *ctx)
{
    // get the mouse position related to the screen and fix the sub-precision before updating it on the game_context (ctx)
    float x, y;
    ctx->mouse_flags = SDL_GetMouseState(&x, &y);
    ctx->mouse_point.x = x + 0.5f;
    ctx->mouse_point.y = y + 0.5f;
}

struct Text
{
    SDL_FRect rect;
    SDL_Surface *surface;
};

bool prepare_text(GameContext &ctx, const char *text, size_t size, SDL_Color color, Text *output)
{
    TTF_SetFontSize(ctx.font, size);

    int w, h;
    if (TTF_GetStringSize(ctx.font, text, 0, &w, &h))
    {
        output->rect = SDL_FRect{.w = (float)w, .h = (float)h};
        output->surface = TTF_RenderText_Blended(ctx.font, text, 0, color);
        return true;
    }

    SDL_Log("Couldn't prepare text SDL: %s\n", SDL_GetError());
    return false;
}

void destroy_text(Text *text)
{
    SDL_DestroySurface(text->surface);
}

void render_text(GameContext &ctx, Text *text)
{
    if (text->surface)
    {
        auto texture = SDL_CreateTextureFromSurface(ctx.renderer, text->surface);
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
            SDL_RenderTexture(ctx.renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
    }
}

void render_fps(GameContext &ctx)
{
    static Uint32 fps = 0, elapsed = 0;
    static char buffer[128] = {0};

    Uint32 current_time, frame_time;

    current_time = SDL_GetTicks();
    frame_time = current_time - ctx.last_time;
    ctx.last_time = current_time;

    elapsed += frame_time;

    if (elapsed >= 5000)
    {
        elapsed = 0;
        fps = (Uint32)(1000.0f / frame_time);
    }

    int rw, rh;
    SDL_GetCurrentRenderOutputSize(ctx.renderer, &rw, &rh);

    snprintf(buffer, sizeof buffer, "FPS: %f", fps);
    Text t = {0};
    if (prepare_text(ctx, buffer, 12, WHITE, &t))
    {
        t.rect.x = rw - t.rect.w - 10;
        t.rect.y = rh - t.rect.h - 10;
        render_text(ctx, &t);
        destroy_text(&t);
    }
}

void render_tile(GameContext &ctx, const Tile &tile)
{
    const SDL_FPoint point = ctx.world_camera->screen_to_world(ctx.mouse_point);

    if (SDL_PointInRectFloat(&point, &tile.rect))
    {
        SDL_Log("(screen x = %f, y = %f)", point.x, point.y);
        SDL_SetRenderDrawColor(ctx.renderer, 0xFF, 0x00, 0x00, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(ctx.renderer, 0xED, 0x6A, 0xFF, 255);
    }
    SDL_RenderFillRect(ctx.renderer, &tile.rect);
    // if (tile.terrain->kind == TerrainKind::Liquid)
    // {
    //     SDL_RenderTexture(ctx.renderer, tileset_grass, &tile.sprite, &tile.rect);
    // }
}

void render_world(GameContext &ctx)
{
    SDL_SetRenderTarget(ctx.renderer, ctx.world_layer_texture);
    // Apply world camera transformations to the renderer
    SDL_SetRenderScale(ctx.renderer, ctx.world_camera->zoom, ctx.world_camera->zoom);

    // @fixme: I think we could avoid this RenderClear since we will render in the entire texture anyway
    SDL_SetRenderDrawColor(
        ctx.renderer,
        DEFAULT_BACKGROUND_COLOR.r,
        DEFAULT_BACKGROUND_COLOR.g,
        DEFAULT_BACKGROUND_COLOR.b,
        DEFAULT_BACKGROUND_COLOR.a);

    SDL_RenderClear(ctx.renderer);

    for (auto &tile : tiles_data)
    {
        render_tile(ctx, tile);
    }

    if (ctx.world_camera->render_grid)
    {
        SDL_SetRenderDrawColor(ctx.renderer, 0xfa, 0xfa, 0xfa, 0xff);
        for (int i = 0; i < MAP_SIZE; i++)
        {
            // render horizontal grid line
            float x = 0;
            float y = i * TILE_SIZE;
            SDL_RenderLine(ctx.renderer, x, y, MAP_SIZE * TILE_SIZE, y);

            // render vertical grid line
            x = i * TILE_SIZE;
            y = 0;
            SDL_RenderLine(ctx.renderer, x, y, x, MAP_SIZE * TILE_SIZE);
        }
    }

    // 2. Reset Render Target to Window
    SDL_SetRenderTarget(ctx.renderer, NULL);

    // 3. Compose everything on the main renderer
    SDL_SetRenderDrawColor(
        ctx.renderer,
        DEFAULT_BACKGROUND_COLOR.r,
        DEFAULT_BACKGROUND_COLOR.g,
        DEFAULT_BACKGROUND_COLOR.b,
        DEFAULT_BACKGROUND_COLOR.a);

    SDL_RenderClear(ctx.renderer);

    SDL_RenderTexture(ctx.renderer, ctx.world_layer_texture, NULL, &ctx.world_camera->viewport);
    // Present the final rendered frame
    // SDL_RenderPresent(ctx.renderer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  int screen_width = 1024;
  int screen_height = 800;

  if (!SDL_Init(SDL_INIT_VIDEO))
    {
      SDL_Log("Couldn't initialise SDL: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  if (!TTF_Init())
    {
      SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  if (!SDL_CreateWindowAndRenderer("Diego Rocha", screen_width, screen_height, 0, &window, &renderer))
    {
      SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  SDL_SetWindowResizable(window, false);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  // @todo: put it in a configuration
  if (SDL_SetRenderVSync(renderer, 1) == false)
    {
      SDL_Log("Could not enable VSync! SDL error: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  if (!SDL_CaptureMouse(true))
    {
      SDL_Log("Could not capture mouse: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  const char *fontPath = "font.ttf";
  TTF_Font *font = TTF_OpenFont(fontPath, 28);
  if (!font)
    {
      SDL_Log("Could not load %s! SDL_ttf Error: %s\n", fontPath, SDL_GetError());
      return SDL_APP_FAILURE;
    }

  // setup ImGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
  io.Fonts->AddFontDefault();

  auto world_layer_texture = SDL_CreateTexture(renderer,
                                               SDL_PIXELFORMAT_RGBA8888,
                                               SDL_TEXTUREACCESS_TARGET,
                                               WORLD_WIDTH,
                                               WORLD_HEIGHT);

  if (!world_layer_texture)
    {
      SDL_Log("Could not create world layer: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }
  // @note: set it for transparency
  // SDL_SetTextureBlendMode(ctx->world_layer_texture, SDL_BLENDMODE_BLEND);

  auto game_context = new GameContext(screen_width, screen_height, window, renderer, world_layer_texture, font);
  *appstate = game_context;
  if (!game_context->load_assets())
    {
      SDL_Log("Couldn't load tileset_grass asset: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  // initialize_terrain_mask();
  initialize_map();

  // Initialize SDL, create window/renderer, load assets
  // Set up *appstate if you want to avoid global variables
  return SDL_APP_CONTINUE; // Or SDL_APP_FAILURE on error
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    GameContext *ctx = static_cast<GameContext *>(appstate);
    if (ctx)
    {
        return handle_events(*ctx, event);
    }

    // if the GameContext is not valid there's no reason to keep going
    return SDL_APP_FAILURE;
}

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

SDL_AppResult SDL_AppIterate(void *appstate)
{
    GameContext *ctx = static_cast<GameContext *>(appstate);

    SDL_SetRenderDrawColor(
        ctx->renderer,
        DEFAULT_BACKGROUND_COLOR.r,
        DEFAULT_BACKGROUND_COLOR.g,
        DEFAULT_BACKGROUND_COLOR.b,
        DEFAULT_BACKGROUND_COLOR.a);

    SDL_RenderClear(ctx->renderer);

    // if (ctx->should_scale)
    // {
    //     ctx->zoom_scale = std::clamp(ctx->zoom_scale, 1.0f, 5.0f);
    //     SDL_SetRenderScale(ctx->renderer, ctx->zoom_scale, ctx->zoom_scale);
    //     ctx->should_scale = false;
    // }

    // update_mouse(ctx);

    // if (0)
    // {
    //     SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); // Grid color

    //     for (int y = 0; y < GRID_HEIGHT; y++)
    //     {
    //         for (int x = 0; x < GRID_WIDTH; x++)
    //         {
    //             SDL_FRect cell = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    //             bool mouse_in_cell = SDL_PointInRectFloat(&ctx->mouse_point, &cell);
    //             if (mouse_in_cell)
    //             {
    //                 if (ctx->mouse_flags & SDL_BUTTON_LMASK)
    //                 {
    //                     SDL_SetRenderDrawColor(ctx->renderer, 100, 0, 100, 255);
    //                     SDL_RenderFillRect(ctx->renderer, &cell);
    //                     SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); // Grid color
    //                 }
    //                 else
    //                 {
    //                     SDL_RenderFillRect(ctx->renderer, &cell);
    //                 }
    //             }
    //             else
    //             {
    //                 SDL_RenderRect(ctx->renderer, &cell);
    //             }
    //         }
    //     }
    // }

    if (0)
    {
        /* Load the icon */
        SDL_Texture *texture = IMG_LoadTexture(ctx->renderer, "tile_frame.png");
        if (!texture)
        {
            SDL_Log("Couldn't load icon: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }
        else
        {
            SDL_FRect dst;
            SDL_GetTextureSize(texture, &dst.w, &dst.h);
            dst.x = 100; //((w / scale) - dst.w) / 2;
            dst.y = 100; //((h / scale) - dst.h) / 2;

            SDL_RenderTexture(ctx->renderer, texture, NULL, &dst);
        }
    }

    // render_fps(ctx);

    render_world(*ctx);

    // Start the Dear ImGui frame
    // ImGui_ImplSDLRenderer3_NewFrame();
    // ImGui_ImplSDL3_NewFrame();
    // ImGui::NewFrame();
    // ImGui::ShowDemoWindow(&show_demo_window);
    // ImGui::Render();

    // ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ctx->renderer);

    SDL_RenderPresent(ctx->renderer);

    return SDL_APP_CONTINUE; // Or SDL_APP_FAILURE
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    GameContext *ctx = static_cast<GameContext *>(appstate);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (ctx)
    {
        TTF_CloseFont(ctx->font);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
    }

    TTF_Quit();
    SDL_Quit();
}
