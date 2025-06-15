#pragma once

#include <SDL3/SDL.h>

#include <backends/imgui_impl_sdl3.h>

#include "game.h"

SDL_AppResult handle_events(GameContext &ctx, SDL_Event *event);
