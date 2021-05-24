// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "wall.hpp"
#include "worldEditSystem.hpp"
#include "render_components.hpp"
#include "turn.hpp"
#include "background.hpp"
#include "player.hpp"
#include "string"
#include "Enemy1AI.hpp"
#include "Enemy2AI.hpp"
#include "Enemy3AI.hpp"
#include "Enemy4AI.hpp"
#include "Enemy5AI.hpp"
#include "Enemy6AI.hpp"
#include "projectile.hpp"
#include "tile.hpp"
#include "particles.hpp"
#include "../ext/json/single_include/nlohmann/json.hpp"
using jsonf = nlohmann::json;
#include "power.hpp"
#include "spellpower.hpp"
#include "boomerang.hpp"
#include "artifact.hpp"
#include "money.hpp"
#include "ui.hpp"
#include "health.hpp"
#include "animSystem.hpp"
#include "Enemy7AI.hpp"
#include "inventory.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sound.hpp>

// Game configuration
using json = nlohmann::json;
std::string WorldSystem::level_to_load = "area1test.json";
bool WorldSystem::isDraggingCamera = false;
bool WorldSystem::isGamePaused = false;
bool WorldSystem::in_game = false;
vec2 WorldSystem::mouse_position;
float WorldSystem::currentTurnDelay = WorldSystem::turnDelayMs;
SoundSystem WorldSystem::sound = SoundSystem();
bool WorldSystem::stepping_turn = false;
int WorldSystem::current_area;
float WorldSystem::base_chrab = 0;
ECS::Entity WorldSystem::buttons[7];
const int MAX_ACTIVE_CHARACTER_RANGE = 8;

Button* meleeButton;
Button* rangedAttackButton;

std::vector<std::vector<int>> WorldSystem::world_grid;
std::vector<std::vector<int>> WorldSystem::world_grid_tileset;

// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
	points(0)
{
	// Pause: initialize game pause state
    isGamePaused = false;
    isDraggingCamera = false;
	// initialize turn delay
	currentTurnDelay = WorldSystem::turnDelayMs;

	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());

	// Playing background music indefinitely
	//init_audio();
    SoundSystem::init_audio();
    turn_queue = std::list<ECS::Entity>();
    UISystem::turn_queue = &turn_queue;
    UISystem::turn_order_position = {window_size_px.x - 112, 70};
    
}

// code from WorldSystem() world grid initialization
// won't need this anymore
void WorldSystem::setWorldGrid() {
    // world grid initialization
    int width = world_grid.size();
    int height = world_grid[0].size();
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                world_grid[x][y] = Tile::WALL;
            }
            else {
                world_grid[x][y] = Tile::FLOOR;
            }
        }
    }
}

WorldSystem::~WorldSystem(){
	// Destroy music components
	//if (background_music != nullptr)
	//	Mix_FreeMusic(background_music);
	//Mix_CloseAudio();

	// Destroy all created components
	ECS::ContainerInterface::clear_all_components();
//
//	// Close the window
//	glfwDestroyWindow(window);
}

