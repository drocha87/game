#include <string>
#include <algorithm>
#include <format>
#include <array>

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

const int TILE_SIZE = 48;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 10;

static SDL_Color WHITE = {255, 255, 255, SDL_ALPHA_OPAQUE};

enum TileType : uint8_t
{
    Grass,
    Sand,
    Water,
    Unknown,
};

struct Tile
{
    TileType type;
    size_t index;
};

static const Tile UnknownTile = {.type = TileType::Unknown};
static const size_t MAP_SIZE = 128;
static std::array<Tile, MAP_SIZE * MAP_SIZE> tiles;

void initialize_map()
{
    size_t index = 0;
    for (auto &tile : tiles)
    {
        tile.type = TileType::Water;
        tile.index = index;
        index++;
    }
}

Tile get_tile_from_coord(int x, int y)
{
    size_t index = y * MAP_SIZE + x;
    if (tiles.size() > index)
    {
        return tiles.at(index);
    }
    return UnknownTile;
}

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
};

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

bool prepare_text(GameContext *ctx, const char *text, size_t size, SDL_Color color, Text *output)
{
    TTF_SetFontSize(ctx->font, size);

    int w, h;
    if (TTF_GetStringSize(ctx->font, text, 0, &w, &h))
    {
        output->rect = SDL_FRect{.w = (float)w, .h = (float)h};
        output->surface = TTF_RenderText_Blended(ctx->font, text, 0, color);
        return true;
    }

    SDL_Log("Couldn't prepare text SDL: %s\n", SDL_GetError());
    return false;
}

void destroy_text(Text *text)
{
    SDL_DestroySurface(text->surface);
}

void render_text(GameContext *ctx, Text *text)
{
    if (text->surface)
    {
        auto texture = SDL_CreateTextureFromSurface(ctx->renderer, text->surface);
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
            SDL_RenderTexture(ctx->renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
    }
}

void render_fps(GameContext *ctx)
{
    static Uint32 fps = 0, elapsed = 0;
    static char buffer[128] = {0};

    Uint32 current_time, frame_time;

    current_time = SDL_GetTicks();
    frame_time = current_time - ctx->last_time;
    ctx->last_time = current_time;

    elapsed += frame_time;

    if (elapsed >= 5000)
    {
        elapsed = 0;
        fps = (Uint32)(1000.0f / frame_time);
    }

    int rw, rh;
    SDL_GetCurrentRenderOutputSize(ctx->renderer, &rw, &rh);

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

void render_tile(GameContext *ctx, Tile tile, int x, int y)
{
    static char buffer[128] = {0};
    Text output = {0};

    SDL_FRect cell = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

    snprintf(buffer, sizeof buffer, "%zu", tile.index);
    if (prepare_text(ctx, buffer, 12, WHITE, &output))
    {
        output.rect.x = cell.x;
        output.rect.y = cell.y;
        render_text(ctx, &output);
        destroy_text(&output);
    }

    switch (tile.type)
    {
    case TileType::Grass:
    {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 100, 0, 255);
        break;
    }
    case TileType::Water:
    {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 100, 255);
        break;
    }
    default:
    {
        SDL_SetRenderDrawColor(ctx->renderer, 100, 0, 0, 255);
        break;
    }
    }

    SDL_RenderRect(ctx->renderer, &cell);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    initialize_map();

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

    SDL_SetWindowResizable(ctx->window, true);

    // @todo: put it in a configuration
    if (SDL_SetRenderVSync(ctx->renderer, 1) == false)
    {
        SDL_Log("Could not enable VSync! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CaptureMouse(true))
    {
        SDL_Log("Could not capture mouse: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // Initialize SDL, create window/renderer, load assets
    // Set up *appstate if you want to avoid global variables
    return SDL_APP_CONTINUE; // Or SDL_APP_FAILURE on error
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; // Terminate app gracefully
    }
    else if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        // Handle resize here
        // handleWindowResize(event->window.windowID, event->window.data1, event->window.data2);
    }
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        SDL_MouseWheelEvent wheel = event->wheel;
        // @todo: handle flipped SDL_MOUSEWHEEL_FLIPPED
        if (wheel.direction == SDL_MOUSEWHEEL_NORMAL && wheel.y != 0)
        {
            GameContext *ctx = static_cast<GameContext *>(appstate);
            ctx->zoom_scale *= wheel.y < 0 ? 0.9f : 1.1f;
            ctx->should_scale = true;
        }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    GameContext *ctx = static_cast<GameContext *>(appstate);

    SDL_SetRenderDrawColor(ctx->renderer, 0x18, 0x18, 0x18, 255);
    SDL_RenderClear(ctx->renderer);

    if (ctx->should_scale)
    {
        ctx->zoom_scale = std::clamp(ctx->zoom_scale, 1.0f, 5.0f);
        SDL_SetRenderScale(ctx->renderer, ctx->zoom_scale, ctx->zoom_scale);
        ctx->should_scale = false;
    }

    update_mouse(ctx);

    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            Tile tile = get_tile_from_coord(x, y);
            render_tile(ctx, tile, x, y);
        }
    }

    if (0)
    {
        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); // Grid color

        for (int y = 0; y < GRID_HEIGHT; y++)
        {
            for (int x = 0; x < GRID_WIDTH; x++)
            {
                SDL_FRect cell = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                bool mouse_in_cell = SDL_PointInRectFloat(&ctx->mouse_point, &cell);
                if (mouse_in_cell)
                {
                    if (ctx->mouse_flags & SDL_BUTTON_LMASK)
                    {
                        SDL_SetRenderDrawColor(ctx->renderer, 100, 0, 100, 255);
                        SDL_RenderFillRect(ctx->renderer, &cell);
                        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255); // Grid color
                    }
                    else
                    {
                        SDL_RenderFillRect(ctx->renderer, &cell);
                    }
                }
                else
                {
                    SDL_RenderRect(ctx->renderer, &cell);
                }
            }
        }
    }

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

    SDL_RenderPresent(ctx->renderer);

    return SDL_APP_CONTINUE; // Or SDL_APP_FAILURE
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    GameContext *ctx = static_cast<GameContext *>(appstate);
    TTF_CloseFont(ctx->font);

    SDL_DestroyRenderer(ctx->renderer);
    SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}