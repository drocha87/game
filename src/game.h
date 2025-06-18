#pragma once

#include <algorithm>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

// struct Camera
// {
//   SDL_FRect screen_rect;   // Where the world is drawn on screen
//   SDL_FPoint camera_pos{}; // Top-left position in world coordinates (camera offset)
//   float zoom = 1.0f;
//   float prev_zoom = 1.0f;

//   bool render_grid = false;
//   bool snapping = false;
//   SDL_FPoint snap_offset = {0};

//   Camera(const SDL_FRect &screen_area, float initialZoom)
//       : screen_rect(screen_area), zoom(initialZoom), prev_zoom(initialZoom) {}

//   // Converts screen point to world coordinates
//   SDL_FPoint screen_to_world(SDL_FPoint screen_point) const
//   {
//     float local_x = (screen_point.x - screen_rect.x) / zoom;
//     float local_y = (screen_point.y - screen_rect.y) / zoom;
//     return {
//         local_x + camera_pos.x,
//         local_y + camera_pos.y};
//   }

//   // Call before changing zoom
//   void apply_zoom_at_cursor(int mouse_x, int mouse_y)
//   {
//     // Convert mouse to world before zoom
//     float world_x_before = (mouse_x - screen_rect.x) / prev_zoom + camera_pos.x;
//     float world_y_before = (mouse_y - screen_rect.y) / prev_zoom + camera_pos.y;

//     // Convert mouse to world after zoom
//     float world_x_after = (mouse_x - screen_rect.x) / zoom;
//     float world_y_after = (mouse_y - screen_rect.y) / zoom;

//     // Adjust camera so world point under cursor remains the same
//     camera_pos.x = world_x_before - world_x_after;
//     camera_pos.y = world_y_before - world_y_after;
//   }

//   void handle_mouse_wheel(int mouse_x, int mouse_y, float wheel_y)
//   {
//     float new_zoom = zoom;
//     float zoom_step = 1.1f;

//     if (wheel_y > 0)
//       new_zoom *= zoom_step;
//     else if (wheel_y < 0)
//       new_zoom /= zoom_step;

//     new_zoom = std::clamp(new_zoom, 0.5f, 4.0f);
//     if (new_zoom == zoom)
//       return;

//     prev_zoom = zoom;
//     zoom = new_zoom;
//     apply_zoom_at_cursor(mouse_x, mouse_y);
//   }

//   void handle_snapping(SDL_MouseMotionEvent &motion)
//   {
//     int dx = motion.x - snap_offset.x;
//     int dy = motion.y - snap_offset.y;

//     camera_pos.x -= dx / zoom;
//     camera_pos.y -= dy / zoom;

//     snap_offset.x = motion.x;
//     snap_offset.y = motion.y;
//   }
// };

struct Camera
{
  SDL_FRect viewport; // Where the world is drawn on screen

  // SDL_FRect screen_rect; // The screen area where the camera's view is rendered
  // SDL_FRect camera_rect; // The screen area where the camera's view is rendered

  float zoom = 1.0f;
  float prev_zoom = 1.0f; // Stores zoom from *before* the current frame/input

  bool render_grid = false;

  bool snapping = false;
  SDL_FPoint snap_offset = {0};

  // Constructor to set initial values
  Camera(const SDL_FRect &view, float initialZoom)
      : zoom(initialZoom), prev_zoom(initialZoom), viewport(view) {}

  SDL_FPoint screen_to_world(SDL_FPoint screen_point) const
  {
    float x = (screen_point.x - viewport.x) / zoom;
    float y = (screen_point.y - viewport.y) / zoom;
    return {x, y};
  }

