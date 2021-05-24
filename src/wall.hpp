//
// Created by Alan Zhang on 2021-02-10.
//

#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Wall
{
    vec2 grid_position{};
    // Creates all the associated render resources and default transform
    static ECS::Entity createWall(vec2 grid_position);

    Wall(vec2 grid_position);
};
