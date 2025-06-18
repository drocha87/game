#include "events.h"
#include <algorithm>

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
    SDL_GetWindowSize(ctx.window, &ctx.screen_width, &ctx.screen_height);

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
      ctx.world_camera->render_grid = !ctx.world_camera->render_grid;
    }
    break;
  }

  case SDL_EVENT_MOUSE_WHEEL:
  {
    SDL_MouseWheelEvent wheel = event->wheel;
    // @todo: handle flipped SDL_MOUSEWHEEL_FLIPPED
    if (wheel.direction == SDL_MOUSEWHEEL_NORMAL && wheel.y != 0)
    {
      ctx.world_camera->handle_mouse_wheel(wheel.integer_x, wheel.integer_y, wheel.y);

      // ctx.world_camera.prev_zoom = ctx.world_camera.zoom;
      // ctx.world_camera.zoom += wheel.y < 0 ? -0.1f : 0.1f;
      // ctx.world_camera.zoom = std::clamp(ctx.world_camera.zoom, 0.50f, 3.0f);

      // ctx.world_camera.zoom_at_cursor(ctx.mouse_point);
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE && mouse.down)
    {
      ctx.world_camera->snapping = true;
      ctx.world_camera->snap_offset.x = mouse.x;
      ctx.world_camera->snap_offset.y = mouse.y;
    }
    break;
  }

  case SDL_EVENT_MOUSE_BUTTON_UP:
  {
    SDL_MouseButtonEvent &mouse = event->button;
    if (mouse.button == SDL_BUTTON_MIDDLE)
    {
      ctx.world_camera->snapping = false;
      ctx.world_camera->snap_offset.x = 0;
      ctx.world_camera->snap_offset.y = 0;
    }
    break;
  }

  case SDL_EVENT_MOUSE_MOTION:
  {
    SDL_MouseMotionEvent &motion = event->motion;
    if (ctx.world_camera->snapping)
    {
      ctx.world_camera->handle_snapping(motion);
    }
    SDL_FPoint p = {motion.x, motion.y};
    if (SDL_PointInRectFloat(&p, &ctx.world_camera->viewport))
    {
      ctx.world_camera->handle_mouse_over(ctx.renderer, event);
    }
    ctx.mouse_point.x = motion.x;
    ctx.mouse_point.y = motion.y;
    break;
  }
  default:
  {
    break;
  }
  }

  return SDL_APP_CONTINUE;
}