#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "projectile.hpp"
#include "turn.hpp"


struct Boomerang
{
    ECS::Entity thrower;
    vec2 origin;
    float spin = 0;
    float angle = 0;
    float speed;
    float decayTime;
    std::vector<vec2> entered;
    std::string boomerangType;

    static ECS::Entity createBoomerang(const std::string &boomerangType, float spin, float decayTime,
                                       vec2 origin, vec2 goal_pos_p, float speed, ECS::Entity thrower);
};

