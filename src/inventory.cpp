#pragma once
#include "inventory.hpp"

Weapon::Weapon(std::basic_string<char> theName, int damage) {
    this->name = theName;
    this->damage = damage;
}

Inventory::~Inventory() {
    this->weapons.clear();
    this->artifacts.clear();
    this->keys.clear();
}

Key::Key(int id) {
    this->id = id;
}

ECS::Entity Key::createKey(vec2 gridPosition, int id) {
    ECS::Entity entity = ECS::Entity();
    // Create a tile component to be able to refer to all tiles
    auto& key = ECS::registry<Key>.emplace(entity, id);

    std::string key_key = "key";

    ShadedMesh& sprite = cache_resource(key_key + std::to_string(id));

    if (sprite.effect.program.resource == 0) {
        RenderSystem::createSprite(sprite, textures_path(key_key + ".png"), std::move("textured"));
    }

    ECS::registry<ShadedMeshRef>.emplace(entity, sprite);

    // Initialize the motion
    auto& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = gridPosition;
    tile_position.scale = { 100, 100 };
    tile_position.isSolid = false;

    return entity;
}
