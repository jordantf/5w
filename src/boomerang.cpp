
#include "boomerang.hpp"
#include "hitbox.hpp"
#include "projectile.hpp"
#include "render.hpp"

ECS::Entity Boomerang::createBoomerang(const std::string& boomerangType, float spin, float decayTime,
                                       vec2 origin, vec2 goal_pos_p, float speed,
                                       ECS::Entity thrower) {
    ECS::Entity entity = ECS::Entity();

    ShadedMesh& resource = cache_resource("boomerang");
    std::string texture_path = "boomerang_" + boomerangType + "_sprite.png";

    if (resource.effect.program.resource == 0) {
        try {
            RenderSystem::createSprite(resource, textures_path(texture_path), "textured");
        } catch (std::runtime_error& error) {
        }
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    ECS::registry<Boomerang>.emplace(entity);
    auto& boomerang = ECS::registry<Boomerang>.get(entity);
    boomerang.spin = spin;
    boomerang.origin = origin;
    boomerang.decayTime = decayTime;
    boomerang.boomerangType = boomerangType;
    boomerang.speed = speed;
    boomerang.thrower = thrower;

    ECS::registry<Hitbox>.emplace(entity);
    auto& hitbox = ECS::registry<Hitbox>.get(entity);

//    std::string key = "boomerang_triangle";
//    ShadedMesh& resource = cache_resource(key);

    constexpr float z = -0.1f;
    vec3 color = { 0.5,0.1,0.5 };

    // Corner points
    ColoredVertex v;
    v.position = {-0.25,-0.4375,z}; //0,0
    v.color = color;
    hitbox.vertices.push_back(v);
    v.position = { -0.4375,0.4375,z }; //0,1
    v.color = color;
    hitbox.vertices.push_back(v);
    v.position = { 0.4375,0.25,z }; //1,1
    v.color = color;
    hitbox.vertices.push_back(v);

    hitbox.vertex_indices.push_back(0);
    hitbox.vertex_indices.push_back(1);
    hitbox.vertex_indices.push_back(2);

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = {-1, -1};
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = {0, 0};

    auto& moving = ECS::registry<Moving>.emplace(entity);
    moving.oldPos = origin;
    moving.step = (goal_pos_p - origin);
    moving.step = moving.step/length(moving.step) * speed;

    return entity;
}
