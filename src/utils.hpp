//
// Created by Roark on 2021-03-12.
//
#pragma once
//#ifndef FIVE_WONDERS_UTILS_HPP
//#define FIVE_WONDERS_UTILS_HPP

#include "world.hpp"
#include "debug.hpp"
#include <iostream>

struct Cell_Edges {
    bool has_edge[4]; // NESW clockwise
    int edge_ids[4];
};

struct Edge {
    vec2 start{};
    vec2 end{};
    float x3, y3, x4, y4;
    bool is_3d;
    Edge(vec2 s, vec2 e, bool is3d = false) {
        start = s;
        end = e;
        is_3d = is3d;
    }
};

// store vertices once so we don't have to recompute
struct V {
    V() = default;
    vec2 pos{};
    explicit V(vec2 p) {
        pos = p;
    }
};

//#endif //FIVE_WONDERS_UTILS_HPP