// Moved to sound.cpp
void WorldSystem::init_audio()
{
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");

	background_music = Mix_LoadMUS(audio_path("area_1_theme.wav").c_str());
    button_click_sound = Mix_LoadWAV(audio_path("button.wav").c_str());

	if (background_music == nullptr || button_click_sound == nullptr)
		throw std::runtime_error("Failed to load sounds make sure the data directory is present: "+
		    (audio_path("area_1_theme.wav"),
			 audio_path("button.wav")));

}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	auto& bg = ECS::registry<Background>;
     for (unsigned int i=0; i< bg.components.size(); i++)
    {
        auto& motion = ECS::registry<Motion>.get(bg.entities[i]);

        motion.position = vec2({window_size_in_game_units.x/2.0, window_size_in_game_units.y/2.0 });
    }

    // Processing the state
    assert(ECS::registry<ScreenState>.components.size() <= 1);
    auto& screen = ECS::registry<ScreenState>.components[0];

    for (auto& entity : ECS::registry<Character>.entities) {
        // Death!
        if (ECS::registry<Character>.get(entity).currentHealth <= 0) {
            if (on_death(entity)) {
                screen.chrom_abb = 0;
                screen.bloom_factor = 0;
                return;
            }
        }
    }

    // TODO: consider making a superclass for all grabable items
    // handle power item

    for (auto& player : ECS::registry<Inventory>.entities) {
        auto& tilePos = ECS::registry<TilePosition>.get(player);
        for (auto& power_e : ECS::registry<Power>.entities) {
            auto& power_tile = ECS::registry<TilePosition>.get(power_e);
            if (isOccupied(tilePos.grid_pos, power_tile.grid_pos)) {
                enableMelee = true;
                Power p = ECS::registry<Power>.get(power_e);
                vec2 pos = ECS::registry<TilePosition>.get(current_character).screen_pos;
                ParticleSystem::createParticleBurst("particles/star.png", 30, 50, 100,
                                                    pos, pos, {1000, 2000}, 0, 100, {0, 0}, 0.99, 20, 40, 0, {1.0, 1.0}, 0.3);
                if (p.name == "rusty shortsword") {
                    // remove end turn button
                    ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities[ECS::registry<Button>.size()-1]);

                    ECS::Entity button;
                    button = Button::createButton({580, 750}, 64, 64,
                                                  [this](){this->on_click_toggle_melee_attack();},
                                                  "ui/spell_icons/regular/sword_icon.png", "ui/spell_icons/selected/sword_icon.png", "ui/spell_icons/selected/sword_icon.png", true);
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "Strike\nattack enemies with a powerful melee blow.\nCost: @"));

                    ECS::registry<BattleUI>.emplace(button, BattleUI::M_ATTACK);
                    ECS::registry<JODBUTTON>.emplace(button);


                    button = Button::createButton({980, 750}, 64, 64,
                                                  [this]() { this->on_click_end_turn(); },
                                                  "ui/spell_icons/regular/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png");
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "End player turn."));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
                    ECS::registry<JODBUTTON>.emplace(button);

                    ECS::registry<TempText>.emplace(UISystem::showText(
                            "You picked up a sword!\nGo attack the slime", {400, 500}, 0.6f), 10000.f);

                } else if (p.name == "boomerang") {
                    // remove end turn button to preserve button order
                    ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities[ECS::registry<Button>.size()-1]);
                    ECS::Entity button;
                    button = Button::createButton({660, 750}, 64, 64,
                                                  [this](){this->on_click_toggle_ranged_attack();},
                                                  "ui/spell_icons/regular/boom_icon.png", "ui/spell_icons/selected/boom_icon.png", "ui/spell_icons/selected/boom_icon.png", true);
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "Boomerang\nthrow a bouncing boomerang\nto attack your foes"
                                                                  "\nCost: @"));

                    ECS::registry<BattleUI>.emplace(button, BattleUI::R_ATTACK);
                    ECS::registry<JODBUTTON>.emplace(button);


                    button = Button::createButton({980, 750}, 64, 64,
                                                  [this]() { this->on_click_end_turn(); },
                                                  "ui/spell_icons/regular/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png");
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "End player turn."));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
                    ECS::registry<JODBUTTON>.emplace(button);

                    ECS::registry<TempText>.emplace(UISystem::showText(
                            "You picked up a boomerang! This is a\nranged weapon that can pierce \n multiple enemies."
                            "Try it out on that\n line of slimes!", {400, 500}, 0.6f), 16000.f);

                } else if (p.name == "hook tome") {
                    // remove end turn button to preserve button order
                    ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities[ECS::registry<Button>.size()-1]);
                    ECS::Entity button;

                    button = Button::createButton({740, 750}, 64, 64,
                                                  [this]() {
                                                      this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::ONE);
                                                  },
                            // TODO testspell change image.
                                                  "ui/spell_icons/regular/hook_icon.png", "ui/spell_icons/selected/hook_icon.png",
                                                  "ui/spell_icons/selected/hook_icon.png", true);
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "HOOK to a location at, or adjacent to,\n a wall\nCost: ||"));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::SPELL1);
                    ECS::registry<JODBUTTON>.emplace(button);


                    button = Button::createButton({980, 750}, 64, 64,
                                                  [this]() { this->on_click_end_turn(); },
                                                  "ui/spell_icons/regular/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png");
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "End player turn."));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
                    ECS::registry<JODBUTTON>.emplace(button);

                    ECS::registry<TempText>.emplace(UISystem::showText(
                            "You got a grappling hook spell. This allows you\nto grapple next to walls,"
                            "\nand can take you over ravines.", {400, 500}, 0.6f), 18000.f);
                } else if (p.name == "switch tome") {
                    // remove end turn button to preserve button order
                    ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities[ECS::registry<Button>.size()-1]);
                    ECS::Entity button;

                    button = Button::createButton({820, 750}, 64, 64,
                                                  [this]() {
                                                      this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::TWO);
                                                  },
                            // TODO testspell change image.
                                                  "ui/spell_icons/regular/switch_icon.png",
                                                  "ui/spell_icons/selected/switch_icon.png",
                                                  "ui/spell_icons/selected/switch_icon.png", true);
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "SWAP location with an enemy\nCost: |"));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::SPELL2);
                    ECS::registry<JODBUTTON>.emplace(button);

                    button = Button::createButton({980, 750}, 64, 64,
                                                  [this]() { this->on_click_end_turn(); },
                                                  "ui/spell_icons/regular/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png");
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "End player turn."));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
                    ECS::registry<JODBUTTON>.emplace(button);

                    ECS::registry<TempText>.emplace(UISystem::showText(
                            "This spell allows you to switch places\nwith an enemy. Use it to clear\nyour path.", {400, 500}, 0.6f), 12000.f);
                } else if (p.name == "push tome") {
                    // remove end turn button to preserve button order
                    ECS::ContainerInterface::remove_all_components_of(ECS::registry<Button>.entities[ECS::registry<Button>.size()-1]);
                    ECS::Entity button;
                    button = Button::createButton({900, 750}, 64, 64,
                                                  [this]() {
                                                      this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::THREE);
                                                  },
                            // TODO testspell change image.
                                                  "ui/spell_icons/regular/push_icon.png", "ui/spell_icons/selected/push_icon.png",
                                                  "ui/spell_icons/selected/push_icon.png", true);
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "PUSH a nearby enemy\nCost: |"));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::SPELL3);
                    ECS::registry<JODBUTTON>.emplace(button);

                    button = Button::createButton({980, 750}, 64, 64,
                                                  [this]() { this->on_click_end_turn(); },
                                                  "ui/spell_icons/regular/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png",
                                                  "ui/spell_icons/selected/endturn_icon.png");
                    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                                  "End player turn."));
                    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
                    ECS::registry<JODBUTTON>.emplace(button);
                    ECS::registry<TempText>.emplace(UISystem::showText(
                            "This spell allows you to push\nany enemy. Use it to clear\nyour path.", {400, 500}, 0.6f), 12000.f);
                }

                Character &c = ECS::registry<Character>.get(player);
                Power power = ECS::registry<Power>.get(power_e);
                if (power.point > c.meleeDamage) {
                    c.meleeDamage = power.point;
                    sound.play("reward");
                    std::cout << "player picked up POWER ITEM, new damage: " << c.meleeDamage << std::endl;
                    ECS::registry<Weapon>.remove(current_character);
                    Weapon &w = ECS::registry<Weapon>.emplace(current_character, power.name, power.point);
                    auto &i = ECS::registry<Inventory>.get(player);
                    i.weapons.clear();
                    i.weapons.push_back(w);
                }
                ECS::ContainerInterface::remove_all_components_of(power_e);
                UISystem::updateUI();
            }
        }
        for (auto& spellPower_e : ECS::registry<SpellPower>.entities) {
            auto& spellPower_tile = ECS::registry<TilePosition>.get(spellPower_e);
            if (isOccupied(tilePos.grid_pos, spellPower_tile.grid_pos)) {
                Character &c = ECS::registry<Character>.get(player);
                SpellPower spellPower = ECS::registry<SpellPower>.get(spellPower_e);
                sound.play("reward");
                ECS::ContainerInterface::remove_all_components_of(spellPower_e);

                showSpellSelect(current_character, spellPower.spell_effect);
                // UISystem::updateUI();
            }
        }
        for (auto& artifact : ECS::registry<Artifact>.entities) {
            auto& artifact_tile = ECS::registry<TilePosition>.get(artifact);
            if (isOccupied(tilePos.grid_pos, artifact_tile.grid_pos)) {
                auto &character = ECS::registry<Character>.get(player);
                if (character.currentHealth > 100)
                    character.currentHealth = 150;
                else
                    character.currentHealth += 50;

                ECS::registry<Player>.get(player).hasArtifact = true;
                vec2 pos = ECS::registry<TilePosition>.get(current_character).screen_pos;
                ParticleSystem::createParticleBurst("particles/star.png", 30, 50, 100,
                                                    pos, pos, {1000, 2000}, 0, 100, {0, 0}, 0.99, 20, 40, 0, {1.0, 1.0}, 0.3);

                auto& i = ECS::registry<Inventory>.get(player);
                auto& a = ECS::registry<Artifact>.get(artifact);
                bool newArtifact = true;
                for (auto& a_0 : i.artifacts) {
                    if (a_0.area == a.area) {
                        newArtifact = false;
                        break;
                    }
                }
                if (newArtifact) i.artifacts.push_back(a);
                ECS::ContainerInterface::remove_all_components_of(artifact);
            }
        }

        for (auto& money : ECS::registry<Money>.entities) {
            auto& money_tile = ECS::registry<TilePosition>.get(money);
            if (isOccupied(tilePos.grid_pos, money_tile.grid_pos)) {
                auto& inventory = ECS::registry<Inventory>.get(player);
                auto& money_values = ECS::registry<Money>.get(money);
                inventory.gold += money_values.value;
                std::cout << "Gold: " << inventory.gold;
                ECS::ContainerInterface::remove_all_components_of(money);
            }
        }
		if (is_goal(tilePos.grid_pos)) {
            if (ECS::registry<Player>.get(player).hasArtifact == true && ECS::registry<Shopkeeper>.components.empty()) {
                save_inventory(ECS::registry<Inventory>.components[0]); // There should always be an inventory component at any point
                endLevel();
                return;
            } else {
                if (!ECS::registry<TempText>.has(player)) {
                    ECS::registry<TempText>.emplace(player, 10000.f);
                    UISystem::showText(player,
                                       "You aren't ready to leave yet!\nFind the artifact first!", {450, 300}, 0.6f);
                }
            }
        }

        for (auto& ke : ECS::registry<Key>.entities) {
            auto& key_pos = ECS::registry<TilePosition>.get(ke);
            if (isOccupied(tilePos.grid_pos, key_pos.grid_pos)) {
                auto& inventory = ECS::registry<Inventory>.get(player);
                auto& key = ECS::registry<Key>.get(ke);
                inventory.keys.push_back(key);
                ECS::ContainerInterface::remove_all_components_of(ke);
            }
        }
    }

    for (auto& t : ECS::registry<TempText>.entities) {
        auto& temp = ECS::registry<TempText>.get(t);
        temp.counter -= elapsed_ms;
        if (ECS::registry<Text>.has(t)) {
            auto& text = ECS::registry<Text>.get(t);
            text.opacity = max(min(temp.counter/temp.init_counter, text.opacity), 0.f);
            text.position += elapsed_ms/1000 * temp.movement;
        } else {
            ECS::registry<TempText>.remove(t);
        }
        if (temp.counter <= 0) {
            ECS::registry<TempText>.remove(t);
            ECS::registry<Text>.remove(t);
        }
    }

    for (auto& ie : ECS::registry<Interactible>.entities) {
        auto& pos = ECS::registry<TilePosition>.get(ie);
        auto& interactible = ECS::registry<Interactible>.get(ie);
        bool foundPlayer = false;
        for (auto& p : ECS::registry<Player>.entities) {
            auto& player_pos = ECS::registry<TilePosition>.get(p);
            if (withinRange(player_pos.grid_pos,pos.grid_pos, 1)) {
                interactible.toggleAdjacent(true, &p);
                foundPlayer = true;
                break;
            }
        }
        if (!foundPlayer)
            interactible.toggleAdjacent(false, nullptr);
    }

    // Update timers and health bars on all hurt entities
    for (auto& entity : ECS::registry<HurtQueue>.entities) {
        auto& hq = ECS::registry<HurtQueue>.get(entity);
        auto& hurt_entity = hq.q.front();
        bool decremented = false;
        if (hurt_entity.delayCounterMs > 0) {
            hurt_entity.delayCounterMs -= elapsed_ms;
        } else if (hurt_entity.counterMs > 0) {
            float init_counter_value = hurt_entity.counterMs;
            hurt_entity.counterMs -= elapsed_ms;
            float new_counter_value = max(0.f, hurt_entity.counterMs);
            float elapsed = init_counter_value - new_counter_value;
            float frac = elapsed / hurt_entity.initCounterMs;

            // VISUAL EFFECTS
            float frac_screen = hurt_entity.counterMs / hurt_entity.initCounterMs;
            screen.chrom_abb = decremented ? max(screen.chrom_abb,
                                                 frac_screen * 0.5f + base_chrab) :
                               frac_screen * 0.5f + base_chrab;
            screen.bloom_factor = decremented ? max(screen.bloom_factor,
                                                    float(pow(frac_screen, 2)) * 0.5f) :
                                  pow(frac_screen, 2) * 0.5f;

            if (!ECS::registry<Text>.has(entity)) {
                vec2 pos = ECS::registry<TilePosition>.get(entity).screen_pos - RenderSystem::get_cam_offset();
                UISystem::showText(entity, std::to_string(hurt_entity.damage), pos, hurt_entity.damage / 20 + 1.0);
                if (ECS::registry<TempText>.has(entity))
                    ECS::registry<TempText>.remove(entity);
                ECS::registry<TempText>.insert(entity, TempText(init_counter_value, {0, -20}));
                auto &text = ECS::registry<Text>.get(entity);
                text.outline = true;
            }

            if (hurt_entity.damage == 0) continue;

            auto &targetChar = ECS::registry<Character>.get(entity);
            targetChar.currentHealth = hurt_entity.counterMs / hurt_entity.initCounterMs * hurt_entity.damage +
                                       hurt_entity.finalHealth;
            float healthPct = ((float) targetChar.currentHealth) / ((float) targetChar.maxHealth);

            ECS::registry<HealthBar>.remove(entity);
            TilePosition &entity_pos = ECS::registry<TilePosition>.get(entity);
            Health::createHealthBar(entity, healthPct, entity_pos.grid_pos);
            decremented = true;
        } else {
            // Destroy hurt struct
            screen.chrom_abb = base_chrab;
            screen.bloom_factor = 0;
            auto &targetChar = ECS::registry<Character>.get(entity);
            if (hurt_entity.damage != 0)
                targetChar.currentHealth = hurt_entity.finalHealth;
            hq.q.pop_front();

            if (hq.q.empty()) {
                ECS::registry<HurtQueue>.remove(entity);
            }
            UISystem::destroyText(entity);
        }
    }

    // step turn once delay is over
    if(stepping_turn) complete_turn_step();

	// current_character should be the one

	auto& turn = ECS::registry<Turn>.get(current_character);

    can_act = false;
	if (turn.currentAction == turn.IDLE && !ECS::registry<Moving>.has(current_character)) {
        if (ECS::registry<Player>.has(current_character)) {
            // Take inputs
            // TODO: Remember to set turn.currentAction to MOVING/ATTACKING if those inputs are made
            // TODO: When movement is finished, set state to turn.IDLE
            Player& player = ECS::registry<Player>.get(current_character);
            can_act = true;
            if (player.hasPath) {
                player.move();
                UISystem::updateUI();
            }
        } else {
            // TODO: enemy AI
            // regardless, will call step_turn();
            assert(ECS::registry<EnemyAIContainer>.has(current_character)); // enemy should have AI!
            auto &enemyAI = ECS::registry<EnemyAIContainer>.get(current_character);
            enemyAI->beginTurn();
        }
    }

	if(DebugSystem::in_debug_mode) {
	    std::cout << ECS::registry<Edge>.components.size() << "\n";
	    for(auto& edge : ECS::registry<Edge>.components) {
	        float thickness = 8.f;
	        if (edge.is_3d)
	            thickness = 3.f;
	        if(edge.start.x == edge.end.x) {
                float height = edge.end.y - edge.start.y;
                DebugSystem::createLineAbs(vec2(edge.start.x * 60+200, (edge.end.y + edge.start.y) * 30+200),
                                           vec2(thickness, height * 60));
            }
	        else if (edge.start.y == edge.end.y) {
                float width = edge.end.x - edge.start.x;
                DebugSystem::createLineAbs(vec2((edge.end.x + edge.start.x) * 30+200, edge.start.y * 60+200),
                                           vec2(width * 60, thickness));
	        }
	    }

        if(ECS::registry<V>.size() > 0) {
            for (auto &v : ECS::registry<V>.components) {
                DebugSystem::createLineAbs(v.pos*60.f+200.f, vec2(15, 15));
            }
        }
	}
}

