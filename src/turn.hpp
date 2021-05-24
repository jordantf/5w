#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "character.hpp"


struct Turn
{
    // TODO: To contain Turn-usable resources, such as movement (replenishes every turn)
    int movement = 200; // 3 cells for now
    int attacks = 3; // 1 attack per turn for now
    int spells = 1; // 1 spells
    enum action {IDLE, MOVING, ATTACKING} currentAction = IDLE;
    Turn (Character character);
};

struct Moving
{
    vec2 oldPos;
    vec2 step;
};