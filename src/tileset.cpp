#include "tileset.h"

SDL_Point get_terrain_mask(int mask)
{
  switch (mask)
    {
    case 0:   return {0, 0};
    case 4:   return {1, 0};
    case 84:  return {2, 0};
    case 92:  return {3, 0};
    case 124: return {4, 0};
    case 116: return {5, 0};
    case 80:  return {6, 0};
    case 16:  return {0, 1};
    case 28:  return {1, 1};
    case 117: return {2, 1};
    case 95:  return {3, 1};
    case 255: return {4, 2};
      // {
      //   static const std::array<SDL_Point, 3> options = {{{4, 2},
      //                                                     {1, 4},
      //                                                     {4, 1}}};

      //   static std::random_device rd;
      //   static std::mt19937 rng(rd());
      //   static std::uniform_int_distribution<std::size_t> dist(0, options.size() - 1);

      //   return options[dist(rng)];
      // }
    case 253: return {5, 1};
    case 113: return {6, 1};
    case 21:  return {0, 2};
    case 87:  return {1, 2};
    case 221: return {2, 2};
    case 127: return {3, 2};
    case 247: return {5, 2};
    case 209: return {6, 2};
    case 29:  return {0, 3};
    case 125: return {1, 3};
    case 119: return {2, 3};
    case 199: return {3, 3};
    case 215: return {4, 3};
    case 213: return {5, 3};
    case 81:  return {6, 3};
    case 31:  return {0, 4};
    case 241: return {2, 4};
    case 20:  return {3, 4};
    case 65:  return {4, 4};
    case 17:  return {5, 4};
    case 1:   return {6, 4};
    case 23:  return {0, 5};
    case 223: return {1, 5};
    case 245: return {2, 5};
    case 85:  return {3, 5};
    case 68:  return {4, 5};
    case 93:  return {5, 5};
    case 112: return {6, 5};
    case 5:   return {0, 6};
    case 71:  return {1, 6};
    case 197: return {2, 6};
    case 69:  return {3, 6};
    case 64:  return {4, 6};
    case 7:   return {5, 6};
    case 193: return {6, 6};
    default:  return {0, 0};
    }
}
