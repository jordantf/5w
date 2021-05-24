#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include "text.hpp"
#include "graphic.hpp"
#include "button.hpp"

struct Tile
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createTile(vec2 grid_position, int tile_type, int area_type);

    int isBlocking; // 0 if passable, 1 if blocked
    enum tile_type { FLOOR, WALL, HOLE, GOAL };
};

struct TilesheetTile
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createTile(vec2 grid_position, int tilesheet, int tile_type, int area_type, std::string shader_name);

    int tilesheet;
};

struct Torch
{
    bool lit = false;
    vec3 colour;
    float brightness;
    std::string key;
    void setLight(bool lit);
    ECS::Entity self; //unfortunately needed for setting LightSource component
    ECS::Entity flame_particles;
    ECS::Entity smoke_particles;
    static ECS::Entity createTorch(vec2 grid_position, bool lit, float brightness = 0.5, vec3 colour = {1.0, 0.8, 0.3});
    static void removeTorch(ECS::Entity e);
};

class Interactible
{
public:
    Interactible(std::string hoverText);
    ~Interactible();
//    static void removeInteractible(ECS::Entity self);
    std::function<void(ECS::Entity*)> onInteract;
    ECS::Entity button;
    ECS::Entity text;
    ECS::Entity* adjacentPlayer = nullptr;
    std::string desc;

    void toggleAdjacent(bool isAdjacent, ECS::Entity* adjacentPlayer);

    struct InteractibleDisplay{};

    void interact();

    void destroy();
};

struct Door
{
    bool locked;
    bool opened;
    bool horizontal;
    int id;
    bool toggle(bool try_open);
    void unlock();
    int door_type;
    vec2 position;
    static ECS::Entity createDoor(vec2 grid_position, int id, int door_type, bool horizontal, bool locked);

};