// Step to next turn
void WorldSystem::step_turn() {
    if (!ECS::registry<HurtQueue>.components.empty()) return;
    // update turn timer
    if (stepping_turn) return;
    currentTurnDelay = 0.f;
    stepping_turn = true;
    vec2 loc;
    if (!ECS::registry<Turn>.entities.empty()) {
        auto entity = ECS::registry<Turn>.entities[0];
        loc = ECS::registry<TilePosition>.get(entity).screen_pos;
    }
        

}

void WorldSystem::complete_turn_step() {

    stepping_turn = false;

    assert(!turn_queue.empty());
    // std::cout << "current character before" << current_character.id;
    // Move entity to back
    // Current character to be shuffled to back must be the same as the entity currently with Turn!
    assert(current_character.id == ECS::registry<Turn>.entities[0].id);

    ECS::registry<Turn>.remove(current_character);

    // First character in the queue was the one having the current turn (before calling step_turn())
    turn_queue.pop_front();
    // Queue up old character at back
    turn_queue.push_back(current_character);

    // Find the next character within range and give them the turn
    // This loop is guaranteed to exit when the turn queue returns back to the player (as long as there is only 1 player)
    while(1) {
        auto new_char = turn_queue.front();
        TilePosition& tile_position = ECS::registry<TilePosition>.get(new_char);
        TilePosition& player_pos = ECS::registry<TilePosition>.get(ECS::registry<Player>.entities[0]);
        // get approximate diagonal distance
        int x = player_pos.grid_pos.x - tile_position.grid_pos.x;
        int y = player_pos.grid_pos.y - tile_position.grid_pos.y;
        if ((pow(x, 2) + pow(y, 2)) > pow(MAX_ACTIVE_CHARACTER_RANGE, 2)) {
            std::cout << "Skipping turn: Too far: " << pow(x, 2) + pow(y, 2) << std::endl;
            turn_queue.pop_front();
            turn_queue.push_back(new_char);
        }
        else {
            auto& c = ECS::registry<Character>.get(new_char);
            ECS::registry<Turn>.emplace(new_char, c);
            break;
        }
    }

    auto& turn = ECS::registry<Turn>.get(turn_queue.front());
    auto& character = ECS::registry<Character>.get(turn_queue.front());
    turn.movement = character.max_movement;
    turn.attacks = character.max_attacks;
    turn.spells = character.max_casts;

    current_character = turn_queue.front();

//    std::cout << "STEPPING TURN: New turn order is" << std::endl;
//
//    int i = 1;
//    for (auto entity : turn_queue) {
//        assert(ECS::registry<Character>.has(entity));
//        auto& character = ECS::registry<Character>.get(entity);
//        std::cout << i << ": " << character.name << std::endl;
//        i++;
//    }
    UISystem::updateTurn();

    Character& current_char = ECS::registry<Character>.get(current_character);
    std::cout << "Current character is: " << current_char.name << std::endl;
    std::cout << "HP: " << current_char.currentHealth << "/" << current_char.maxHealth << std::endl;

    if (ECS::registry<Player>.has(current_character)) {
        *UISystem::current_player = current_character;
        UISystem::updateUI();


        ECS::registry<Button>.components[selected_button].off_select();
        selected_button = 0;
        auto& target_button = ECS::registry<Button>.components[selected_button];
        target_button.notify(true, target_button.centre, [this]() {});
//        target_button.on_select();
//        on_click_move();
        UISystem::updateButtons();
    }

}

void WorldSystem::awake() {// Debugging for memory/component leaks
    std::cout << "Waking\n";

    mouseHandler = MouseHandler();

    // Reset the game speed
    current_speed = 1.f;

    // Pause:
    WorldSystem::isGamePaused = false;

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
    while (ECS::registry<Motion>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

    while (ECS::registry<Character>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());

    // Debugging for memory/component leaks
//    ECS::ContainerInterface::list_all_components();

    // set in_game to false
    in_game = false;
    // set title screen
}

void WorldSystem::start_game() {
    std::cout << "world::startgame\n";
    in_game = true;
}

// TODO: move menu button funcs to separate file?
void WorldSystem::on_click_move() {
    auto& b_0 = ECS::registry<Button>.components[selected_button];
    b_0.off_select();
    selected_button = 0;
    auto& button = (current_area==0) ?
              ECS::registry<Button>.components[0] :
              ECS::registry<Button>.get(buttons[0]);
    button.on_select();
    // TODO: grey out buttons when player cannot move
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character)) {
        auto &player = ECS::registry<Player>.get(current_character);
        player.on_click_move();
        player.show_tiles();
    }
}

void WorldSystem::on_click_attack() {
    // TODO: grey out buttons when player cannot attack
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character))
    {
        auto &player = ECS::registry<Player>.get(current_character);
        player.on_click_attack();
    }
}

void WorldSystem::on_click_end_turn() {
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character)) {
        auto &player = ECS::registry<Player>.get(current_character);
        player.on_click_end_turn();
    }
}

// todo: move this to buttons
void WorldSystem::on_click_toggle_melee_attack() {
    std::cout << "toggling melee attack";
    auto& b_0 = ECS::registry<Button>.components[selected_button];
    b_0.off_select();
    selected_button = 1;
    auto& b = (current_area==0) ?
              ECS::registry<Button>.components[1] :
              ECS::registry<Button>.get(buttons[1]);
    b.on_select();
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character)) {
        auto &player = ECS::registry<Player>.get(current_character);
        player.show_tiles();
        player.on_click_melee_attack();
    }
}

void WorldSystem::on_click_toggle_ranged_attack() {
    auto& b_0 = ECS::registry<Button>.components[selected_button];
    b_0.off_select();
    selected_button = 2;
    auto& b = (current_area==0) ?
              ECS::registry<Button>.components[2] :
              ECS::registry<Button>.get(buttons[2]);
    b.on_select();
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character)) {
        auto& player = ECS::registry<Player>.get(current_character);
        player.show_tiles();
        player.on_click_ranged_attack();
    }
}

// testspell
void WorldSystem::on_click_toggle_spell_with_target(WorldSystem::SpellButton button) {
    auto& b_0 = ECS::registry<Button>.components[selected_button];
    b_0.off_select();
    selected_button = 3 + button - 1;
    auto& b = (current_area==0) ?
            ECS::registry<Button>.components[3 + button - 1] :
            ECS::registry<Button>.get(buttons[3+button-1]);
    b.on_select();
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character)) {
        auto & player = ECS::registry<Player>.get(current_character);
        player.on_click_spell_cast(button);
        player.show_tiles();

    }
}

