#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "animation.hpp"


struct Power
{
    int point = 5;
    std::string name;

    static ECS::Entity createPower(std::string texture_path, std::string name, int point, vec2 grid_position);
    static ECS::Entity createAnimatedPower(int point, vec2 grid_position, Animation starting_anim);
};
