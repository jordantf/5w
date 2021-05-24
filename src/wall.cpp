//
// Created by Alan Zhang on 2021-02-10.
//

// Header
#include "wall.hpp"
#include "render.hpp"

ECS::Entity Wall::createWall(vec2 grid_position)
{
    auto entity = ECS::Entity();

    // Create rendering primitives
    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path("wall.png"), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the motion
    auto& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    tile_position.scale = { 100, 100 };

    // Create and (empty) Turtle component to be able to refer to all turtles
    ECS::registry<Wall>.emplace(entity, grid_position);

    return entity;
}

Wall::Wall(vec2 grid_pos) {
    grid_position = grid_pos;
}