void WorldSystem::endBattle() {
    // Debugging for memory/component leaks
    ECS::ContainerInterface::list_all_components();
    std::cout << "Exiting battle\n";

    // initialize turn delay
    currentTurnDelay = WorldSystem::turnDelayMs;
    stepping_turn = false;

    // Pause:
    WorldSystem::isGamePaused = false;
    WorldSystem::in_game = false;

    assert(ECS::registry<ScreenState>.components.size() <= 1);
    auto& screen = ECS::registry<ScreenState>.components[0];
    base_chrab = 0;
    screen.chrom_abb = 0;
    screen.bloom_factor = 0;

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
    while (ECS::registry<Motion>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

    while (ECS::registry<Character>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());

    while (ECS::registry<TilePosition>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<TilePosition>.entities.back());

    while (ECS::registry<Interactible>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Interactible>.entities.back());

    if (ECS::registry<Edge>.components.size()>0) {
        ECS::registry<Edge>.clear();
    }

    ParticleSystem::destroyAllEmitters();

    UISystem::clearBattleUI();
    // Clear turn order
    turn_queue.clear();

    // Remove instance of turn
    for (auto entity : ECS::registry<Turn>.entities)
        ECS::registry<Turn>.remove(entity);

    // Debugging for memory/component leaks
    ECS::ContainerInterface::list_all_components();
    resetCamera = true;
}

// Reset the world state to its initial state
void WorldSystem::restart()
{
	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();
	std::cout << "Restarting\n";

    // initialize turn delay
    currentTurnDelay = WorldSystem::turnDelayMs;
    stepping_turn = false;

	// Reset the game speed
	current_speed = 1.f;

	// Pause:
	WorldSystem::isGamePaused = false;

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
    while (ECS::registry<Motion>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

    while (ECS::registry<Character>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Character>.entities.back());

    while (ECS::registry<TilePosition>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<TilePosition>.entities.back());

    while (ECS::registry<Interactible>.entities.size()>0) {
        ECS::registry<Interactible>.components.back().destroy();
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Interactible>.entities.back());
    }

    if (ECS::registry<Edge>.components.size()>0) {
        ECS::registry<Edge>.clear();
    }

    if (ECS::registry<V>.components.size()>0) {
        ECS::registry<V>.clear();
    }

    ParticleSystem::destroyAllEmitters();

    UISystem::clearBattleUI();

    // load background
    Background::changeBackground("library.png", vec2(2.3f, 2.3f));

	// Clear turn order
    turn_queue.clear();

	// Remove instance of turn
	for (auto entity : ECS::registry<Turn>.entities)
	    ECS::registry<Turn>.remove(entity);

	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();

	//level_to_load = "area" + std::to_string(current_area) + ".json";

    load_area(level_to_load);

    // setWorldGrid();
    loadTiles();

    // TODO:
    // create power after tiless
    // hack until we put this in savefile
    std::cout << "current_area: " << current_area;
    ECS::Entity power;
    if (current_area == 0) {
        power = Power::createPower("power_rusty.png", "rusty shortsword", 1, {2, 2});
        power = Power::createPower("boomerang_normal_sprite.png", "boomerang", 1, {8,3});
        power = Power::createPower("hook_tome.png", "hook tome", 1, {9, 9});
        power = Power::createPower("switch_tome.png", "switch tome", 1, {5,10});
        power = Power::createPower("push_tome.png", "push tome", 1, {2,9});
    }

    GenerateBoundingEdges();
    load_chars(level_to_load);

//    GenerateBoundingEdges();

    cursor = create_cursor(vec2(0, 0), "target_select.png");

    if (current_area == 4) {
        loadShopkeeper(false);
    }

    createSpells();

    //for some reason making these global to world causes unrelated segfault
    int buttony = 750;
    int buttonx = 500;
    int buttonSpacing = 80;

    int num=0; // helper for spacing; incremented each time
    // bottom buttons
    ECS::Entity button;
    button = Button::createButton({buttonx + buttonSpacing*num++, buttony}, 64, 64,
                                  [this](){this->on_click_move();},
                                  "ui/spell_icons/regular/move_icon.png", "ui/spell_icons/selected/move_icon.png", "ui/spell_icons/selected/move_icon.png", true);
    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                  "Select a tile to\n move to on the grid\n with the mouse."
                                                  "\nCost: # - #####"));
    ECS::registry<BattleUI>.emplace(button, BattleUI::MOVE);
    ECS::registry<JODBUTTON>.emplace(button);

    buttons[num-1] = button;


    if (current_area != 0) {

        button = Button::createButton({buttonx + buttonSpacing*num++, buttony}, 64, 64,
                                      [this](){this->on_click_toggle_melee_attack();},
                                      "ui/spell_icons/regular/sword_icon.png", "ui/spell_icons/selected/sword_icon.png", "ui/spell_icons/selected/sword_icon.png", true);
        ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                      "Strike\nattack enemies with a powerful melee blow.\nCost: @"));

        ECS::registry<BattleUI>.emplace(button, BattleUI::M_ATTACK);
        ECS::registry<JODBUTTON>.emplace(button);

        buttons[num-1] = button;

        button = Button::createButton({buttonx + buttonSpacing*num++, buttony}, 64, 64,
                                      [this](){this->on_click_toggle_ranged_attack();},
                                      "ui/spell_icons/regular/boom_icon.png", "ui/spell_icons/selected/boom_icon.png", "ui/spell_icons/selected/boom_icon.png", true);
        ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                      "Boomerang\nthrow a bouncing boomerang\nto attack your foes"
                                                      "\nCost: @"));

        ECS::registry<BattleUI>.emplace(button, BattleUI::R_ATTACK);
        ECS::registry<JODBUTTON>.emplace(button);

        buttons[num-1] = button;

    }

    button = Button::createButton({980, buttony}, 64, 64,
                                  [this]() { this->on_click_end_turn(); },
                                  "ui/spell_icons/regular/endturn_icon.png",
                                  "ui/spell_icons/selected/endturn_icon.png",
                                  "ui/spell_icons/selected/endturn_icon.png");
    ECS::registry<Tooltip>.insert(button, Tooltip({850, 40},
                                                  "End player turn."));
    ECS::registry<BattleUI>.emplace(button, BattleUI::TURN);
    ECS::registry<JODBUTTON>.emplace(button);
//    buttons[6] = button;

    ECS::Entity e = ECS::Entity();
    ECS::registry<ButtonSelector>.insert(e, ButtonSelector({ 850, 40 }));
    UISystem::showUI(UISystem::BATTLE);

    ECS::Entity turn = UISystem::showText("TURN ORDER",
                                          {UISystem::turn_order_position.x - 50, UISystem::turn_order_position.y - 40}, 0.5f);
    ECS::registry<BattleUI>.emplace(turn, BattleUI::HUD);
    UISystem::updateTurn();

    // set default character action to movement
    selected_button = 0;
    auto& target_button = ECS::registry<Button>.components[selected_button];
    target_button.notify(true, target_button.centre, [this]() {});
//    target_button.on_select();
//    on_click_move();
    UISystem::updateButtons();

    load_interactibles();

    assert(!WorldSystem::isOccupied({ 4,1 }));
}

void WorldSystem::loadTiles() {// render pass for world grid
    int width = world_grid.size();
    int height = world_grid[0].size();

    // Resize world_grid_tileset if size not equal to world_grid size
    if (world_grid.size() != world_grid_tileset.size()) {
        // Resets tileset:
        world_grid_tileset.resize(width);
        for (int i = 0; i < width; ++i) {
            world_grid_tileset[i].resize(height);
        }
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                if (world_grid[x][y] == Tile::WALL) {
                    world_grid_tileset[x][y] = WALLTOP0;
                } else if (world_grid[x][y] == Tile::FLOOR) {
                    world_grid_tileset[x][y] = FLOOR0;
                } else {
                    world_grid_tileset[x][y] = FLOOR0;
                }
            }
        }
    }

    // Create the tiles
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            ECS::Entity entity = TilesheetTile::createTile({x, y}, world_grid_tileset[x][y], world_grid[x][y], current_area, "tileset");
        }
    }
}

ECS::Entity WorldSystem::create_cursor(vec2 tilepos, std::string path)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    ECS::registry<Cursor>.emplace(entity);

    auto& tile_pos = ECS::registry<TilePosition>.emplace(entity);
    tile_pos.grid_pos = tilepos;
    tile_pos.screen_pos = tilepos * vec2(100, 100);
    tile_pos.scale = vec2(100, 100);
    tile_pos.isSolid = false;

    return entity;
}

void WorldSystem::updateCursorPosition(vec2 tilepos) {
    if(ECS::registry<TilePosition>.size() == 0) return;

    if(!ECS::registry<TilePosition>.has(cursor)) return;
    auto& tpos = ECS::registry<TilePosition>.get(cursor);
    tpos.grid_pos = tilepos;
    tpos.screen_pos = tilepos * vec2(100, 100);

    for(auto& e : ECS::registry<TilePosition>.components) {
        if(e.grid_pos == tpos.grid_pos)e.lit = true;
        else e.lit = false;
    }
}

void WorldSystem::spawn_character(const std::string& type, vec2 gridPos, int ai_type, spriteType sprType, Animation * startingAnim, int movement_points, int current_HP, int HP, int attack) {
    ECS::Entity entity;

    //create char based on sprite type
    if(sprType == Animated) {
        entity = Character::createAnimatedCharacter(type, gridPos, *startingAnim, movement_points, HP, attack);
        // TODO: all instances of "100" need to be changed to allow for save/load to respect HP values
		ECS::registry<Character>.get(entity).currentHealth = current_HP;
        Health::createHealthBar(entity, 1.0, gridPos);
    }
    else if(sprType == Textured) entity = Character::createCharacter(type, gridPos, movement_points);

    turn_queue.push_back(entity);
    switch (ai_type) {
        case 1: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy1AI(entity, this)); return;
        case 2: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy2AI(entity, this)); return;
        case 3: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy3AI(entity, this, false)); return;
        case 4: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy4AI(entity, this, "trident", ivec2(3,5))); return;
        case 5: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy5AI(entity, this)); return;
        case 6: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy3AI(entity, this, true)); return;
        case 7: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy6AI(entity, this)); return;
        case 8: ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy7AI(entity, this)); return;
        default:
            ECS::registry<EnemyAIContainer>.emplace(entity, new Enemy1AI(entity, this)); return;
    }
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
    // Loop over all collisions detected by the physics system
    auto &registry = ECS::registry<PhysicsSystem::Collision>;
    for (unsigned int i = 0; i < registry.components.size(); i++) {
        // The entity and its collider
        auto entity = registry.entities[i];
        auto entity_other = registry.components[i].other;
    }

    // Remove all collisions from this simulation step
    ECS::registry<PhysicsSystem::Collision>.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
	return glfwWindowShouldClose(window)>0;
}


// find a character at the given tilepos.
ECS::Entity WorldSystem::findCharacterAt(vec2 loc) {
    for (auto entity : ECS::registry<TilePosition>.entities) {
        TilePosition& entity_pos = ECS::registry<TilePosition>.get(entity);
        if (isOccupied(entity_pos.grid_pos, loc) && entity_pos.isSolid && ECS::registry<Character>.has(entity)) {
            return entity;
        }
    }
    throw std::runtime_error("No entity at target");
}

vec2 WorldSystem::findRandomPosition() {
    while (true) {
        int r1 = rand() % 9 + 1;
        int r2 = rand() % 9 + 1;
        if (!(WorldSystem::is_wall({ r1,r2 }) || WorldSystem::isOccupied({ r1,r2 }))) {
            return { r1,r2 };
        }
    }
    return{ 0, 0 };
}

// Returns true if movement was successful
bool WorldSystem::move_character(Turn& turn, TilePosition& tilePos, vec2 destPos) {

    int dest_x = destPos.x;
    int dest_y = destPos.y;
    if ((dest_x < 0) || (dest_x > world_grid.size() - 1) || (dest_y < 0) || (dest_y > world_grid[0].size() - 1)) {
        std::cout << "Trying to move off the edge of the world into (" << dest_x << "," << dest_y << ")" << std::endl;
        return false; // Trying to move off the edge of the world
    }
    // Tile in-bounds
    int destinationTile = world_grid[dest_x][dest_y];
    if (destinationTile == Tile::WALL) {
        std::cout << "Trying to move into wall at (" << dest_x << "," << dest_y << ")" << std::endl;
        return false;
    }
    else {
        // find power item
        bool foundPower = false;
        ECS::Entity power_entity;
        // not a wall; check if tile is occupied
        for (auto entity : ECS::registry<TilePosition>.entities) {
            TilePosition &entity_pos = ECS::registry<TilePosition>.get(entity);
            vec2 enemy_pos = entity_pos.grid_pos;
            if (isOccupied(destPos, enemy_pos)) {
                if (entity_pos.isSolid) {
                    std::cout << "Trying to move into an occupied tile at (" << dest_x << "," << dest_y << ")"
                        << std::endl;
                    return false;
                }
            }
        }
        // handle movement
        ECS::registry<Moving>.emplace(current_character);
        auto &moving = ECS::registry<Moving>.get(current_character);
        moving.oldPos = tilePos.grid_pos;
        turn.currentAction = turn.MOVING;
        turn.movement--;
        tilePos.grid_pos.x = dest_x;
        tilePos.grid_pos.y = dest_y;
        sound.play(current_character, WALK);
        moving.step = (tilePos.grid_pos - moving.oldPos) * 0.05f;

        std::cout << "Moved into valid space at (" << dest_x << "," << dest_y << ")" << std::endl;
        return true;
    }
}
bool WorldSystem::isOccupied(vec2 src, vec2 dest) {
    return src == dest;
    // return src.x == dest.x && src.y == dest.y;
}
bool WorldSystem::isOccupied(vec2 loc) {
    for (auto entity : ECS::registry<TilePosition>.entities) {

        TilePosition& entity_pos = ECS::registry<TilePosition>.get(entity);
        vec2 enemy_pos = entity_pos.grid_pos;
        if (isOccupied(enemy_pos, loc) && entity_pos.isSolid) {
            return true;
        }
    }
    return false;
}


