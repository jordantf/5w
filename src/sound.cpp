// Header
#include "sound.hpp"
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "wall.hpp"
#include "worldEditSystem.hpp"
#include "render_components.hpp"
#include "turn.hpp"
#include "background.hpp"
#include "player.hpp"
#include "button.hpp"
#include "string"
#include "Enemy1AI.hpp"
#include "Enemy2AI.hpp"
#include "Enemy3AI.hpp"
#include "projectile.hpp"
#include "power.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>

//enum SoundType { DEATH, HIT, HURT, WALK, BGM, CLICK, REWARD, PUSH, HOOK, SWITCH, HEAL, };
//enum CharType { PLAYER, MONSTER, SYSTEM, POWER, SPELL };
//enum Music { MENU, AREA1 };
std::vector<Mix_Music*> SoundSystem::background_music;
Mix_Chunk* SoundSystem::button_click_sound;
Mix_Chunk* SoundSystem::character_move_sound;
// std::vector<std::vector<Mix_Chunk*>> soundMatrix;
std::vector<std::vector<Mix_Chunk*>> SoundSystem::soundMatrix;
SoundSystem::SoundSystem() {
	/*num.resize(3, std::vector<int>(6, 1));*/
	background_music.resize(6, nullptr);
	soundMatrix.resize(5, std::vector<Mix_Chunk*>(11, nullptr));
}
SoundSystem::~SoundSystem() {
    for (Mix_Music* music : background_music) {
        Mix_FreeMusic(music);
    }
	Mix_CloseAudio();
}

void SoundSystem::init_audio() {
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");
	// music references

    background_music = std::vector<Mix_Music*>(6, nullptr);
    soundMatrix.resize(5, std::vector<Mix_Chunk*>(11, nullptr));

    Mix_VolumeMusic(30); // Music volume
    Mix_Volume(-1,30); // SFX volume

    background_music[MENU] = Mix_LoadMUS(audio_path("menu.wav").c_str());
    background_music[SHOP] = Mix_LoadMUS(audio_path("shop_theme.wav").c_str());
	background_music[AREA1] = Mix_LoadMUS(audio_path("area_1_theme.wav").c_str());
    background_music[AREA2] = Mix_LoadMUS(audio_path("area_2_theme.wav").c_str());
    background_music[AREA3] = Mix_LoadMUS(audio_path("area_3_theme.wav").c_str());
    background_music[AREA4] = Mix_LoadMUS(audio_path("area_4_theme.wav").c_str());
	button_click_sound = Mix_LoadWAV(audio_path("button.wav").c_str());
	// character_move_sound = Mix_LoadWAV(audio_path("combat_sfx/whiff.wav").c_str());
	soundMatrix[PLAYER][WALK] = Mix_LoadWAV(audio_path("combat_sfx/walk_player.wav").c_str());
	soundMatrix[MONSTER][WALK] = Mix_LoadWAV(audio_path("combat_sfx/walk_mons.wav").c_str());
	soundMatrix[PLAYER][DEATH] = Mix_LoadWAV(audio_path("combat_sfx/death_player.wav").c_str());
	soundMatrix[MONSTER][DEATH] = Mix_LoadWAV(audio_path("combat_sfx/big_monster_death.wav").c_str());
	soundMatrix[PLAYER][HURT] = Mix_LoadWAV(audio_path("combat_sfx/weak_hit.wav").c_str());
	soundMatrix[MONSTER][HURT] = Mix_LoadWAV(audio_path("combat_sfx/strong_hit.wav").c_str());
	soundMatrix[PLAYER][HIT] = Mix_LoadWAV(audio_path("combat_sfx/hit_player.wav").c_str());
	soundMatrix[MONSTER][HIT] = Mix_LoadWAV(audio_path("combat_sfx/hit_player.wav").c_str());
	soundMatrix[POWER][REWARD] = Mix_LoadWAV(audio_path("combat_sfx/reward.wav").c_str());

	//spells
	soundMatrix[SPELL][PUSH] = Mix_LoadWAV(audio_path("combat_sfx/push.wav").c_str());
	soundMatrix[SPELL][SWITCH] = Mix_LoadWAV(audio_path("combat_sfx/switch.wav").c_str());
	soundMatrix[SPELL][HOOK] = Mix_LoadWAV(audio_path("combat_sfx/hook.wav").c_str());

}
//void play(ECS::Entity entity, SoundType key);
//void play(CharType c, SoundType key);
//void play(SoundType key);
//void play(const std::string& key);
void SoundSystem::play(CharType c, SoundType key) {
	Mix_PlayChannel(-1, soundMatrix[c][key], 0);
}
void SoundSystem::play(ECS::Entity entity, SoundType key) {
	CharType character;
	if (ECS::registry<Player>.has(entity)) {
		character = PLAYER;
	}
	else if(ECS::registry<Power>.has(entity)) {
		character = POWER;
	}
	else {
		character = MONSTER;
	}
	Mix_PlayChannel(-1, soundMatrix[character][key], 0);

}

void SoundSystem::play(const std::string& key) {
    if (key == "bgm_menu") {
        //Mix_FadeOutMusic(500);
        Mix_HaltMusic();
        Mix_PlayMusic(background_music[MENU], -1);
    }
	else if (key == "bgm_area1") {
        Mix_HaltMusic();
		Mix_PlayMusic(background_music[AREA1], -1);
	}
    else if (key == "bgm_area2") {
        Mix_HaltMusic();
        Mix_PlayMusic(background_music[AREA2], -1);
    }
	else if (key == "bgm_area3") {
        Mix_HaltMusic();
        Mix_PlayMusic(background_music[AREA3], -1);
    }
	else if (key == "bgm_shop") {
        Mix_HaltMusic();
        Mix_PlayMusic(background_music[SHOP], -1);
    }
    else if (key == "bgm_area4") {
        Mix_HaltMusic();
        Mix_PlayMusic(background_music[AREA4], -1);
    }
	else if (key == "click") {
		Mix_PlayChannel(-1, button_click_sound, 0);
	}
	else if (key == "move") {
		//int x = num[0][0];
		//std::cout << "MoSADADSA (" << num[0][0] << "," << x << ")" << std::endl;
		Mix_PlayChannel(-1, soundMatrix[PLAYER][WALK], 0);
	}
	else if (key == "reward") {
		//int x = num[0][0];
		//std::cout << "MoSADADSA (" << num[0][0] << "," << x << ")" << std::endl;
		Mix_PlayChannel(-1, soundMatrix[POWER][REWARD], 0);
	}
	// spells
	else if (key == "spell_hook") {
		Mix_PlayChannel(-1, soundMatrix[SPELL][HOOK], 0);
	}
    else if (key == "spell_switch") {
        Mix_PlayChannel(-1, soundMatrix[SPELL][SWITCH], 0);
    }
    else if (key == "spell_push") {
        Mix_PlayChannel(-1, soundMatrix[SPELL][PUSH], 0);
    }
}

void SoundSystem::stopMusic() {
    Mix_HaltMusic();
}
