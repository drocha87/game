#pragma once

#include <unordered_map>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

class Asset {
 public:
  bool load_texture(SDL_Renderer* renderer, const std::string& id, const std::string& path);
  SDL_Texture* get_texture(const std::string& id) const;
  void unload_all();

 private:
  std::unordered_map<std::string, SDL_Texture*> textures_;
};