  // Adjust camera position so that the point 'screen_cursor_pos' remains
  // fixed in world coordinates when zoom changes.
  // Call this *before* you update `zoom = new_zoom` and then update `prev_zoom = old_zoom`.
  void apply_zoom_at_cursor(float mouse_x, float mouse_y)
  {
    // Calculate the world position of the cursor *before* zoom changes
    // using the previous zoom level and camera position.
    float world_cursor_x_old = viewport.x + (mouse_x - viewport.x) / prev_zoom;
    float world_cursor_y_old = viewport.y + (mouse_y - viewport.y) / prev_zoom;

    // Now, with the new zoom, calculate where the camera *should* be
    // so that this same world point 'world_cursor_x_old' is still under
    // 'screen_cursor_pos' on the screen.
    viewport.x = world_cursor_x_old - (mouse_x - viewport.x) / zoom;
    viewport.y = world_cursor_y_old - (mouse_y - viewport.y) / zoom;
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
    // viewport.w = new_world_screen_w;
    // viewport.h = new_world_screen_h;
  }

  void handle_snapping(SDL_MouseMotionEvent &motion)
  {
    int dx = motion.x - snap_offset.x;
    int dy = motion.y - snap_offset.y;

    viewport.x += dx;
    viewport.y += dy;

    snap_offset.x = motion.x;
    snap_offset.y = motion.y;
  }

  void handle_mouse_over(SDL_Renderer *renderer, SDL_Event *event)
  {
    if (SDL_ConvertEventToRenderCoordinates(renderer, event))
    {
      SDL_Log("(fixed x = %f, y = %f)", event->motion.x, event->motion.y);
    }
  }
};

// struct Camera
// {
//   float x, y;
//   float zoom = 1;
//   float prev_zoom = 1;
//   bool dragging;
//   bool render_grid = false;

//   // convert from screen coordinates to camera coordinates
//   SDL_FPoint screen_to_camera(SDL_FPoint p)
//   {
//     return {((p.x - x) / zoom), ((p.y - y) / zoom)};
//   }

//   void zoom_at_cursor(SDL_FPoint p)
//   {
//     x = p.x - (((p.x - x) / prev_zoom) * zoom);
//     y = p.y - (((p.y - y) / prev_zoom) * zoom);
//   };
// };

struct GameContext
{
  SDL_Window *window;
  SDL_Renderer *renderer;

  int screen_width, screen_height;

  TTF_Font *font;
  SDL_MouseButtonFlags mouse_flags;

  // timer related variables
  Uint32 last_time;

  SDL_FPoint mouse_prev_point = {0};
  SDL_FPoint mouse_point = {0};

  SDL_Texture *world_layer_texture = nullptr;

  Camera *world_camera;

  GameContext(int screen_width, int screen_height, SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *world_layer_texture, TTF_Font *font)
      : screen_width(screen_width),
        screen_height(screen_height),
        window(window),
        renderer(renderer),
        world_layer_texture(world_layer_texture),
        last_time(SDL_GetTicks()),
        font(font)
  {
    float w, h;
    SDL_GetTextureSize(world_layer_texture, &w, &h);

    // Define the initial viewport for the camera.
    SDL_FRect initialViewport = {0.0f, 0.0f, w, h};
    world_camera = new Camera(initialViewport, 1.0f);
  }

  // Change 3: Destructor to free dynamically allocated Camera object
  ~GameContext()
  {
    if (world_camera)
    {
      delete world_camera;
    }
    // Also destroy other SDL resources
    // if (font)
    //   TTF_CloseFont(font);
    // if (world_layer_texture)
    //   SDL_DestroyTexture(world_layer_texture);
    // if (renderer)
    //   SDL_DestroyRenderer(renderer);
    // if (window)
    //   SDL_DestroyWindow(window);
    // TTF_Quit();
    // SDL_Quit();
  }

  // Disable copy constructor and assignment operator for `GameContext`
  // when managing raw pointers, unless you implement deep copying.
  // This prevents subtle bugs where `world_camera` might be double-deleted.
  GameContext(const GameContext &) = delete;
  GameContext &operator=(const GameContext &) = delete;

  // @fixme: do not do this like this
  SDL_Texture *grass;

  bool load_assets()
  {
    grass = IMG_LoadTexture(renderer, "assets/demo.png");
    return !!grass;
  }
};
