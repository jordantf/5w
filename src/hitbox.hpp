#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"


struct Hitbox
{
    std::vector<ColoredVertex> vertices;
    std::vector<uint16_t> vertex_indices;
};


