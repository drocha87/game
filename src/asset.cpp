#include "asset.h"

bool Asset::load_texture(SDL_Renderer* renderer, const std::string& id, const std::string& path) {
  SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
  
  if (!texture) {
    SDL_Log("Failed to create texture for '%s': %s", path.c_str(), SDL_GetError());
    return false;
  }
  
  textures_[id] = texture;
  return true;
}

SDL_Texture* Asset::get_texture(const std::string& id) const {
  auto it = textures_.find(id);
  return (it != textures_.end()) ? it->second : nullptr;
}

void Asset::unload_all() {
  for (auto& [_, tex] : textures_) {
    SDL_DestroyTexture(tex);
  }
  textures_.clear();
}
