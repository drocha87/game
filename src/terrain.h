#pragma once

enum class TerrainKind
  {
    Solid,
    Liquid
  };

class Terrain
{
public:
  TerrainKind kind;
  virtual const char *name() const = 0;
  virtual bool is_passable() const = 0;
  virtual bool is_shoreline() const { return false; }
  virtual ~Terrain() {}
};

class Water : public Terrain
{
public:
  enum class Depth
    {
      Shallow,
      Deep
    };

  Depth depth;

  Water(Depth d) : depth(d) { kind = TerrainKind::Liquid; }

  const char *name() const override
  {
    return depth == Depth::Shallow ? "Shallow Water" : "Deep Water";
  }

  bool is_passable() const override
  {
    return depth == Depth::Shallow; // Example rule: shallow water is passable with penalties
  }

  bool is_shoreline() const override
  {
    return depth == Depth::Shallow;
  }
};

class Land : public Terrain
{
public:
  enum class Type
    {
      Grass,
      Sand,
      Mud,
      Stone,
      Snow,
      Dirt,
      Road,
      Bridge
    };

  Type type;

  Land(Type t) : type(t) { kind = TerrainKind::Solid; }

  const char *name() const override
  {
    switch (type)
      {
      case Type::Grass:
        return "Grass";
      case Type::Sand:
        return "Sand";
      case Type::Mud:
        return "Mud";
      case Type::Stone:
        return "Stone";
      case Type::Snow:
        return "Snow";
      case Type::Dirt:
        return "Dirt";
      case Type::Road:
        return "Road";
      case Type::Bridge:
        return "Bridge";
      }
    return "Unknown";
  }

  bool is_passable() const override
  {
    // You can implement more advanced logic per tile type
    return type != Type::Stone && type != Type::Mud;
  }
};
