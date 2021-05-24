// Header
#include "tile.hpp"
#include "render.hpp"
#include "lightSource.hpp"
#include "ui.hpp"

ECS::Entity Tile::createTile(vec2 grid_position, int tile_type, int area_type)
{
    auto entity = ECS::Entity();

    // Create rendering primitives
    if (tile_type == FLOOR) {
        std::string key = "floor" + std::to_string(area_type);
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0)
            RenderSystem::createSprite(resource, textures_path("floor_" + std::to_string(area_type) + ".png"), "textured");
        ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    } else if (tile_type == WALL) {
        std::string key = "wall" + std::to_string(area_type);
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0)
            RenderSystem::createSprite(resource, textures_path("wall_" + std::to_string(area_type) + ".png"), "textured");
        ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    }
    else { // goal tile
        std::string key = "goal";
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0)
            RenderSystem::createSprite(resource, textures_path("exit.png"), "textured");
        ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    }

    // Initialize the motion
    auto& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    tile_position.scale = { 100, 100 };
    tile_position.isSolid = false;

    // Create a tile component to be able to refer to all tiles
    auto& tile = ECS::registry<Tile>.emplace(entity);


    // Set the tile properties
    if (tile_type == WALL) {
        tile.isBlocking = 1;
    } else {
        tile.isBlocking = 0;
    }

    return entity;
}

ECS::Entity TilesheetTile::createTile(vec2 grid_position, int tilesheet, int tile_type, int area_type, std::string shader_name) {
    auto entity = ECS::Entity();

    // Create a tile component to be able to refer to all tiles
    auto& sheet = ECS::registry<TilesheetTile>.emplace(entity);

    sheet.tilesheet = tilesheet;

    std::string key = "tilesets/" + std::to_string(area_type);

    ShadedMesh& sprite = cache_resource(key + "_tile");
    if (sprite.effect.program.resource == 0) {
        RenderSystem::createSprite(sprite, textures_path(key), std::move(shader_name), true);
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, sprite);

    // Initialize the motion
    auto& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    tile_position.scale = { 100, 100 };
    tile_position.isSolid = false;

    auto& tile = ECS::registry<Tile>.emplace(entity);

    // Set the tile properties
    if (tile_type == tile.WALL) {
        tile.isBlocking = 1;
    } else {
        tile.isBlocking = 0;
    }

    return entity;
}

ECS::Entity Torch::createTorch(vec2 grid_position, bool lit, float brightness, vec3 colour) {
    ECS::Entity e =  ECS::Entity();
    auto& torch = ECS::registry<Torch>.emplace(e);
    torch.colour = colour;
    torch.lit = lit;
    torch.brightness = brightness;

    auto& tile_position = ECS::registry<TilePosition>.emplace(e);
    tile_position.grid_pos = grid_position;
    tile_position.isSolid = false;
    tile_position.scale = {100,100};

    std::string key = std::to_string(grid_position.x) + "_" + std::to_string(grid_position.y) + "_torch";

    ShadedMesh& sprite = cache_resource(key);
    if (sprite.effect.program.resource == 0) {
        RenderSystem::createSprite(sprite,
                                   textures_path(lit?"torch_lit":"torch_unlit"),
                                   "tint_textured", false);
    }
    ECS::registry<ShadedMeshRef>.emplace(e, sprite);
    if (lit) {
        auto& ls = ECS::registry<LightSource>.emplace(e);
        ls.strength = brightness;
        torch.flame_particles = ParticleSystem::createParticleEmitter("particles/flame.png", 60.f, 120.f,
                                                                      100.f * grid_position - vec2(10, 10), 100.f * grid_position + vec2(10, 10),
                                                                      {1200.f, 2000.f}, -M_PI/2, 0.5, {40.f, 80.f},
                                                                      {0.f, -60.f}, 0.95, 10, 20, -5.0,
                                                                      {0.8, 1.0}, 1.0);

        torch.smoke_particles = ParticleSystem::createParticleEmitter("particles/smoke.png", 0.f, 50.f,
                                                                      100.f * grid_position - vec2(10, 20), 100.f * grid_position + vec2(10, 0),
                                                               {1500.f, 3000.f}, -M_PI/2, 1.0, {300.f, 500.f},
                                                               {0.f, -60.f}, 0.95, 20, 30, 30.0,
                                                               {0.1, 0.15}, 0.6);

    }
    torch.key = key;
    torch.self = e;
    return e;
}

