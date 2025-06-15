#include "events.h"

SDL_AppResult handle_events(GameContext &ctx, SDL_Event *event)
{
  ImGui_ImplSDL3_ProcessEvent(event);

  switch (event->type)
  {
  case SDL_EVENT_QUIT:
  {
    return SDL_APP_SUCCESS; // Terminate app gracefully
  }

  case SDL_EVENT_WINDOW_RESIZED:
  {
    // Handle resize here
    // handleWindowResize(event->window.windowID, event->window.data1, event->window.data2);
    break;
  }

  case SDL_EVENT_MOUSE_WHEEL:
  {
    SDL_MouseWheelEvent wheel = event->wheel;
    // @todo: handle flipped SDL_MOUSEWHEEL_FLIPPED
    if (wheel.direction == SDL_MOUSEWHEEL_NORMAL && wheel.y != 0)
    {
      ctx.zoom_scale *= wheel.y < 0 ? 0.9f : 1.1f;
      ctx.should_scale = true;
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE && mouse.down)
    {
      ctx.dragging = true;
      ctx.last_mouse_x = mouse.x;
      ctx.last_mouse_y = mouse.y;
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_UP:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE)
    {
      ctx.dragging = false;
    }
    break;
  }

  case SDL_EVENT_MOUSE_MOTION:
  {
    SDL_MouseMotionEvent &motion = event->motion;
    if (ctx.dragging)
    {
      int dx = motion.x - ctx.last_mouse_x;
      int dy = motion.y - ctx.last_mouse_y;

      ctx.camera_x += dx;
      ctx.camera_y += dy;

      ctx.last_mouse_x = motion.x;
      ctx.last_mouse_y = motion.y;
    }
    break;
  }
  default:
  {
    break;
  }
  }

  return SDL_APP_CONTINUE;
}