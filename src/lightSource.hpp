//
// Created by Roark on 2021-03-12.
//

#ifndef FIVE_WONDERS_LIGHTSOURCE_HPP
#define FIVE_WONDERS_LIGHTSOURCE_HPP

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "world.hpp"

struct LightSource {
    vec3 lightColour = {1.0, 0.8, 0.7};
    float strength; //default = 1.0
};


#endif //FIVE_WONDERS_LIGHTSOURCE_HPP
