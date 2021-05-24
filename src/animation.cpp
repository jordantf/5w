//
// Created by Roark on 2021-02-28.
//

#include "animation.hpp"

#include <utility>
#include <iostream>

Animation::Animation(std::string animName, const std::string& texture_path, std::string shader_name, vec2 spriteSize, vec2 texSize, int numFrames,
                     float frameRate, bool doesLoop, Animation* nextAnim, vec2 origin = {0,0})
                     : animName(std::move(animName)), spriteDims(spriteSize), texDims(texSize), numFrames(numFrames), framerate(frameRate),
                       doesLoop(doesLoop), nextAnim(nextAnim), originOffset(origin)
                     {
    // start at frame 1
    this->curFrame = 1;

    // rows and columns
    this->cols_rows = texSize/spriteSize;

    // create the sprite
    const std::string& key = texture_path;
    ShadedMesh& sprite = cache_resource(key);
    if (sprite.effect.program.resource == 0) {
        try {
            RenderSystem::createSprite(sprite, spritesheet_path(texture_path), std::move(shader_name));
        } catch (std::runtime_error& error) {
            // TODO: Remove when we have enemy sprites
            std::cout << "Entity sprite doesn't exist for " << animName << std::endl;
            std::cout << error.what() << std::endl;
        }
    }
    this->shadedMesh = &sprite;
}
