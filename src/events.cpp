#include "events.h"
#include <algorithm>

SDL_AppResult handle_events(Game &game, SDL_Event *event)
{
  switch (event->type)
  {
  case SDL_EVENT_QUIT:
  {
    return SDL_APP_SUCCESS; // Terminate app gracefully
  }

  case SDL_EVENT_WINDOW_RESIZED:
  {
    // SDL_GetWindowSize(game.window, &game.width, &game.height);

    // Handle resize here
    // handleWindowResize(event->window.windowID, event->window.data1, event->window.data2);
    break;
  }

  case SDL_EVENT_KEY_DOWN:
  {
    SDL_KeyboardEvent key = event->key;
    if (key.key == SDLK_Q)
    {
      return SDL_APP_SUCCESS; // Terminate app gracefully
    }
    else if (key.key == SDLK_G)
    {
      game.render_grid = !game.render_grid;
    }
    break;
  }

  case SDL_EVENT_MOUSE_WHEEL:
  {
    SDL_MouseWheelEvent wheel = event->wheel;
    // @todo: handle flipped SDL_MOUSEWHEEL_FLIPPED
    if (wheel.direction == SDL_MOUSEWHEEL_NORMAL && wheel.y != 0)
    {
      game.handle_mouse_wheel(wheel.integer_x, wheel.integer_y, wheel.y);
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE && mouse.down)
    {
      game.snapping = true;
      game.snap_offset.x = mouse.x;
      game.snap_offset.y = mouse.y;
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_UP:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE)
    {
      game.snapping = false;
      game.snap_offset.x = 0;
      game.snap_offset.y = 0;
    }
    break;
  }

  case SDL_EVENT_MOUSE_MOTION:
  {
    SDL_MouseMotionEvent &motion = event->motion;
    if (game.snapping)
    {
      game.handle_snapping(motion);
    }
    SDL_FPoint p = {motion.x, motion.y};
    if (SDL_PointInRectFloat(&p, &game.viewport))
    {
      // game.handle_mouse_over(game.renderer, event);
    }
    game.mouse_position.x = motion.x;
    game.mouse_position.y = motion.y;
    break;
  }
  default:
  {
    break;
  }
  }

  return SDL_APP_CONTINUE;
}
