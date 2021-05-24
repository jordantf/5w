// Header
#include "worldEditSystem.hpp"
#include "common.hpp"
#include "../ext/json/single_include/nlohmann/json.hpp"
#include "button.hpp"
#include "ui.hpp"
using jsonf = nlohmann::json;
using json = nlohmann::json;


#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>

#define AREA_LIST "list_areas.json"

WorldEditSystem::WorldEditSystem(WorldSystem *w, ivec2 w_size) {
    world = w;
    window_size = w_size;
    tiles = std::vector<ECS::Entity*>(36, nullptr);
}

void WorldEditSystem::showLevels() {
    std::list<std::string> json_areas;
    std::ifstream json_list(save_path(AREA_LIST));
    std::string content((std::istreambuf_iterator<char>(json_list)),
                        (std::istreambuf_iterator<char>()));
    if (json_list.is_open()) {
        auto save_state = json::parse(content);
        // Parse JSON
        for (auto& element : save_state.items()) {
            // Terrain
            if (element.key() == "list_areas") {
                int outerSize = element.value().size();
                for (int i = 0; i < element.value().size(); i++) {
                    json_areas.push_back(element.value()[i]);
                }
            }
        }
        json_list.close();   //close the file object.
    }
    int y = 40;
    for (const std::string& str : json_areas) {
        Button::createButton({200, y}, 40, 40,
                             [this,str](){
            this->loadLevel(str);
            this->editLevel();
            this->start();
            }, "buttons/editor_edit.png");
        Button::createButton({242, y}, 40, 40, [this,str](){this->playLevel(str);}, "buttons/editor_play.png");
        auto e = UISystem::showText(str, {267, y + 5}, 0.5);
        auto& text = ECS::registry<Text>.get(e);
        text.colour = {0, 0, 0};
        y += 50;
    }
}

void WorldEditSystem::loadLevel(std::string path) {
    current_file = path;
    world->load_area(path);
}

void WorldEditSystem::start() {
//    UISystem::clearText();
    UISystem::clearGraphics();
    UISystem::clearText();
    ECS::Entity button = Button::createButton({140,50}, 90, 60,
                                        [this](){
                                            vec2 pos = this->editorPosition->grid_pos;

                                            while (ECS::registry<Character>.entities.size()>0)
                                                ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());
                                            this->world->load_chars(current_file);
                                            this->world->save_area(current_file);

                                            while (ECS::registry<Character>.entities.size()>0)
                                                ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());

                                            ECS::Entity e = Character::createCharacter("editor", pos, 0);
                                            auto& c = ECS::registry<Character>.get(e);
                                            ECS::registry<Turn>.emplace(e, c);
                                            this->editorPosition = &ECS::registry<TilePosition>.get(e);
                                        },
                                        "ui/editor/save_button.png");
    ECS::registry<Tooltip>.insert(button, Tooltip({window_size.x - 40,50}, "Save level"));

    world->in_game = true;
    while (ECS::registry<Character>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());
    while (ECS::registry<TilePosition>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<TilePosition>.entities.back());
    world->loadTiles();

    ECS::Entity e = Character::createCharacter("editor", {0,0}, 0);
    auto& c = ECS::registry<Character>.get(e);
    ECS::registry<Turn>.emplace(e, c);
    editorPosition = &ECS::registry<TilePosition>.get(e);
    showTilePalette();
    updatePalette(world->world_grid_tileset[0][0], false);
}

void WorldEditSystem::step(float elapsed_ms, vec2 window_size_in_game_units) {

}

