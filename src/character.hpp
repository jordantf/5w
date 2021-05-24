#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "animation.hpp"
#include "spell.hpp"

enum spriteType { Textured, Animated };

struct Character
{
    std::string name;
    int maxHealth = 20;
    float currentHealth = maxHealth;
    int meleeDamage = 5;
    int rangedDamage = 7;
    int max_movement = 3; //default values for movement per turn
    int max_attacks = 1; // default number of attacks per turn
    int max_casts = 3; // max number of spells you can cast in a turn
    int spell_casts = 1; // current number of spells still can cast
    enum ANIMSTATE{IDLE, RUN, ATTACK, PUSHED, HURT, NONE} anim_state = NONE;

    static ECS::Entity createCharacter(const std::string& character_name, vec2 grid_position, int movement_points);
    static ECS::Entity createAnimatedCharacter(const std::string& character_name, vec2 grid_position,
                                               Animation starting_anim, int movement_points, int HP, int attack);
    bool castSpell(ECS::Entity self, Spell spell, vec2 target_pos);
};
