//
// Created by Roark on 2021-02-05.
//

#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Background
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createBackground(std::string path, vec2 scale);

    vec2 dims;

    vec2 base_position;

    float parallax;

    static void changeBackground(std::string new_path, vec2 scale);
};

#endif //BACKGROUND_HPP
