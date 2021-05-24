#pragma once
//
// Created by Roark on 2021-02-28.
//
#include "common.hpp"
#include "render_components.hpp"
#include "render.hpp"


struct Animation {
    std::string animName;
    vec2 spriteDims;
    vec2 texDims;
    vec2 originOffset = {0,0};
    float framerate;
    int curFrame;
    int numFrames;

    vec2 cols_rows{};

    Animation* nextAnim;
    bool doesLoop;

    float ms = 0.f; // only for non looping

    bool flipped_x = false;
    bool flipped_y = false;

    ShadedMesh* shadedMesh;

    Animation(std::string animName, const std::string& texture_path, std::string shader_name, vec2 spriteSize, vec2 texSize, int numFrames,
                     float frameRate, bool doesLoop, Animation* nextAnim, vec2 origin);
};
//ANIMATION_HPP