// New: encapsulate some code to findCharacterAt()
// Computes the effects of the attack on the target. Attacker must be having the active turn!
//
// current_char_turn: the Turn component of the character initiating the attack
// target: The entity (has to have a character component!!!) who is being attacked
// returns true if there was a target to be attacked, else returns false
bool WorldSystem::attack_target(ECS::Entity attacker, vec2 targetTile, bool melee) {
    std::cout << "Tried to attack space at (" << targetTile.x << "," << targetTile.y << ")" << std::endl;

    // Determine damage is ranged or not (if target_select_mode is true, then it's ranged)
    int damage = !melee ?
        ECS::registry<Character>.get(attacker).rangedDamage :
        ECS::registry<Character>.get(attacker).meleeDamage;
    // Attacker should have active turn
    assert(ECS::registry<Turn>.has(attacker));
    Turn& current_char_turn = ECS::registry<Turn>.get(attacker);

    for (auto entity : ECS::registry<TilePosition>.entities) {
        TilePosition &entity_pos = ECS::registry<TilePosition>.get(entity);
        if (entity_pos.grid_pos.x == targetTile.x && entity_pos.grid_pos.y == targetTile.y
            && ECS::registry<Character>.has(entity) &&
            // make sure entity is not self!
            (entity.id != attacker.id)) {
            // entity is target!
            Character& targetChar = ECS::registry<Character>.get(entity);
            float angle = RenderSystem::cartesianTheta(targetTile - ECS::registry<TilePosition>.get(attacker).grid_pos);
            ParticleSystem::createParticleBurst("particles/damage_spark.png",50, 200.f, 400.f,
                                                100.f * targetTile - 50.f, 100.f * targetTile + vec2(50, 50), {1000.f, 2000.f},
                                                angle, 0.4, {0.f, 15.f}, 0.95, 10.0, 30.f, -5.f, {1.0, 1.0}, 0.6);

            if(ECS::registry<Player>.has(entity)) {
                // change chromatic aberration
                base_chrab = (targetChar.maxHealth-targetChar.currentHealth)/targetChar.maxHealth * 0.3;

                auto& screen = ECS::registry<ScreenState>.components[0];
                screen.chrom_abb = base_chrab;
            }

            if (!ECS::registry<HurtQueue>.has(entity)) {
                ECS::registry<HurtQueue>.emplace(entity);

                auto &hq = ECS::registry<HurtQueue>.get(entity);
                Hurt hurt_entity = Hurt();
                hurt_entity.damage = damage;
                hurt_entity.finalHealth = targetChar.currentHealth - damage;
                if (hurt_entity.finalHealth <= 0) {
                    hurt_entity.death = true;

                    // Generate Gold Drop
                    int gold_value = rand() % 50 + 5;
                    Money::createMoney(targetChar.name + "-money", entity_pos.grid_pos, gold_value);
                }
                hq.q.push_back(hurt_entity);
            }  else {
                auto& hq = ECS::registry<HurtQueue>.get(entity);
                if (!hq.q.empty()) {
                    auto& h = hq.q.front();
                    for (auto& hurt : hq.q) {
                        hurt.counterMs = min(hurt.counterMs, h.initCounterMs/(hq.q.size() + 1));
                    }
                    Hurt hurt_entity = Hurt();
                    hurt_entity.damage = damage;
                    hurt_entity.finalHealth = hq.q.back().finalHealth - damage;
                    if (hurt_entity.finalHealth <= 0) {
                        hurt_entity.death = true;
                    }
                    hq.q.push_back(hurt_entity);
                } else {
                    Hurt hurt_entity = Hurt();
                    hurt_entity.damage = damage;
                    hurt_entity.finalHealth = targetChar.currentHealth - damage;
                    if (hurt_entity.finalHealth <= 0) {
                        hurt_entity.death = true;

                        // Generate Gold Drop
                        int gold_value = rand() % 50 + 5;
                        Money::createMoney(targetChar.name + "-money", entity_pos.grid_pos, gold_value);
                    }
                    hq.q.push_back(hurt_entity);
                }
            }
            if (melee) {
                current_char_turn.attacks--;
                current_char_turn.currentAction = current_char_turn.IDLE;
            }
            sound.play(attacker, HIT);
            sound.play(entity, HURT);
            return true;
        }
    }

    if (melee)
            current_char_turn.currentAction = current_char_turn.IDLE;
    return false;

}

void WorldSystem::on_mouse_move(vec2 mouse_pos, bool setY, int y) {
//	if (!ECS::registry<DeathTimer>.has(character))
//	{
    // TODO: add menu checking

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    window_size.x = w; window_size.y = h;
    Tooltip *tt = &ECS::registry<Tooltip>.components.back();
    bool show = false;
    for (auto& e : ECS::registry<Button>.entities) {
        Button& button = ECS::registry<Button>.get(e);
        if (ECS::registry<Tooltip>.has(e)) {
            if (mouse_pos.x > button.centre.x - button.width / 2 &&
                mouse_pos.x < button.centre.x + button.width / 2
                && mouse_pos.y > button.centre.y - button.height / 2 &&
                mouse_pos.y < button.centre.y + button.height / 2) {
                if (!button.hover) {
                    // mouse over
                    show = true;
                    tt = &ECS::registry<Tooltip>.get(e);
                    button.on_hover();
                } else {
                    if (setY)
                        UISystem::moveTooltip({mouse_pos.x,  y});
                    else
                        UISystem::moveTooltip({mouse_pos.x,  mouse_pos.y});
                }
            } else {
                if (button.hover) {
                    // mouse off
                    UISystem::clearTooltip();
                    button.off_hover();
                }
            }
        }
    }
    if (show) {
        if (setY)
            UISystem::createTooltip(*tt, {mouse_pos.x, y});
        else
            UISystem::createTooltip(*tt, {mouse_pos.x, mouse_pos.y});
    }
//	}
}

// camtest
void WorldSystem::set_mouse_highlight_pos(vec2 camPos) {
    vec2 new_pos = mouse_position + camPos;
    new_pos /= ECS::registry<TilePosition>.components[0].scale;

    new_pos.x = floor(new_pos.x + 0.5);
    new_pos.y = floor(new_pos.y + 0.5);

    highlighted_tile = new_pos;

    updateCursorPosition(highlighted_tile);
}
void print_pos(std::string str, vec2 pos) {
    std::cout << str << ": " << pos.x << "," << pos.y << std::endl;
}
void WorldSystem::on_mouse_button(int button, int action, int mods) {
    auto& registry = ECS::registry<Button>;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (!in_game) return;
        auto& turn = ECS::registry<Turn>.get(current_character);
        TilePosition& tile_position = ECS::registry<TilePosition>.get(current_character);
        if(!ECS::registry<Player>.has(current_character)) return;
        auto& player = ECS::registry<Player>.get(current_character);
        // TODO: fix ratchet check for out of bounds
        if (highlighted_tile.x >= world_grid.size() || highlighted_tile.x < 0 ||
            highlighted_tile.y >= world_grid[0].size() || highlighted_tile.y < 0) {
            return;
        }
        if (can_act & (turn.currentAction == turn.IDLE))
            player.on_click_tile(highlighted_tile);

        UISystem::updateButtons();
    }

    // camtest testcam
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // camera_origin = mouse_position;
        isDraggingCamera = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        /*vec2 displacement = mouse_position - camera_origin;
        print_pos("camera_origin", camera_origin);
        print_pos("mouse_position", mouse_position);
        print_pos("displacement", displacement);*/
        // RenderSystem::cam_position = mouse_position;
        isDraggingCamera = false;
    }
    
}
// On key callback
// Pause: added Pausing
// TODO A1: check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
    // Pause: Pause Key = P
    const int pause_key = GLFW_KEY_ESCAPE;

    // Pause: flip isGamePaused
    if (action == GLFW_RELEASE && key == pause_key)
    {
        WorldSystem::isGamePaused = !WorldSystem::isGamePaused;
        showPause(WorldSystem::isGamePaused);
    }

    //  Pause: do nothing if game is paused and key isn't Pause Key
    if (WorldSystem::isGamePaused && key != pause_key) {
        return;
    }

    // If current character is player
    if (ECS::registry<Player>.has(current_character) && ECS::registry<Turn>.has(current_character))
    {
        auto& turn = ECS::registry<Turn>.get(current_character);
        TilePosition& tile_position = ECS::registry<TilePosition>.get(current_character);
        bool can_move = turn.movement > 0;
        // TODO: clean this up, perhaps store the index of the selected button globally
        if ((action == GLFW_PRESS) && (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT || key == GLFW_KEY_ENTER ||
            key == GLFW_KEY_A || key == GLFW_KEY_D)) {
            ECS::registry<Button>.components[selected_button].off_select();
            int size = ECS::registry<Button>.components.size() - 1; // don't include end turn button, so -1
            if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
                if (selected_button + 1 >= size) selected_button = 0;
                else selected_button += 1;
            }
            if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
                if (selected_button - 1 < 0) selected_button = size - 1;
                else selected_button -= 1;
            }
            auto& target_button = ECS::registry<Button>.components[selected_button];
            target_button.notify(!target_button.clicked, target_button.centre, [this]() { this->play_sound(); });
            std::cout << target_button.key << " selected!" << std::endl;
            //            target_button.on_select();
        }
    }

    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        restart();
    }

    // camtest
    if (action == GLFW_REPEAT && key == GLFW_KEY_SPACE)
    {
        RenderSystem::cam_position = { 552,560 };
    }
    if (action == GLFW_RELEASE && key == GLFW_KEY_U) {
        save_area("save.json");
    }
    else if (action == GLFW_RELEASE && key == GLFW_KEY_I) {
        level_to_load = "save.json";
        restart();
    }

    // TODO: Testing purposes, remove later
    if (action == GLFW_RELEASE && key == GLFW_KEY_T)
    {
        step_turn();
    }
    // Adds a char to end of turn list
