#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.h"
#include "events.h"

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
  
  if (!SDL_CreateWindowAndRenderer("Terraplana", screen_width, screen_height, 0, &window, &renderer))
    {
      SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }

  SDL_SetWindowResizable(window, false);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  // @todo: put it in a configuration
  // if (SDL_SetRenderVSync(renderer, 1) == false)
  //   {
  //     SDL_Log("Could not enable VSync! SDL error: %s\n", SDL_GetError());
  //     return SDL_APP_FAILURE;
  //   }
  
  //SDL_SetRenderLogicalPresentation(renderer, logical_width, logical_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  
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

  Game *game = new Game(window, renderer, font);
  if (!game->load_assets())
    {

      SDL_Log("Couldn't load tileset_grass asset: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }
  
  if (!game->create_world())
    {
      SDL_Log("Couldn't create game world: %s\n", SDL_GetError());
      return SDL_APP_FAILURE;
    }      

  // initialize_terrain_mask();
  game->initialize_map();

  *appstate = game;
  
  // Initialize SDL, create window/renderer, load assets
  // Set up *appstate if you want to avoid global variables
  return SDL_APP_CONTINUE; // Or SDL_APP_FAILURE on error
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  Game *game = static_cast<Game *>(appstate);
  if (game)
    {
      return handle_events(*game, event);
    }

  return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  Game *game = static_cast<Game *>(appstate);
  
  game->prev_time = game->curr_time;
  game->curr_time = SDL_GetPerformanceCounter();
  game->delta_time = (double)(game->prev_time - game->curr_time) / game->frequency;
  
  auto result = game->render();
  if (result != SDL_APP_CONTINUE) return result;

  double frame_time = (double)(SDL_GetPerformanceCounter() - game->curr_time) / game->frequency;
  // SDL_Log("frame took %fms", frame_time); 
  if (frame_time < game->TARGET_FRAME_TIME) {
    SDL_Delay((Uint32)((game->TARGET_FRAME_TIME - frame_time) * 1000.0));
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  Game *game = static_cast<Game *>(appstate);

  if (game)
    {
      TTF_CloseFont(game->font);
      SDL_DestroyRenderer(game->renderer);
      SDL_DestroyWindow(game->window);
    }

  TTF_Quit();
  SDL_Quit();
}
