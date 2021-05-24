//
// Created by Roark on 2021-03-05.
//

#ifndef FIVE_WONDERS_ARTIFACT_HPP
#define FIVE_WONDERS_ARTIFACT_HPP

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <functional>

class Artifact {
public:
    std::string name;
    int area;

    // Creates all the associated render resources and default transform
    static ECS::Entity createArtifact(vec2 gridPos, int area, std::string path);
};


#endif //FIVE_WONDERS_ARTIFACT_HPP