//    if (action == GLFW_RELEASE && key == GLFW_KEY_A && (mod & GLFW_MOD_SHIFT))
//    {
//		ECS::Entity new_char = Character::createCharacter(std::to_string(turn_queue.size()), {1,1});
//        turn_queue.push_back(new_char);
//    }

    // Debugging
    if (key == GLFW_KEY_D && (mod & GLFW_MOD_SHIFT))
        DebugSystem::in_debug_mode = (action != GLFW_RELEASE);

    if (key == GLFW_KEY_S && (mod & GLFW_MOD_SHIFT))
        in_game = true;

    if (key == GLFW_KEY_B && (action == GLFW_PRESS)) {
        if (UISystem::mode == UISystem::NONE)
            UISystem::showUI(UISystem::BATTLE);
        else UISystem::showUI(UISystem::NONE);
    }
}

// TODO: make this less ugly
void WorldSystem::play_sound() {
        sound.play("click");
}

void WorldSystem::save_area(const std::string& name) {
	jsonf save;

    // Terrain
    const int width = world_grid.size();
    const int height = world_grid[0].size();

    // static array: terrain
    // dynamic array: terrainOther
    std::vector<std::vector<int>> terrainVector(height, std::vector<int>(width));
    std::vector<std::vector<int>> tileVector(height, std::vector<int>(width));

	for (int y = 0; y < height; y++) {
		//std::array<int, 19> row;
        //int* rowOther = new int [width];
		for (int x = 0; x < width; x++) {
			//row[x] = world_grid[x][y];
            //rowOther[x] = world_grid[x][y];
            terrainVector[y][x] = world_grid[x][y];
            tileVector[y][x] = world_grid_tileset[x][y];
		}
		//terrain[y] = row;
        //terrainOther[y] = rowOther;
	}

	save["area"] = current_area;

    for (int k = 0; k < terrainVector.size(); k++) {
        save["terrain"][k] = terrainVector[k];
        save["tileset"][k] = tileVector[k];
    }

    // Characters
    json skeleton = json::array();
    json slime = json::array();
    json worm = json::array();
    json sahuagin = json::array();
    json pirate = json::array();

    for (auto entity : ECS::registry<Character>.entities) {
        Character character = ECS::registry<Character>.get(entity);
        // Player
        if (character.name == "player") {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            save[character.name]["position"] = position;
//            Deprecated!
//            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };
//            save[character.name]["HP"] = hp_array;
        }
        // Skeleton
        if (character.name.find("skeleton") == 0) {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };

            skeleton.push_back( { { "name", character.name }, { "position", position }, { "HP", hp_array } } );
        }
        // Slime
        if (character.name.find("slime") == 0) {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };

            slime.push_back({ { "name", character.name }, { "position", position }, { "HP", hp_array } });
        }
        // Worm
        if (character.name.find("bookworm") == 0) {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };

            worm.push_back({ { "name", character.name }, { "position", position }, { "HP", hp_array } });
        }
        // Sahuagin
        if (character.name.find("sahuagin") == 0) {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };

            sahuagin.push_back({ { "name", character.name }, { "position", position }, { "HP", hp_array } });
        }
        // Pirate
        if (character.name.find("pirate") == 0) {
            TilePosition character_pos = ECS::registry<TilePosition>.get(entity);
            std::array<int, 2> position = { (int)round(character_pos.grid_pos.x), (int)round(character_pos.grid_pos.y) };
            std::array<int, 2> hp_array = { (int)character.currentHealth, character.maxHealth };

            pirate.push_back({ { "name", character.name }, { "position", position }, { "HP", hp_array } });
        }
    }

    if (!ECS::registry<Inventory>.components.empty())
        save_inventory(ECS::registry<Inventory>.components[0]);

    save["skeletons"] = skeleton;
    save["slimes"] = slime;
    save["bookworms"] = worm;
    save["sahuagins"] = sahuagin;
    save["pirates"] = pirate;

    // Character Attacks and Moves
    auto& turn = ECS::registry<Turn>.get(current_character);
    save["turns"]["moves"] = turn.movement;
    save["turns"]["attacks"] = turn.attacks;

    // Artifact
    for (auto entity : ECS::registry<Artifact>.entities) {
        TilePosition artifact_pos = ECS::registry<TilePosition>.get(entity);
        save["artifact"]["position"] = { artifact_pos.grid_pos.x, artifact_pos.grid_pos.y };
    }


    json torches = json::array();

    for (auto& torch : ECS::registry<Torch>.entities) {
        TilePosition torch_pos = ECS::registry<TilePosition>.get(torch);
        std::array<int, 2> position = { (int)round(torch_pos.grid_pos.x), (int)round(torch_pos.grid_pos.y) };
        torches.push_back({{"position", position}, {"lit", ECS::registry<Torch>.get(torch).lit}});
    }

    json doors = json::array();

    for (auto& door : ECS::registry<Door>.entities) {
        TilePosition door_pos = ECS::registry<TilePosition>.get(door);
        std::array<int, 2> position = { (int)round(door_pos.grid_pos.x), (int)round(door_pos.grid_pos.y) };
        auto& real_door = ECS::registry<Door>.get(door);
        doors.push_back({{"position", position},
                           {"horizontal", real_door.horizontal},
                           {"locked", real_door.locked},
                           {"id", real_door.id},
                           {"door_type", real_door.door_type}});
    }

    json keys = json::array();

    for (auto& key : ECS::registry<Key>.entities) {
        TilePosition key_pos = ECS::registry<TilePosition>.get(key);
        std::array<int, 2> position = { (int)round(key_pos.grid_pos.x), (int)round(key_pos.grid_pos.y) };
        auto& real_key = ECS::registry<Key>.get(key);
        keys.push_back({{"position", position},
                         {"id", real_key.id}});
    }

    json spellpowers = json::array();
    for (auto& sp : ECS::registry<SpellPower>.entities) {
        TilePosition s_pos = ECS::registry<TilePosition>.get(sp);
        std::array<int, 2> position = { (int)round(s_pos.grid_pos.x), (int)round(s_pos.grid_pos.y) };
        auto& power = ECS::registry<SpellPower>.get(sp);
        spellpowers.push_back({{"position", position},
                        {"spell_type", power.spell_effect},
                        {"name", power.name},
                        {"texture_path", power.texture_path}});
    }

    save["torches"] = torches;
    save["doors"] = doors;
    save["keys"] = keys;
    save["spell_powers"] = spellpowers;

	// Save game state into JSON
    std::cout << save_path(name);
    std::ofstream savefile(save_path(name));
    if (savefile.is_open()) {
        savefile << save;
        savefile.close();
    }
    else {
        std::cout << "Can't Open File\n";
    }

    // For Testing Purposes
    std::ifstream open_savefile(save_path(name));
    std::string savefile_content((std::istreambuf_iterator<char>(open_savefile)),
        (std::istreambuf_iterator<char>()));
    std::cout << "Save File Contents:\n" << savefile_content;
    open_savefile.close();
}

void WorldSystem::load_area(const std::string &name) {
    std::ifstream savefile(save_path(name));
    std::cout << save_path(name);
    std::string content((std::istreambuf_iterator<char>(savefile)),
        (std::istreambuf_iterator<char>()));


    // if file opened successfully
    if (savefile.is_open()) {
        auto save_state = json::parse(content);
        // Parse JSON
        for (auto& element : save_state.items()) {
            // Terrain
            if (element.key() == "terrain") {
                int outerSize = element.value().size();
                int innerSize = element.value()[0].size();
                // world_grid.resize(innerSize, std::vector<int>(outerSize, 0));
                world_grid.resize(innerSize);
                for (int i = 0; i < innerSize; ++i) {
                    world_grid[i].resize(outerSize);
                }
                int out = world_grid.size();
                int in = world_grid[0].size();
                for (int y = 0; y < outerSize; y++) {
                    for (int x = 0; x < element.value()[y].size(); x++) {
                        world_grid[x][y] = element.value()[y][x];
                    }
                }
            }
            if (element.key() == "tileset") {
                int outerSize = element.value().size();
                int innerSize = element.value()[0].size();
                world_grid_tileset.resize(innerSize);
                for (int i = 0; i < innerSize; ++i) {
                    world_grid_tileset[i].resize(outerSize);
                }
                int out = world_grid_tileset.size();
                int in = world_grid_tileset[0].size();
                for (int y = 0; y < outerSize; y++) {
                    for (int x = 0; x < element.value()[y].size(); x++) {
                        world_grid_tileset[x][y] = element.value()[y][x];
                    }
                }
            }
            if (element.key().compare("area") == 0) {
                setArea(element.value());
            }
        }
        savefile.close();   //close the file object.
    }
}

