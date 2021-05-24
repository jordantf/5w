//
// Created by Roark on 2021-02-05.
//

#include "background.hpp"

#include <iostream>
#include "render.hpp"

ECS::Entity Background::createBackground(std::string path, vec2 scale)
{
    auto entity = ECS::Entity();

    // Create rendering primitives
    std::string key = path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the motion
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.scale = scale * static_cast<vec2>(resource.texture.size);

    // Create and (empty) Turtle component to be able to refer to all turtles
    ECS::registry<Background>.emplace(entity);
    auto& bg = ECS::registry<Background>.get(entity);
    bg.dims.x = 256; //hardcode
    bg.dims.y = 256;
    bg.base_position = {600, 400};
    bg.parallax = 0.5f;

    return entity;
}

void Background::changeBackground(std::string new_path, vec2 scale) {
    std::cout << "changing bacnground\n";
    auto& backgrounds = ECS::registry<Background>;

    // if a background exists, remove them
    if(backgrounds.size() > 0) {
        for(unsigned int i = 0; i < backgrounds.size(); i++) {
            auto& entity = backgrounds.entities[i];
            auto& shaderRef = ECS::registry<ShadedMeshRef>.components[i];
            free(shaderRef.reference_to_cache);
            backgrounds.remove(entity);
            ECS::registry<ShadedMeshRef>.remove(entity);
            ECS::registry<Motion>.remove(entity);
        }
    }

    // create a new one
    createBackground(std::move(new_path), scale);
}
