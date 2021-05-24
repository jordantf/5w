// internal
#include "turn.hpp"
#include "tiny_ecs.hpp"


Turn::Turn(Character character) {
    attacks = character.max_attacks;
    movement = character.max_movement;
    spells = character.max_casts;
}