void WorldSystem::load_chars(const std::string &name) {
    std::ifstream savefile(save_path(name));
    std::string content((std::istreambuf_iterator<char>(savefile)),
                        (std::istreambuf_iterator<char>()));

    // if file opened successfully
    if (savefile.is_open()) {
        auto save_state = json::parse(content);

        // Parse JSON
        for (auto &element : save_state.items()) {

            // Skeleton
            if (element.key() == "skeletons") {
                for (auto &i : element.value()) {
                    auto skeleton_pos = i.find("position");
                    json array = skeleton_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *skeletonAnim = setAnim("skeleton_idle", "skeleton_idle.png", {16, 16},
                                                 {32, 16}, 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 2, Animated, skeletonAnim, 3, current_health,
                                    max_health, 15);
                    std::cout << "Skeleton spawned\n";
                }
            }

            // Slime
            if (element.key() == "slimes") {
                for (auto &i : element.value()) {
                    auto slime_pos = i.find("position");
                    json array = slime_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto* slimeAnim = setAnim("slime_idle", "slime_idle.png", { 16, 16 },
                        { 80, 64 }, 20, 30, true, false, false, nullptr);
                    if(current_area == 0)
                        spawn_character(i.find("name").value(), position, 1, Animated, slimeAnim, 0, current_health, max_health, 8);
                    else
                        spawn_character(i.find("name").value(), position, 1, Animated, slimeAnim, 1, current_health, max_health, 8);
                    std::cout << "Slime spawned\n";

                }
            }
            // Wraith
            if (element.key() == "wraiths") {
                for (auto &i : element.value()) {
                    auto wraith_pos = i.find("position");
                    json array = wraith_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *wraithAnim = setAnim("wraith_idle", "wraith_idle.png", {16, 16},
                                               {16, 32}, 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 6, Animated, wraithAnim, 6, current_health,
                                    max_health, 25);
                    std::cout << "Wraith spawned\n";
                }
            }

            // Ancient skeleton
            if (element.key() == "ancient_skeletons") {
                for (auto &i : element.value()) {
                    auto sk_pos = i.find("position");
                    json array = sk_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *skAnim = setAnim("ancient_skeleton_idle", "ancient_skeleton_idle.png", {16, 16},
                                           {32, 16}, 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 7, Animated, skAnim, 3, current_health,
                                    max_health, 5);
                    std::cout << "Skeleton spawned\n";
                }
            }

            // Red slime
            if (element.key() == "red_slimes") {
                for (auto &i : element.value()) {
                    auto sk_pos = i.find("position");
                    json array = sk_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *rsAnim = setAnim("red_slime_idle", "redslime_idle.png", {16, 16},
                                           {80, 64}, 20, 30, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 1, Animated, rsAnim, 3, current_health,
                                    max_health, 5);
                    std::cout << "redslime spawned\n";
                }
            }

            // bats
            if (element.key() == "bats") {
                for (auto &i : element.value()) {
                    auto bat_pos = i.find("position");
                    json array = bat_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *batAnim = setAnim("bat_idle", "bat_idle.png", {16, 16},
                                            {32, 16}, 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 6, Animated, batAnim, 7, current_health,
                                    max_health, 8);
                    std::cout << "Bat spawned\n";
                }
            }

            // psycho
            if (element.key() == "psychos") {
                for (auto &i : element.value()) {
                    auto psy_pos = i.find("position");
                    json array = psy_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *psyAnim = setAnim("psycho_idle", "psycho_idle.png", {16, 16},
                                            {32, 16}, 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 8, Animated, psyAnim, 10, current_health,
                                    max_health, 10);
                    std::cout << "Psycho spawned\n";
                }
            }

            // Illiterate Bookworm
            if (element.key() == "bookworms") {
                for (auto &i : element.value()) {
                    auto worm_pos = i.find("position");
                    json array = worm_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *wormAnim = setAnim("bookworm_idle", "bookworm_idle.png", {16, 16}, {16, 32},
                                             2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 3, Animated, wormAnim, 2, current_health,
                                    max_health, 11);
                    std::cout << "Illiterate Bookworm spawned\n";
                }
            }

            // Sahuagin
            if (element.key() == "sahuagins") {
                for (auto &i : element.value()) {
                    auto sahuagin_pos = i.find("position");
                    json array = sahuagin_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *sahuaginAnim = setAnim("sahuagin_idle", "sahuagin_idle.png", {16, 16}, {32, 16},
                                                 2, 2, true, false, false, nullptr);
                    spawn_character(i.find("name").value(), position, 4, Animated, sahuaginAnim, 3, current_health,
                                    max_health, 12);
                    std::cout << "Sahuagin spawned\n";
                }
            }



            // Pirate
            if (element.key() == "pirates") {
                for (auto &i : element.value()) {
                    auto pirate_pos = i.find("position");
                    json array = pirate_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto char_hp = i.find("HP");
                    json hp_values = char_hp.value();
                    int max_health = hp_values[1];
                    int current_health = hp_values[0];
                    auto *pirateAnim = setAnim("pirate_idle", "pirate_idle.png", {16, 16}, {480, 16},
                                               30, 5, true, false, false, nullptr);
                    // TODO: make pirate use Spells
                    spawn_character(i.find("name").value(), position, 5, Animated, pirateAnim, 2, current_health,
                                    max_health, 15);
                    std::cout << "Pirate spawned\n";
                }
            }

            // artifact
            if (element.key() == "artifact") {
                auto artifact_pos = element.value().find("position");
                json array = artifact_pos.value();
                vec2 position = {array[0], array[1]};
                if (current_area == 1)
                    Artifact::createArtifact(position, 1, "typewriter.png");
                else if (current_area == 2)
                    Artifact::createArtifact(position, 2, "teapot.png");
                else if(current_area == 0)
                    Artifact::createArtifact(position, 3, "strange_triangle.png");
                else if (current_area == 3)
                    Artifact::createArtifact(position, 4, "strange_device.png");
            }

            // power
            if (element.key() == "power") {
                auto power_pos = element.value().find("position");
                json array = power_pos.value();
                vec2 position = {array[0], array[1]};
                auto point_ = element.value().find("point");
                int point = point_.value();
                auto t_path = element.value().find("texture_path");
                std::string path = t_path.value();
                auto name_ = element.value().find("name");
                std::string w_name = name_.value();
                Power::createPower(path, w_name, point, position);
            }

            // Torch
            if (element.key() == "torches") {
                for (auto &i : element.value()) {
                    auto torch_pos = i.find("position");
                    json array = torch_pos.value();
                    vec2 position = {array[0], array[1]};
                    auto isLit = i.find("lit");
                    bool lit = isLit.value();
                    Torch::createTorch(position, lit);
                }
            }

            // scramble turn queue before load player

//            std::vector<ECS::Entity> temp{std::begin(turn_queue), std::end(turn_queue)};
//            std::shuffle(std::begin(temp), std::end(temp), rng);
//
//            turn_queue.clear();
//            for (auto &e : temp) {
//                turn_queue.push_back(e);
//            }

            // Player
            if (element.key() == "player") {
                auto player_pos = element.value().find("position");
                json array = player_pos.value();
                vec2 position = {array[0], array[1]};
                auto char_hp = element.value().find("HP");
                json hp_values = char_hp.value();
                int max_health = 150;//hp_values[1];
                int current_health = 150;//hp_values[0];
                auto *playerAnim = setAnim("player_idle", "player_idle.png",
                                           {16, 20}, {608, 20}, 38, 5, true, false, false, nullptr);
                current_character = Character::createAnimatedCharacter("player", position, *playerAnim, 5, max_health,
                                                                       10);
                ECS::registry<Player>.insert(current_character, Player(current_character, this));

                auto &ls = ECS::registry<LightSource>.emplace(current_character);
                ls.strength = 0.9;
                ECS::registry<Player>.get(current_character).hasArtifact = false;
                turn_queue.push_front(current_character);
                UISystem::current_player = std::make_shared<ECS::Entity>(current_character);
                Health::createHealthBar(current_character, 1.0, position);
                load_inventory(current_character);

                auto &inv = ECS::registry<Inventory>.get(current_character);
                if (!inv.weapons.empty()) {
                    ECS::registry<Weapon>.emplace(current_character, inv.weapons.back());
                    auto &dmg = ECS::registry<Character>.get(current_character).meleeDamage;
                    dmg = inv.weapons.back().damage;
                }
                auto& c = ECS::registry<Character>.get(current_character);
                c.maxHealth = inv.hp;
                c.currentHealth = inv.hp;
                c.max_movement = inv.mp;
                c.max_attacks = inv.ap;
                c.max_casts = inv.sp;
                ECS::registry<Turn>.emplace(current_character, ECS::registry<Character>.get(current_character));

                std::cout << "Player spawned\n";
            }

            // Doors
            if (element.key() == "doors") {
                for (auto &i : element.value()) {
                    auto door_pos = i.find("position");
                    json array = door_pos.value();
                    vec2 position = {array[0], array[1]};
                    int id = i.find("id").value();
                    int door_type = i.find("door_type").value();
                    bool horizontal = i.find("horizontal").value();
                    bool locked = i.find("locked").value();
                    Door::createDoor(position, id, door_type, horizontal, locked);
                }
            }

            // Keys
            if (element.key() == "keys") {
                for (auto &i : element.value()) {
                    auto key_pos = i.find("position");
                    json array = key_pos.value();
                    vec2 position = {array[0], array[1]};
                    int id = i.find("id").value();
                    Key::createKey(position, id);
                }
            }

            // SpellPowers
            if (element.key() == "spell_powers") {
                for (auto &i : element.value()) {
                    auto spell_pos = i.find("spell_pos");
                    json array = spell_pos.value();
                    vec2 position = {array[0], array[1]};
                    std::string path = i.find("texture_path").value();
                    std::string name = i.find("name").value();
                    Spell::SpellEffect type = i.find("spell_type").value();
                    auto spellPower = SpellPower::createSpellPower(path, name, type, position);
                }
            }

            if (element.key() == "turns") {
                auto moves = element.value().find("moves");
                int num_moves = moves.value();
                auto attacks = element.value().find("attacks");
                int num_attacks = attacks.value();
                auto &turn = ECS::registry<Turn>.get(current_character);
//                TODO: does this still need to be serialized
//                turn.movement = num_moves;
//                turn.attacks = num_attacks;
            }
        }

        savefile.close();   //close the file object.
    }
}

bool WorldSystem::is_wall(vec2 location) {
    if(location.x < 1 || location.x >= world_grid.size()) return true;
    if(location.y < 1 || location.y >= world_grid[0].size()) return true;
    return world_grid[location.x][location.y] == Tile::WALL;
}

bool WorldSystem::is_goal(vec2 location) {
    return world_grid[location.x][location.y] == Tile::GOAL;
}

bool WorldSystem::is_hole(vec2 location) {
    return world_grid[location.x][location.y] == Tile::HOLE;
}

