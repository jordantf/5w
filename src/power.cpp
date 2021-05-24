// internal
#include "turn.hpp"
#include "power.hpp"
#include "render.hpp"


// TODO: For debug purposes for now
#include <iostream>
#include <utility>

ECS::Entity Power::createPower(std::string texture_path, std::string name, int point, vec2 grid_position) {
    auto entity = ECS::Entity();

    // Below code is based off of salmon.cpp, for rendering
    ShadedMesh& resource = cache_resource(name);
//    std::string texture_path ="power_sprite.png";

    if (resource.effect.program.resource == 0) {
        try {
            RenderSystem::createSprite(resource, textures_path(texture_path), "textured");
        }
        catch (std::runtime_error& error) {
            // TODO: Remove when we have enemy sprites
            std::cout << "Entity sprite doesn't exist for power"  << std::endl;
            std::cout << error.what() << std::endl;
        }
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);


    Power& power = ECS::registry<Power>.emplace(entity);
    power.point = point;
    power.name = std::move(name);
//    std::cout << "set power name to " + name << "\n";

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = { 0, 0 };
    tile_position.grid_pos = grid_position;
    tile_position.isSolid = false;

    return entity;
}


ECS::Entity Power::createAnimatedPower(int point, vec2 grid_position, Animation starting_anim) {

    std::cout << "creating animated power with anim: " << "\n";

    auto entity = ECS::Entity();
    auto animation = std::move(starting_anim);


    //    std::cout << "cols: " << animation.cols_rows.x << "rows: " << animation.cols_rows.y << "\n";

        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<Animation>.emplace(entity, animation);

    auto& power = ECS::registry<Power>.emplace(entity);
    power.point = point;

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    // TODO: Resize this maybe
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = { 0, 0 };
    tile_position.grid_pos = grid_position;
    tile_position.isSolid = false;



    return entity;
}