void Torch::setLight(bool lit) {
    if (lit == this->lit) return;
    this->lit = lit;
    if (lit) {
        auto& ls = ECS::registry<LightSource>.emplace(self);
        ls.strength = brightness;
        auto& grid_position = ECS::registry<TilePosition>.get(self).grid_pos;
//        ParticleSystem::createParticleBurst("particles/flame.png", 5, 90.f, 120.f,
//                                            100.f * grid_position - vec2(10, 10),
//                                            100.f * grid_position + vec2(10, 10),
//                                            {500.f, 1000.f}, -M_PI/2, 1.0, {-10.f, -70.f},
//                                            0.9, 15, 30, -3.0, {0.8, 1.0}, 1.0);
        flame_particles = ParticleSystem::createParticleEmitter("particles/flame.png", 60.f, 120.f,
                                                                      100.f * grid_position - vec2(10, 10), 100.f * grid_position + vec2(10, 10),
                                                                      {1500.f, 3000.f}, -M_PI/2, 0.5, {400.f, 700.f},
                                                                      {0.f, -60.f}, 0.95, 10, 20, -5.0,
                                                                      {0.8, 1.0}, 2.0);

        smoke_particles = ParticleSystem::createParticleEmitter("particles/smoke.png", 0.f, 50.f,
                                                                      100.f * grid_position - vec2(10, 20), 100.f * grid_position + vec2(10, 0),
                                                                      {1500.f, 3000.f}, -M_PI/2, 1.0, {300.f, 500.f},
                                                                      {0.f, -60.f}, 0.95, 20, 30, 30.0,
                                                                      {0.05, 0.1}, 0.6);
    }
    else {
        ECS::registry<LightSource>.remove(self);
        ECS::registry<ParticleEmitter>.remove(flame_particles);
        ECS::registry<ParticleEmitter>.remove(smoke_particles);
    }
    RenderSystem::createSprite(cache_resource(this->key),
                               textures_path(lit?"torch_lit":"torch_unlit"),
                               "tint_textured", false);
}

void Torch::removeTorch(ECS::Entity e) {
    auto& t = ECS::registry<Torch>.get(e);
    if (t.lit) {
        ECS::registry<LightSource>.remove(t.self);
        ECS::registry<ParticleEmitter>.remove(t.flame_particles);
        ECS::registry<ParticleEmitter>.remove(t.smoke_particles);
    }
    ECS::ContainerInterface::remove_all_components_of(e);
}

Interactible::Interactible(std::string hoverText) {
    this->button = ECS::Entity();
    this->text = ECS::Entity();
    onInteract = [this](ECS::Entity* e){};
    desc = hoverText;
}

void Interactible::toggleAdjacent(bool isAdjacent, ECS::Entity* adjacentPlayer) {
    if (isAdjacent) {
        // If hidden, unhide
        if (ECS::registry<Hidden>.has(this->button)) {
            ECS::registry<Hidden>.remove(this->button);
            UISystem::destroyText(this->text);
            UISystem::showText(this->text, desc, {500, 400}, 0.5);
            if (!ECS::registry<Button>.has(this->button)) {
                Button::createButton(this->button, {600,370}, 72, 22, [this](){
                    this->interact();
                }, "buttons/interact.png");
            }
            auto& button = ECS::registry<Button>.get(this->button);
            button.hidden = false;
        }
    } else {
        if (!ECS::registry<Hidden>.has(this->button)) {
            if (ECS::registry<Button>.has(this->button)) {
                auto &button = ECS::registry<Button>.get(this->button);
                button.hidden = true;
            }
            UISystem::destroyText(this->text);
            ECS::registry<Hidden>.emplace(this->button);
        }
    }
    this->adjacentPlayer = adjacentPlayer;
}

void Interactible::interact() { std::cout << "interacting\n";
    onInteract(adjacentPlayer);
}

void Interactible::destroy() {
    UISystem::destroyText(this->text);
    ECS::registry<Button>.remove(this->button);
    ECS::ContainerInterface::remove_all_components_of(this->button);
    ECS::ContainerInterface::remove_all_components_of(this->text);
}

Interactible::~Interactible() {
//    ECS::ContainerInterface::list_all_components_of(this->button);
    UISystem::destroyText(this->text);
    if (ECS::registry<Button>.has(this->button))
        ECS::registry<Button>.remove(this->button);
    ECS::ContainerInterface::remove_all_components_of(this->button);
    ECS::ContainerInterface::remove_all_components_of(this->text);
//    std::cout << "after removing" << std::endl;
//    ECS::ContainerInterface::list_all_components_of(this->button);
}

ECS::Entity Door::createDoor(vec2 grid_position, int id, int door_type, bool horizontal, bool locked) {
    auto entity = ECS::Entity();

    // Create a tile component to be able to refer to all tiles
    auto& door = ECS::registry<Door>.emplace(entity);

    door.id = id;
    door.door_type = door_type;
    door.horizontal = horizontal;
    door.opened = false;
    door.locked = locked;

    // Close the door by creating a wall (:joy:)
    WorldSystem::world_grid[grid_position.x][grid_position.y] = Tile::WALL;

    std::string key = "tilesets/doors";

    ShadedMesh& sprite = cache_resource(key);
    if (sprite.effect.program.resource == 0) {
        RenderSystem::createSprite(sprite, textures_path(key), std::move("tileset"), true);
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, sprite);

    // Initialize the motion
    auto& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    tile_position.scale = { 100, 100 };
    tile_position.isSolid = false;

    door.position = grid_position;

    return entity;
}

bool Door::toggle(bool open) {
    if (locked) {
        ECS::registry<TempText>.emplace(UISystem::showText("Door is locked!", {300, 400}, 0.5f), 3000.f);
        return false;
    }
    WorldSystem::world_grid[position.x][position.y] = open? Tile::FLOOR : Tile::WALL;
    if (open != opened) {
        WorldSystem::GenerateBoundingEdges();
    }
    this->opened = open;
    return true;
}

void Door::unlock() {
    locked = false;
}
