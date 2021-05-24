//
// Created by Roark on 2021-03-03.
//

#ifndef FIVE_WONDERS_GRAPHIC_HPP
#define FIVE_WONDERS_GRAPHIC_HPP

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "world.hpp"

class Graphic {
public:
    Motion* motion;
    ShadedMeshRef* shadedMeshRef;
    static ECS::Entity createGraphic(vec2 centre, vec2 dims, std::string path);
};

class TileGraphic {
public:
    TilePosition* pos;
    ShadedMeshRef* shadedMeshRef;
    static ECS::Entity createTileGraphic(vec2 gridPos, std::string path);
};

#endif //FIVE_WONDERS_GRAPHIC_HPP
