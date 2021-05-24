#pragma once

// internal
#include "common.hpp"
#include "turn.hpp"
#include "sound.hpp"
#include "character.hpp"
#include "render.hpp"
#include "graphic.hpp"
#include "tile.hpp"
#include "text.hpp"
#include "button.hpp"
#include "MouseHandler.hpp"
#include "lightSource.hpp"
#include "utils.hpp"
#include "inventory.hpp"
// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
// For turns
#include <list>
#include <array>

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	enum SpellButton { NONE, ONE, TWO, THREE, FOUR };
	static std::string level_to_load;
	// Creates a window
	WorldSystem(ivec2 window_size_px);

	// Releases all associated resources
	~WorldSystem();

	// on init
    void awake();

	// start level
	void restart();

    void endBattle();

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);
    void set_mouse_highlight_pos(vec2 camPos);

	// Moves turn ahead
	static void step_turn();

    // Moving and attacking character
    bool move_character(Turn& turn, TilePosition& tilePos, vec2 destPos);
	static bool attack_target(ECS::Entity attacker, vec2 targetTile, bool melee);

    // Spawning characters (made public because enemies may choose to summon monsters
    void spawn_character(const std::string& type, vec2 gridPos, int ai_type, spriteType sprType, Animation * startingAnim, int movement_points, int current_HP, int HP, int attack);

	// check if two positions collisions; used in move_character()
	static bool isOccupied(vec2 src, vec2 dest);
	static bool isOccupied(vec2 location);
	// Check for collisions
	void handle_collisions();

	// Renders our scene
	void draw();

	// Should the game be over ?
	bool is_over() const;
    static bool in_game;

    // Game state
    void start_game();

	// OpenGL window handle
	GLFWwindow* window;

	// Pause
	static bool isGamePaused;

	std::function<void(bool)> showPause;
    std::function<void(ECS::Entity, Spell::SpellEffect spell_effect)> showSpellSelect;
	std::function<void()> endLevel;

	// Turn Delay
    static float currentTurnDelay;
	
	constexpr static const float turnDelayMs = 400.f;
    static bool stepping_turn;

	static bool is_wall(vec2 location);
	static bool is_hole(vec2 location);
	static bool is_goal(vec2 location);

    // Line of sight for players and enemies
    static bool hasLoS(vec2 begin, vec2 end);

    std::list<ECS::Entity> turn_queue;

	// return a random empty position, used for spawning in random postion.
	vec2 findRandomPosition();

    static SoundSystem sound;
	// note: this is hardcoded for now
	void play_sound();
	static ECS::Entity findCharacterAt(vec2 loc);
	static std::vector<std::vector<int>> world_grid;

	enum Tiletype {FLOOR0, FLOOR1, FLOOR2, FLOOR3, FLOOR4, FLOOR5,
	        WALL0, WALL1, WALL2, WALL3, WALL4, WALL5,
	        WALLTOP0, WALLTOP1, WALLTOP2, WALLTOP3, WALLTOP4, WALLTOP5};

    static std::vector<std::vector<int>> world_grid_tileset;

    // Input callback functions
    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 mouse_pos, bool setY = false, int y = 0);
    void on_mouse_button(int button, int action, int mods);

    bool resetCamera = false;

    // Mouse position for buttons
    static vec2 mouse_position;
    MouseHandler mouseHandler;

    // Area
    static void setArea(int areaNum);
    static int getArea();
    // Boss specific

    void loadShopkeeper(bool bossmode);
    void unloadShopkeeper();

    // area saving and loading
    void load_area(const std::string& name);
    void save_area(const std::string& name);

    static void loadTiles();

    static void GenerateBoundingEdges();

    void load_chars(const std::string &name);

    int selected_button;
    vec2 camera_origin;	
		static bool isDraggingCamera;

    // Update spell button
    void updateSpellButton(SpellButton button, Spell::SpellEffect spell_effect);
private:

    struct Shopkeeper{};

    // mouse highlighting
    vec2 highlighted_tile;
    vec2 window_size;

    // Loads the audio
	void init_audio();
	void setWorldGrid();
    // world grid declaration: column-major order
    // std::array<std::array<int, 17>, 19> world_grid;

    bool enableMelee;
    static float base_chrab;

	// Deprecated
	unsigned int points;
	ECS::Entity weapon_bg;
	ECS::Entity weapon_graphic;

	//
    void complete_turn_step();

	float current_speed;
	bool can_act;

	// menu funcs
	void on_click_move();
	void on_click_attack();
	void on_click_end_turn();

	void on_click_toggle_melee_attack();
	void on_click_toggle_ranged_attack();

	// testspell

	void on_click_toggle_spell_with_target(SpellButton button);

    // TODO: remove temporary bool and vec2 (this is just for when clickable grid is not yet implemented)
	bool target_select_mode;
	vec2 selected_tile;

	// bool for checking if attack is ranged
	bool ranged_attack_mode;

	ECS::Entity current_character;
	static int current_area;

    // Create spells for player to use
    void createSpells();
    std::vector<Spell> spells_cache;


	ECS::Entity cursor;
	ECS::Entity create_cursor(vec2 tilepos, std::string path);
    void updateCursorPosition(vec2 tilepos);

	// music references
	Mix_Music* background_music;
	Mix_Chunk* button_click_sound;
    Mix_Chunk* character_move_sound;

    // Font references
    std::shared_ptr<Font> serif;
    std::shared_ptr<Font> sans;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	// helper for hasLos
	static bool singleHasLoS(vec2 begin, vec2 end);

    // Death
    bool on_death(ECS::Entity entity);

    void load_interactibles();

    void load_inventory(ECS::Entity player);
    void save_inventory(Inventory inv);

    static ECS::Entity buttons[7];
};
