//
// Created by Roark on 2021-03-05.
//

#include "artifact.hpp"
#include "graphic.hpp"
#include <iostream>

ECS::Entity Artifact::createArtifact(vec2 gridPos, int area, std::string path)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    auto& graphic = ECS::registry<Artifact>.emplace(entity);
    graphic.area = area;
    graphic.name = path;

    auto& tilepos = ECS::registry<TilePosition>.emplace(entity);
    tilepos.scale = {100, 100};
    tilepos.grid_pos = gridPos;
    tilepos.screen_pos = gridPos*tilepos.scale;
    tilepos.isSolid = false;

    return entity;
}


