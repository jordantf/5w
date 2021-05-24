//
// Created by Roark on 2021-03-01.
//

#include "animSystem.hpp"
#include <iostream>
#include "debug.hpp"
#include "player.hpp"

AnimSystem::AnimSystem() {
    this->time_ms = 0.f;
}

AnimSystem::~AnimSystem() {
    // nothing
}

void AnimSystem::step(float elapsed_ms) {
    time_ms += elapsed_ms;

    auto& animations = ECS::registry<Animation>;

    for(int i = 0; i < animations.size(); i++) {
        auto& animation = animations.components[i];

        // debug
        if(DebugSystem::in_debug_mode) {
            int new_frame = (int) floor(time_ms / (1000.f / animation.framerate)) % animation.numFrames + 1;

            if (new_frame != debug_frame) {
                std::cout << new_frame << "\n";
                debug_frame = new_frame;
            }
        }

        // floor to frame, then mod to be within numframes

        if (animation.doesLoop)
            animation.curFrame = ((int)floor(time_ms/(1000.f/animation.framerate)) % animation.numFrames) + 1;
        else {
            animation.ms += (elapsed_ms/(1000.f/animation.framerate));
            animation.curFrame = (int)floor(animation.ms) + 1;
            if (animation.curFrame > animation.numFrames) {
                auto &components = animations.components;
                if (animation.nextAnim != nullptr) {
                    animations.components[i] = *animation.nextAnim;
                }
            }
        }
    }

    for (auto& char_entity : ECS::registry<Character>.entities) {
        if (ECS::registry<Animation>.has(char_entity)) {
            auto& anim = ECS::registry<Animation>.get(char_entity);
            auto& character = ECS::registry<Character>.get(char_entity);

            if (ECS::registry<Player>.has(char_entity)) {
                // For setting animation to running
                if (ECS::registry<Turn>.has(char_entity)) {
                    //auto& turn = ECS::registry<Turn>.get(char_entity);
                    auto& p = ECS::registry<Player>.get(char_entity);
                    if (p.attacked) {
                        bool flip = true;
                        if (p.attackDir.x > 0 || (p.attackDir.y > 0 && p.attackDir.x >= 0)) flip = false;
                        if (!p.ranged) {
                            switchAnim (char_entity,
                                        *setAnim("player_sword_attack", "player_sword_attack.png",
                                                 { 32, 20 }, { 352, 20 },
                                                 11, 15, false, flip, false, *cache_anim("player_idle"), {-8,0}));
                        }
                        p.attacked = false;
                    } else if (character.anim_state != character.RUN && ECS::registry<Moving>.has(char_entity)) {
                        auto& mv = ECS::registry<Moving>.get(char_entity);
                        Animation* playerAnim;
                        if (mv.step.x > 0) {
                            playerAnim = setAnim("player_run_side", "player_run_side.png",
                                                 { 16, 20 }, { 64, 20 },
                                                 4, 12, true, false, false, nullptr);
                            playerAnim->flipped_x = false;
                        } else if (mv.step.x < 0) {
                            playerAnim = setAnim("player_run_side", "player_run_side.png",
                                                 { 16, 20 }, { 64, 20 },
                                                 4, 12, true, false, false, nullptr);
                            playerAnim->flipped_x = true;
                        } else if (mv.step.y > 0) {
                            playerAnim = setAnim("player_run_down", "player_run_down.png",
                                                 { 16, 20 }, { 64, 20 },
                                                 4, 12, true, false, false, nullptr);
                        } else {
                            playerAnim = setAnim("player_run_up", "player_run_up.png",
                                                 { 16, 20 }, { 64, 20 },
                                                 4, 12, true, false, false, nullptr);
                        }
                        character.anim_state = character.RUN;
                        switchAnim(char_entity, *playerAnim);
                    }
                }
                if (character.anim_state != character.IDLE && !ECS::registry<Moving>.has(char_entity)) {
                    switchAnim(char_entity, *setAnim("player_idle", "player_idle.png",
                                                    { 16, 20 }, { 608, 20 },
                                                    38, 5, true, false, false, nullptr));
                    character.anim_state = character.IDLE;
                }
            }
        }
    }

}

Animation* setAnim(std::string animName, const std::string& texture_path, vec2 spriteSize, vec2 texSize, int numFrames,
                               float frameRate, bool doesLoop, bool flipx, bool flipy, Animation* nextAnim, vec2 origin) {
    Animation** cached_anim = cache_anim(animName);
    if (*cached_anim == nullptr) {
        *cached_anim = new Animation(animName, texture_path, "animated",
                                    spriteSize, texSize, numFrames, frameRate, doesLoop, nextAnim, origin);
        (*cached_anim)->flipped_x = flipx;
        (*cached_anim)->flipped_y = flipy;
    }
    (*cached_anim)->flipped_x = flipx;
    (*cached_anim)->flipped_y = flipy;  
    return *cached_anim;
}

void AnimSystem::switchAnim(ECS::Entity e, Animation a) {
    ECS::registry<Animation>.remove(e);
    ECS::registry<Animation>.emplace(e, a);
    a.ms = 0.f;
}

// Returns a resource for every key, initializing with zero on the first query
Animation** cache_anim(std::string animName)
{
    static std::unordered_map<std::string, Animation*> resource_cache;
    const auto it = resource_cache.find(animName);
    if (it == resource_cache.end())
    {
        const auto it_succeeded = resource_cache.emplace(animName, nullptr);
        assert(it_succeeded.second);
        return &it_succeeded.first->second;
    }
    return &it->second;
}

