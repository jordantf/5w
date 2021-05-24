#pragma once

// internal
#include "common.hpp"
#include "graphic.hpp"
#include "worldEditSystem.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>


struct Cutscene {
    std::string img_path;
    Cutscene (std::string path);
};

struct PauseComponent{};

class SceneSystem
{
public:
	// Creates a window
	SceneSystem(ivec2 window_size_px, WorldSystem* world);

	// Releases all associated resources
	~SceneSystem();

	// on init
	void awake();

	// start level
	void clear();

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Check for collisions
	void handle_collisions();

	// OpenGL window handle
	GLFWwindow* window;

    // Scenes
    void title();
    void worldmap();
    void area(int);
    void battle(int);
    void battle(std::string level_to_load);
    void cutscene();
    void pause(bool);
    void spellSelect(ECS::Entity player_character, Spell::SpellEffect spell_effect);
    enum SCENE{TITLE, LEVEL_SELECT, CUTSCENE, BATTLE, EDITOR} scene = TITLE;

private:

    ivec2 window_size_px;
    WorldSystem* world;
    WorldEditSystem* world_editor = nullptr;
    std::vector<Cutscene> queued_cutscenes;
    std::function<void()> end_cutscenes;

    // Input callback functions
    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 mouse_pos);
    void on_mouse_button(int button, int action, int mods);

    bool checkButtons(int button, int action, int mods);

    void editor();

};