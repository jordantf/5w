#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "animation.hpp"
#include "spell.hpp"


struct SpellPower
{
    std::string name;
    std::string texture_path;
    Spell::SpellEffect spell_effect;

    static ECS::Entity createSpellPower(std::string texture_path, std::string name, Spell::SpellEffect spell_type, vec2 grid_position);
};
