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

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "game.h"
#include "events.h"

static SDL_Color WHITE = {255, 255, 255, SDL_ALPHA_OPAQUE};
static SDL_Color DEFAULT_BACKGROUND_COLOR = {0x18, 0x18, 0x18, SDL_ALPHA_OPAQUE};

enum TileType : uint8_t
{
    Grass,
    Sand,
    Water,
    DeepWater,
    Unknown,
};

struct Tile
{
    TileType type;
    size_t index;
    SDL_FRect rect;
    SDL_FRect sprite;
    SDL_Point coord;
    std::array<int, 8> neighbors;
};

constexpr int TILE_SIZE = 32;
constexpr int MAP_SIZE = 50;
constexpr int WORLD_WIDTH = MAP_SIZE * TILE_SIZE;
constexpr int WORLD_HEIGHT = MAP_SIZE * TILE_SIZE;
constexpr int TILES_FLAT_SIZE = MAP_SIZE * MAP_SIZE;

constexpr Tile UnknownTile = {.type = TileType::Unknown};
static std::array<Tile, TILES_FLAT_SIZE> tiles_data;

SDL_Texture *tileset_grass;
static std::array<SDL_Point, 256> terrain_mask4_uv;

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

void initialize_terrain_mask()
{
    terrain_mask4_uv.fill({0, 0});

    terrain_mask4_uv[0] = {0, 0};
    terrain_mask4_uv[4] = {1, 0};
    terrain_mask4_uv[84] = {2, 0};
    terrain_mask4_uv[92] = {3, 0};
    terrain_mask4_uv[124] = {4, 0};
    terrain_mask4_uv[116] = {5, 0};
    terrain_mask4_uv[80] = {6, 0};
    terrain_mask4_uv[16] = {0, 1};
    terrain_mask4_uv[28] = {1, 1};
    terrain_mask4_uv[117] = {2, 1};
    terrain_mask4_uv[95] = {3, 1};
    terrain_mask4_uv[255] = {4, 1};
    terrain_mask4_uv[253] = {5, 1};
    terrain_mask4_uv[113] = {6, 1};
    terrain_mask4_uv[21] = {0, 2};
    terrain_mask4_uv[87] = {1, 2};
    terrain_mask4_uv[221] = {2, 2};
    terrain_mask4_uv[127] = {3, 2};
    // terrain_mask4_uv[255] = {4, 2};
    terrain_mask4_uv[247] = {5, 2};
    terrain_mask4_uv[209] = {6, 2};
    terrain_mask4_uv[29] = {0, 3};
    terrain_mask4_uv[125] = {1, 3};
    terrain_mask4_uv[119] = {2, 3};
    terrain_mask4_uv[199] = {3, 3};
    terrain_mask4_uv[215] = {4, 3};
    terrain_mask4_uv[213] = {5, 3};
    terrain_mask4_uv[81] = {6, 3};
    terrain_mask4_uv[31] = {0, 4};
    // terrain_mask4_uv[255] = {1, 4};
    terrain_mask4_uv[241] = {2, 4};
    terrain_mask4_uv[20] = {3, 4};
    terrain_mask4_uv[65] = {4, 4};
    terrain_mask4_uv[17] = {5, 4};
    terrain_mask4_uv[1] = {6, 4};
    terrain_mask4_uv[23] = {0, 5};
    terrain_mask4_uv[223] = {1, 5};
    terrain_mask4_uv[245] = {2, 5};
    terrain_mask4_uv[85] = {3, 5};
    terrain_mask4_uv[68] = {4, 5};
    terrain_mask4_uv[93] = {5, 5};
    terrain_mask4_uv[112] = {6, 5};
    terrain_mask4_uv[5] = {0, 6};
    terrain_mask4_uv[71] = {1, 6};
    terrain_mask4_uv[197] = {2, 6};
    terrain_mask4_uv[69] = {3, 6};
    terrain_mask4_uv[64] = {4, 6};
    terrain_mask4_uv[7] = {5, 6};
    terrain_mask4_uv[193] = {6, 6};
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
        return (tile.neighbors[i] < 0 || tile.type == tiles_data[tile.neighbors[i]].type);
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
    auto p = terrain_mask4_uv[mask];
    if (p.x == 0 && p.y == 0 && mask != 0)
    {
        SDL_Log("Missing UV for bitmask %d", mask);
    }

    return {(float)p.x * TILE_SIZE, (float)p.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
}

void initialize_map(int noise_seed = 8917237861)
{
    constexpr float NOISE_SCALE = 0.2f;

    for (size_t index = 0; index < tiles_data.size(); ++index)
    {
        auto &tile = tiles_data[index];

        float fx = index % MAP_SIZE;
        float fy = index / MAP_SIZE;

        tile.index = index;
        tile.coord.x = fx;
        tile.coord.y = fy;

        tile.rect = {
            static_cast<float>(tile.coord.x) * TILE_SIZE,
            static_cast<float>(tile.coord.y) * TILE_SIZE,
            static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)};

        float noise = stb_perlin_noise3_seed(fx * NOISE_SCALE, fy * NOISE_SCALE, 0.0f, 0, 0, 0, noise_seed);
        noise = (noise + 1.0f) * 0.5f;

        if (noise < 0.5f)
        {
            tile.type = TileType::Water;
        }
        else
        {
            tile.type = TileType::Grass;
        }

        set_neighbors(tile);
        // else if (noise < 0.6f)
        // {
        //     tile.type = TileType::Sand;
        // }
        // else
        // {
        //     tile.type = TileType::Grass;
        // }
    }

    // Second pass: assign bitmask sprite now that all tiles have types
    for (auto &tile : tiles_data)
    {
        if (tile.type == TileType::Grass)
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

Tile get_tile_from_coord(int x, int y)
{
    size_t index = y * MAP_SIZE + x;
    if (tiles_data.size() > index)
    {
        return tiles_data.at(index);
    }
    return UnknownTile;
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
    // @todo: add more context to tiles
    // static char buffer[128] = {0};
    // Text output = {0};

    // SDL_FRect cell = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

    // snprintf(buffer, sizeof buffer, "%zu", tile.index);
    // if (prepare_text(ctx, buffer, 12, WHITE, &output))
    // {
    //     output.rect.x = tile.rect.x;
    //     output.rect.y = tile.rect.y;
    //     render_text(ctx, &output);
    //     destroy_text(&output);
    // }

    // SDL_RenderTexture(ctx.renderer, tile.bg, NULL, &tile.rect);

    SDL_SetRenderDrawColor(ctx.renderer, 0xED, 0x6A, 0xFF, 255);
    SDL_RenderFillRect(ctx.renderer, &tile.rect);

    switch (tile.type)
    {
    case TileType::Water:
    {
        SDL_SetRenderDrawColor(ctx.renderer, 0xED, 0x6A, 0xFF, 255);
        SDL_RenderFillRect(ctx.renderer, &tile.rect);
        break;
    }
    case TileType::Unknown:
    {
        SDL_SetRenderDrawColor(ctx.renderer, 0xED, 0x6A, 0xFF, 255);
        SDL_RenderFillRect(ctx.renderer, &tile.rect);
        break;
    }

    default:
    {
        // SDL_RenderTexture(ctx.renderer, tileset, &tileset_map.at(tile.type), &tile.rect);
        SDL_RenderTexture(ctx.renderer, tileset_grass, &tile.sprite, &tile.rect);
        break;
    }
    }
}

void render_world(GameContext &ctx)
{
    SDL_SetRenderTarget(ctx.renderer, ctx.world_layer_texture);
    SDL_SetRenderDrawColor(
        ctx.renderer,
        DEFAULT_BACKGROUND_COLOR.r,
        DEFAULT_BACKGROUND_COLOR.g,
        DEFAULT_BACKGROUND_COLOR.b,
        DEFAULT_BACKGROUND_COLOR.a);

    SDL_RenderClear(ctx.renderer);

    // Apply world camera transformations to the renderer
    SDL_SetRenderScale(ctx.renderer, ctx.zoom_scale, ctx.zoom_scale);

    // Remember that SDL_RenderCopyEx takes source/destination rectangles.
    // You'll need to calculate these based on your worldCameraX/Y and worldCameraZoom.
    // For drawing individual world elements, you'd translate their coordinates:
    // SDL_FRect worldDestRect = { spriteX - worldCameraX, spriteY - worldCameraY, spriteWidth, spriteHeight };
    // SDL_RenderTexture(ctx.renderer, spriteTexture, NULL, &worldDestRect);
    // Or you can use SDL_SetRenderLogicalPresentation(ctx.renderer, width, height, SDL_RendererLogicalPresentationMode::SDL_LOGICAL_PRESENTATION_LINEAR);
    // and then just move the world's origin.

    for (auto &tile : tiles_data)
    {
        render_tile(ctx, tile);
    }

    // Reset scale if you only want it for the world texture.
    // SDL_SetRenderScale(ctx.renderer, 1.0f, 1.0f);

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

    int rw, rh;
    SDL_GetRenderOutputSize(ctx.renderer, &rw, &rh);

    // Draw the world layer (potentially with its own final scaling/positioning on the screen)
    // @fixme: do not render the entire world if it is out of the screen
    SDL_FRect world_display_rect = {ctx.camera_x, ctx.camera_y, (float)WORLD_WIDTH, (float)WORLD_HEIGHT};
    SDL_RenderTexture(ctx.renderer, ctx.world_layer_texture, NULL, &world_display_rect); // Blit the entire world texture to the screen

    // Present the final rendered frame
    // SDL_RenderPresent(ctx.renderer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    GameContext *ctx = new GameContext();
    *appstate = ctx;

    // @note: ensure that the game starts with the zoom scaled to 1
    ctx->zoom_scale = 1;
    ctx->last_time = SDL_GetTicks();

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

    // Load font
    const char *fontPath = "font.ttf";
    if (ctx->font = TTF_OpenFont(fontPath, 28); ctx->font == nullptr)
    {
        SDL_Log("Could not load %s! SDL_ttf Error: %s\n", fontPath, SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("BADI", 1024, 800, 0, &ctx->window, &ctx->renderer))
    {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetWindowResizable(ctx->window, false);
    SDL_SetWindowPosition(ctx->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // @todo: put it in a configuration
    if (SDL_SetRenderVSync(ctx->renderer, 1) == false)
    {
        SDL_Log("Could not enable VSync! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    ctx->world_layer_texture = SDL_CreateTexture(ctx->renderer,
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_TARGET,
                                                 WORLD_WIDTH,
                                                 WORLD_HEIGHT);
    if (!ctx->world_layer_texture)
    {
        SDL_Log("Could not create world layer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // @note: set it for transparency
    // SDL_SetTextureBlendMode(ctx->world_layer_texture, SDL_BLENDMODE_BLEND);

    if (!SDL_CaptureMouse(true))
    {
        SDL_Log("Could not capture mouse: %s\n", SDL_GetError());
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
    ImGui_ImplSDL3_InitForSDLRenderer(ctx->window, ctx->renderer);
    ImGui_ImplSDLRenderer3_Init(ctx->renderer);
    io.Fonts->AddFontDefault();

    tileset_grass = IMG_LoadTexture(ctx->renderer, "assets/demo.png");
    if (!tileset_grass)
    {
        SDL_Log("Couldn't load tileset_grass asset: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    initialize_terrain_mask();
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

    update_mouse(ctx);

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
    TTF_CloseFont(ctx->font);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(ctx->renderer);
    SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}