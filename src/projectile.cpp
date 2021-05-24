
#include "projectile.hpp"
#include "render.hpp"


// TODO: For debug purposes for now
#include <iostream>


 ECS::Entity Projectile::createProjectile(const std::string& projectile_type,
                                                vec2 origin, vec2 goal_pos_p, float speed, int pierce, float pierce_frequency,
                                                // This is the attack function in world, attack_target
                                                ECS::Entity thrower, float rotation = 0) {
    auto entity = ECS::Entity();

    // Below code is based off of salmon.cpp, for rendering
    ShadedMesh& resource = cache_resource(projectile_type);
    std::string texture_path = projectile_type + "_sprite.png";

    if (resource.effect.program.resource == 0) {
        try {
            RenderSystem::createSprite(resource, textures_path(texture_path), "textured");
        } catch (std::runtime_error& error) {
            // TODO: Remove when we have enemy sprites
            std::cout << "Projectile sprite doesn't exist for " << projectile_type << std::endl;
            std::cout << error.what() << std::endl;
        }
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = goal_pos_p;
    // TODO: Resize this maybe
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = {0, 0};


    auto& proj = ECS::registry<Projectile>.emplace(entity);
    proj.origin_pos = origin;
    proj.goal_pos = goal_pos_p;
    proj.max_dist = length(goal_pos_p - origin);
    proj.dist_traveled = 0.f;
    proj.speed = speed;

    auto& moving = ECS::registry<Moving>.emplace(entity);
    moving.oldPos = origin;
    moving.step = (goal_pos_p - origin);
    moving.step = moving.step/length(moving.step) * speed;

    proj.thrower_entity = thrower;
    proj.max_pierce = pierce;
    proj.pierce_interval = pierce_frequency;
    proj.pierce_counter = pierce_frequency;
    if (rotation != -1) {
        proj.rotation = rotation;
    } else {
        proj.rotation = RenderSystem::cartesianTheta(goal_pos_p - origin);
    }

    return entity;
}