// internal
#include "turn.hpp"
#include "spellpower.hpp"
#include "render.hpp"


// TODO: For debug purposes for now
#include <iostream>
#include <utility>

ECS::Entity SpellPower::createSpellPower(std::string texture_path, std::string name, Spell::SpellEffect spell_type, vec2 grid_position) {
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


    SpellPower& spellPower = ECS::registry<SpellPower>.emplace(entity);
    spellPower.name = std::move(name);
    spellPower.spell_effect = spell_type;
    spellPower.texture_path = texture_path;
//    std::cout << "set spellPower name to " + name << "\n";

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = { 0, 0 };
    tile_position.grid_pos = grid_position;
    tile_position.isSolid = false;

    return entity;
}