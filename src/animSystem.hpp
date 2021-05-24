//
// Created by Roark on 2021-03-01.
//

#include "common.hpp"
#include "animation.hpp"

#ifndef FIVE_WONDERS_ANIMSYSTEM_HPP
#define FIVE_WONDERS_ANIMSYSTEM_HPP


class AnimSystem {
public:
    AnimSystem();
    ~AnimSystem();
    void step(float elapsed_ms);
    static void switchAnim(ECS::Entity e, Animation a);
private:
    float time_ms;
    int debug_frame;
};

Animation* setAnim(std::string animName, const std::string& texture_path, vec2 spriteSize, vec2 texSize, int numFrames,
                   float frameRate, bool doesLoop, bool flipx, bool flipy, Animation* nextAnim, vec2 origin = {0,0});
Animation** cache_anim(std::string animName);

#endif //FIVE_WONDERS_ANIMSYSTEM_HPP