bool WorldSystem::on_death(ECS::Entity entity) {
    auto& screen = ECS::registry<ScreenState>.components[0];
    screen.chrom_abb = base_chrab;
    if (ECS::registry<Player>.has(entity)) {
        auto& inv = ECS::registry<Inventory>.get(entity);
        for (int i = 0; i < inv.artifacts.size(); i++) {
            if (inv.artifacts[i].area == current_area) {
                inv.artifacts[i] = inv.artifacts.back();
                inv.artifacts.resize(inv.artifacts.size() - 1);
                break;
            }
        }
        save_inventory(inv);
        endLevel();
        return true;
    }
    if (ECS::registry<Shopkeeper>.has(entity)) {
        // TODO: change this to 3rd level artifact
        Artifact::createArtifact(
                ECS::registry<TilePosition>.get(entity).grid_pos, 3, "teapot.png");
    }
    auto& character = ECS::registry<Character>.get(entity);
    std::cout << character.name << " is defeated!" << std::endl;
    sound.play(entity, DEATH);
    if (current_character.id == entity.id)
        step_turn();
    turn_queue.remove(entity);
    ECS::ContainerInterface::remove_all_components_of(entity);
    UISystem::updateTurn();
    return false;
}

// Use this function to test spell properties
void WorldSystem::createSpells() {
    spells_cache = std::vector<Spell>();
    /*Spell push = Spell(Spell::SpellEffect::PUSH, 3, 1, true);
    Spell swap = Spell(Spell::SpellEffect::SWITCH, 0, 5, true);
    spells_cache.push_back(push);
    spells_cache.push_back(swap);*/
    // NOTE: the following is all testing code and doesn't actually add anything to
    // the spells cache. Remove and/or modify as needed
    {
        std::cout << "////////////////\nCreating spells\n////////////////\n";

        Spell spell = Spell(Spell::SpellEffect::DAMAGE);
        spell.setTargetableRange(0, 2, false);
        std::vector<vec2> targetable = std::vector<vec2>();
        targetable.emplace_back(vec2(0, 1));
        targetable.emplace_back(vec2(0, 2));
        targetable.emplace_back(vec2(0, 3));
        targetable.emplace_back(vec2(0, 4));
        targetable.emplace_back(vec2(1, 4));
        spell.setCellsTargetable(targetable);
        std::cout << "targetable cells:\n";
        for (vec2 c : spell.targetable_cells)
            std::cout << "(" << c.x << ", " << c.y << "),";

        std::cout << "targetable cells:\n";
        spell.setTargetableRange(0, 2, false);
        std::vector<vec2> untargetable = std::vector<vec2>();
        untargetable.emplace_back(vec2(0, -1));
        untargetable.emplace_back(vec2(0, -2));
        untargetable.emplace_back(vec2(0, -3));
        untargetable.emplace_back(vec2(1, -1));

        spell.setCellsUntargetable(targetable);
        for (vec2 c : spell.targetable_cells)
            std::cout << "(" << c.x << ", " << c.y << "),";
    } // End testing code

}

void WorldSystem::updateSpellButton(WorldSystem::SpellButton button, Spell::SpellEffect spell_effect) {
    std::string path = "";
    std::string click_path = "";
    std::string tooltip_message = "";
    if (spell_effect == Spell::SpellEffect::SWITCH) {
        path = "ui/spell_icons/regular/switch_icon.png";
        click_path = "ui/spell_icons/selected/switch_icon.png";
        tooltip_message = "SWAP location with an enemy\nCost: |";
    } else if (spell_effect == Spell::SpellEffect::GRAPPLE_TO) {
        path = "ui/spell_icons/regular/hook_icon.png";
        click_path = "ui/spell_icons/selected/hook_icon.png";
        tooltip_message = "HOOK to a location at, or adjacent to,\n a wall  or enemy \nCost: ||";
    } else if (spell_effect == Spell::SpellEffect::PUSH) {
        path = "ui/spell_icons/regular/push_icon.png";
        click_path = "ui/spell_icons/selected/push_icon.png";
        tooltip_message = "PUSH a nearby enemy\nCost: |";
    } else {
        // TODO: heal spell textures
        path = "ui/spell_icons/regular/heal_icon.png";
        click_path = "ui/spell_icons/regular/heal_icon.png";
        tooltip_message = "HEAL yourself\nCost: |";
    }
    // TODO: more spells later

    int buttony = 750;
    int buttonx = 500;
    int buttonSpacing = 80;
    for (auto e : ECS::registry<BattleUI>.entities) {
        auto& bui = ECS::registry<BattleUI>.get(e);
        bool isThis = false;
        switch (button) {
            case WorldSystem::SpellButton::ONE:
                isThis = (bui.type == BattleUI::SPELL1);
                break;
            case WorldSystem::SpellButton::TWO:
                isThis = (bui.type == BattleUI::SPELL2);
                break;
            case WorldSystem::SpellButton::THREE:
                isThis = (bui.type == BattleUI::SPELL3);
                break;
            default:
                break;
        }
        if (isThis) {
            ECS::ContainerInterface::remove_all_components_of(e);
            break;
        }
    }

    int size = ECS::registry<JODBUTTON>.size()-1;
    if (button == WorldSystem::SpellButton::ONE) {
        auto new_button = Button::createButton({ buttonx + buttonSpacing*3, buttony }, 64, 64,
                                               [this]() {this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::ONE);},
                                               path, click_path, click_path, true);
        ECS::registry<Tooltip>.insert(new_button, Tooltip({ 850, buttony }, tooltip_message));
        ECS::registry<BattleUI>.emplace(new_button, BattleUI::SPELL1);
        ECS::registry<JODBUTTON>.emplace(new_button);
        buttons[size] = new_button;
    } else if (button == WorldSystem::SpellButton::TWO) {
        auto new_button = Button::createButton({ buttonx + buttonSpacing*4, buttony }, 64, 64,
                                      [this]() {this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::TWO); },
                                               path, click_path, click_path, true);
        ECS::registry<Tooltip>.insert(new_button, Tooltip({ 850, 40 }, tooltip_message));
        ECS::registry<BattleUI>.emplace(new_button, BattleUI::SPELL2);
        ECS::registry<JODBUTTON>.emplace(new_button);
        buttons[size] = new_button;
    } else { // WorldSystem::SpellButton::THREE
        auto new_button = Button::createButton({ buttonx + buttonSpacing*5, buttony }, 64, 64,
                                      [this]() {this->on_click_toggle_spell_with_target(WorldSystem::SpellButton::THREE); },
                                      path, click_path, click_path, true);
        ECS::registry<Tooltip>.insert(new_button, Tooltip({ 850, 40 }, tooltip_message));
        ECS::registry<BattleUI>.emplace(new_button, BattleUI::SPELL3);
        ECS::registry<JODBUTTON>.emplace(new_button);
        buttons[size] = new_button;
    }
}

// this checks for four positions so it doesn't get diagonally blocked
bool WorldSystem::hasLoS(vec2 begin, vec2 end) {
    float ep = 0.001;
    vec2 o1 = vec2(end.x + ep, end.y + ep);
    vec2 o2 = vec2(end.x + ep, end.y - ep);
    vec2 o3 = vec2(end.x - ep, end.y + ep);
    vec2 o4 = vec2(end.x - ep, end.y - ep);
    return singleHasLoS(begin, o1) ||
        singleHasLoS(begin, o2) ||
        singleHasLoS(begin, o2) ||
        singleHasLoS(begin, o4);
}

bool WorldSystem::singleHasLoS(vec2 begin, vec2 end) {
    float x1 = begin.x;
    float y1 = begin.y;
    float x2 = end.x - begin.x;
    float y2 = end.y - begin.y;
    float len = length(end-begin);
    float epsilon = 0.0000001;

    for(auto& edge : ECS::registry<Edge>.components) {
        vec2 es = edge.start;
        vec2 ee = edge.end;
        float x3 = es.x;
        float y3 = es.y;
        float x4 = ee.x-es.x;
        float y4 = ee.y-es.y;

        float T2 = (x2*(y3-y1) + y2*(x1-x3))/(x4*y2 - y4*x2);
        float T1 = (x3+x4*T2-x1)/x2;
        if(T1 <= 0-epsilon || T1 >= 1+epsilon) continue;
        if(T2 < 0-epsilon || T2 > 1+epsilon) continue;

        // if no conditions failed, then there was an intersection
//        std::cout << "len " << len << " t1 " << T1 << "\n";
//        std::cout << "edge " << es.x << ", " << es.y << "; " << ee.x << ", " << ee.y << "\n";
//        std::cout << "points " << begin.x << ", " << begin.y << "; " << end.x << ", " << end.y << "\n";
        return false;
    }
    // we have los
    return true;
}

void WorldSystem::setArea(int areaNum) { current_area = areaNum; }
int WorldSystem::getArea() { return current_area; }

void WorldSystem::load_interactibles() {
    for (auto e : ECS::registry<Torch>.entities) {
        auto& torch = ECS::registry<Torch>.get(e);
        if (!torch.lit) {
            auto& i = ECS::registry<Interactible>.insert(e, Interactible("Light torch"));
            i.onInteract = [i,e](ECS::Entity* entity) {
                auto& t = ECS::registry<Torch>.get(e);
                t.setLight(true);
                std::cout<<"torch interacted with" << std::endl;
                auto& in = ECS::registry<Interactible>.get(e);
                in.destroy();
                ECS::registry<Interactible>.remove(e);
            };
            std::cout << "added torch interactibility" << std::endl;
        }
    }
    for (auto d : ECS::registry<Door>.entities) {
        auto& door = ECS::registry<Door>.get(d);
        auto& i = ECS::registry<Interactible>.insert(d, Interactible("Interact door"));
        i.onInteract = [i,d](ECS::Entity* entity) {
            auto& real_door = ECS::registry<Door>.get(d);
            if (ECS::registry<Inventory>.has(*entity)) {
                auto& keys = ECS::registry<Inventory>.get(*entity).keys;
                for (auto it = keys.begin(); it != keys.end(); it++) {
                    if (it->id == real_door.id) {
                        keys.erase(it);
                        real_door.unlock();
                        ECS::registry<TempText>.emplace(UISystem::showText("Door unlocked!", {300, 400}, 0.5f), 3000.f);
                        break;
                    }
                }
            }
            real_door.toggle(!real_door.opened);
        };
        std::cout << "added door interactibility" << std::endl;
    }
}
