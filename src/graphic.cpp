//
// Created by Roark on 2021-03-03.
//
#include "graphic.hpp"
#include <iostream>
#include <utility>

ECS::Entity Graphic::createGraphic(vec2 centre, vec2 dims, std::string path)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    auto& mesh = ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    auto& graphic = ECS::registry<Graphic>.emplace(entity);
    graphic.shadedMeshRef = &mesh;

    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = centre;
    motion.scale = dims;
    graphic.motion = &motion;
    return entity;
}

ECS::Entity TileGraphic::createTileGraphic(vec2 gridPos, std::string path) {
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    auto& mesh = ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    auto& graphic = ECS::registry<Graphic>.emplace(entity);
    graphic.shadedMeshRef = &mesh;

    auto& tilePos = ECS::registry<TilePosition>.emplace(entity);
    tilePos.grid_pos = gridPos;
    // Setting initial values, scale is negative to make it face the opposite way
    tilePos.scale = {100, 100};
    return entity;
}
