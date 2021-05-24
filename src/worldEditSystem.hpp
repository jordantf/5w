#pragma once

#include <vector>

#include "world.hpp"
#include "button.hpp"

class WorldEditSystem {
public:
    WorldEditSystem(WorldSystem* w, ivec2 w_size);

    ~WorldEditSystem();

    WorldSystem* world; // the world to be edited

    void showLevels();

    void loadLevel(std::string path);

    void step(float elapsed_ms, vec2 window_size_in_game_units);

    void on_key(int key, int _2, int action, int mod);

    void on_mouse_move(vec2 mouse_pos);

    void on_mouse_button(int button, int action, int mods);

    std::function<void()> editLevel; // called when edit clicked
    std::function<void(std::string str)> playLevel; // called when play clicked
    void start();
    ivec2 window_size;

    static int paletteType(int tilesheetNum);
private:
    TilePosition* editorPosition;
    std::vector<ECS::Entity*> tiles;
    int selectedTile = 0;

    std::string current_file;

    void showTilePalette();

    void updatePalette(int newTile, bool update);
};
