#include "money.hpp"
#include "graphic.hpp"
#include <iostream>

ECS::Entity Money::createMoney(const std::string& money_name, vec2 gridPos, int value)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    std::string key, path;
    if (value < 10) {
        key = "small-gold.png";
        path = "small-gold.png";
    }
    else if (value >= 10 && value <= 30){
        key = "medium-gold.png";
        path = "medium-gold.png";
    }
    else {
        key = "large-gold.png";
        path = "large-gold.png";
    }
    // Create the rendering components
    
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    ECS::registry<Money>.emplace(entity);
    auto& money = ECS::registry<Money>.get(entity);
    money.name = money_name;
    money.value = value;

    auto& tilepos = ECS::registry<TilePosition>.emplace(entity);
    tilepos.scale = { 100, 100 };
    tilepos.grid_pos = gridPos;
    tilepos.screen_pos = gridPos * tilepos.scale;
    tilepos.isSolid = false;

    return entity;
}
