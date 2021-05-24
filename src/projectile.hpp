#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"


struct Projectile
{
    vec2 origin_pos;
    vec2 goal_pos;
    float dist_traveled;
    float max_dist;

    float speed;

    ECS::Entity thrower_entity;
    int max_pierce;
    float pierce_interval;
    float pierce_counter;
    float rotation = 0;

    static ECS::Entity createProjectile(const std::string& projectile_type,
                                                    vec2 origin, vec2 goal_pos_p, float speed, int pierce, float pierce_frequency,
                                                    // This is the attack function in world, attack_target
                                                    ECS::Entity thrower, float rotation);
};
