#pragma once

// internal
#include "common.hpp"
#include "turn.hpp"
#include "character.hpp"

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

enum SoundType { DEATH, HIT, HURT, WALK, BGM, CLICK, REWARD, PUSH, HOOK, SWITCH, HEAL, };
enum CharType { PLAYER, MONSTER, SYSTEM, POWER, SPELL};
enum Music {MENU, SHOP, AREA1, AREA2, AREA3, AREA4};
class SoundSystem
{
public:
	// Creates a window
	SoundSystem();

	// Releases all associated resources
	~SoundSystem();

	static void init_audio();
	static void play(ECS::Entity entity, SoundType key);
	static void play(CharType c, SoundType key);
	static void play(const std::string& key);


	static void stopMusic();

private:
	static std::vector<Mix_Music*> background_music;
	static Mix_Chunk* button_click_sound;
	static Mix_Chunk* character_move_sound;
	// std::vector<std::vector<Mix_Chunk*>> soundMatrix;
	static std::vector<std::vector<Mix_Chunk*>> soundMatrix;

};