void WorldEditSystem::on_key(int key, int _2, int action, int mod) {
    // WASD for movement
    if (action == GLFW_PRESS)
    {
        vec2 movement = {0,0};
        switch(key) {
            case GLFW_KEY_W:
                movement.y = -1;
                break;
            case GLFW_KEY_A:
                movement.x = -1;
                break;
            case GLFW_KEY_S:
                movement.y = 1;
                break;
            case GLFW_KEY_D:
                movement.x = 1;
                break;
            default:
                break;
        }
        editorPosition->grid_pos += movement;
        editorPosition->grid_pos.x = clamp(editorPosition->grid_pos.x, 0.f, float(world->world_grid.size() - 1));
        editorPosition->grid_pos.y = clamp(editorPosition->grid_pos.y, 0.f, float(world->world_grid[0].size() - 1));
        updatePalette(world->world_grid_tileset[editorPosition->grid_pos.x][editorPosition->grid_pos.y], false);
    }

    if (action == GLFW_PRESS && ((key == GLFW_KEY_UP) || (key == GLFW_KEY_LEFT) || (key == GLFW_KEY_RIGHT) || (key == GLFW_KEY_DOWN)))
    {
        int newTile;
        switch(key) {
            case GLFW_KEY_UP:
                newTile = ((selectedTile + 36) - 6) % 36;
                break;
            case GLFW_KEY_LEFT:
                newTile = ((selectedTile + 36) - 1) % 36;
                break;
            case GLFW_KEY_DOWN:
                newTile = (selectedTile + 6) % 36;
                break;
            case GLFW_KEY_RIGHT:
                newTile = (selectedTile + 1) % 36;
                break;
        }
        updatePalette(newTile, true);
    }
}

void WorldEditSystem::on_mouse_move(vec2 mouse_pos) {
    world->on_mouse_move(mouse_pos);

    // no need to do anything as selected tile is handled in the main.cpp loop.
}

void WorldEditSystem::on_mouse_button(int button, int action, int mods) {

}

void WorldEditSystem::showTilePalette() {
    int area = world->getArea();
    vec2 paletteOrigin = {240, window_size.y-240};
    UISystem::showGraphic("tilesets/" + std::to_string(area) + ".png", paletteOrigin, {480, 480});
    paletteOrigin = paletteOrigin - vec2(240, 240); // Set origin to top left corner
    for (int i = 0; i < 36; i++) {
        vec2 tilesheet_coord = {(i%6) * 80, (floor(i/6)) * 80};
        tilesheet_coord = tilesheet_coord + paletteOrigin + vec2(40,40); // {40,40} for centering
        ECS::Entity* e = new ECS::Entity(Button::createButton(tilesheet_coord, 80, 80,
                                                              [this,i](){
                this->updatePalette(i, true);
            },
                                             "ui/editor/tile_button.png", "ui/editor/tile_selected_button.png", "ui/editor/tile_button.png",
                                             true, true, i));
        tiles[i] = e;
    }
}

void WorldEditSystem::updatePalette(int newTile, bool update) {
    auto& oldSelect = ECS::registry<Button>.get(*tiles[selectedTile]);
    oldSelect.toggleOff();
    auto& newSelect = ECS::registry<Button>.get(*tiles[newTile]);
    newSelect.toggleOn();
    selectedTile = newTile;
    if (update) {
        vec2 pos = editorPosition->grid_pos;

        world->world_grid[pos.x][pos.y] = paletteType(selectedTile);
        world->world_grid_tileset[pos.x][pos.y] = selectedTile;
        while (ECS::registry<TilePosition>.entities.size()>0)
            ECS::ContainerInterface::remove_all_components_of(ECS::registry<TilePosition>.entities.back());
        world->loadTiles();
        ECS::Entity e = Character::createCharacter("editor", pos, 0);
        auto& c = ECS::registry<Character>.get(e);
        ECS::registry<Turn>.emplace(e, c);
        editorPosition = &ECS::registry<TilePosition>.get(e);
//        ECS::ContainerInterface::list_all_components();
    }
}

int WorldEditSystem::paletteType(int tilesheetNum) {
    if (tilesheetNum <= 5) {
        // Tiles 0 to 5, first row
        return Tile::FLOOR;
    } else if (tilesheetNum <= 17) {
        // Tiles 6 to 11, and 12 to 17, second row
        return Tile::WALL;
    } else if (tilesheetNum <= 23) {
        // Fourth row is voids
        return Tile::HOLE;
    } else {
        if (tilesheetNum == 35) return Tile::GOAL;
        else return Tile::FLOOR;
    }
}

WorldEditSystem::~WorldEditSystem() {
    for(ECS::Entity* e : tiles) {
        if (e != nullptr) {
            ECS::ContainerInterface::remove_all_components_of(*e);
            delete e;
            e = nullptr;
        }
    }
    UISystem::clearGraphics();
    UISystem::clearText();
    while (ECS::registry<Button>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities.back());

    while (ECS::registry<Character>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());

    while (ECS::registry<TilePosition>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<TilePosition>.entities.back());

    world->resetCamera = true;
}

