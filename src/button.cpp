//
// Created by Roark on 2021-02-10.
//

#include "button.hpp"
#include "render.hpp"
#include "ui.hpp"
#include "graphic.hpp"
#include <iostream>
#include <utility>

ECS::Entity Button::createButton(vec2 centre, int width, int height, std::function<void()> func, std::string path,
                                 std::string clicked_path, std::string hover_path, bool toggle, bool unique, int id)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    auto& button = ECS::registry<Button>.emplace(entity);
    button.centre = centre;
    button.height = height;
    button.width = width;
    button.click_func = std::move(func);
    button.toggle = toggle;

    // Create the rendering components
    button.key = path;
    button.path = path;
    if (unique) {
        button.key = button.key + std::to_string(id);
    }
    ShadedMesh& resource = cache_resource(button.key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    if (clicked_path.compare("") == 0) {
        button.clicked_path = path;
    } else {
        button.clicked_path = clicked_path;
    }

    if (hover_path.compare("") == 0) {
        button.hover_path = path;
    } else {
        button.hover_path = hover_path;
    }

    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = centre;
    motion.scale = vec2({ width, height });

    return entity;
}

Button& Button::createButton(ECS::Entity entity, vec2 centre, int width, int height, std::function<void()> func, std::string path,
                                 std::string clicked_path, std::string hover_path, bool toggle, bool unique, int id)
{
    auto& button = ECS::registry<Button>.emplace(entity);
    button.centre = centre;
    button.height = height;
    button.width = width;
    button.click_func = std::move(func);
    button.toggle = toggle;

    // Create the rendering components
    button.key = path;
    button.path = path;
    if (unique) {
        button.key = button.key + std::to_string(id);
    }
    ShadedMesh& resource = cache_resource(button.key);
    if (resource.effect.program.resource == 0)
        RenderSystem::createSprite(resource, textures_path(path), "textured");

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    if (clicked_path.compare("") == 0) {
        button.clicked_path = path;
    } else {
        button.clicked_path = clicked_path;
    }

    if (hover_path.compare("") == 0) {
        button.hover_path = path;
    } else {
        button.hover_path = hover_path;
    }

    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = centre;
    motion.scale = vec2({ width, height });

    return button;
}

bool Button::on_click(vec2 mouse_pos, std::function<void()> sound) {
    if (hidden) return false;
    if (disabled) {
        return false;
    }
    if(!(mouse_pos.x > this->centre.x - this->width / 2 &&
            mouse_pos.x < this->centre.x + this->width / 2
    && mouse_pos.y > this->centre.y - this->height / 2 &&
            mouse_pos.y < this->centre.y + this->height / 2)) return false;

    sound();

    std::cout << "button pressed" << std::endl;
    clicked = !clicked;

    if (clicked) {
        // TODO: doesn't look like createSprite registers to ECS so overwriting the sprite like this should be ok
        RenderSystem::createSprite(cache_resource(this->key), textures_path(this->clicked_path), "textured");
    }

    if(click_func != nullptr && click_func != NULL)
        click_func();
    return true;
}

void Button::on_release(vec2 mouse_pos) {
//    std::cout << "button released" << std::endl;
    if (clicked == false) return;
    if (!toggle) { clicked = false; }

    if (!clicked) {
        // TODO: doesn't look like createSprite registers to ECS so overwriting the sprite like this should be ok
        RenderSystem::createSprite(cache_resource(this->key), textures_path(this->path), "textured");
    }
}

void Button::on_release() {
//    std::cout << "button released" << std::endl;
    if (clicked == false) return;
    if (!toggle) { clicked = false; }

    if (!clicked) {
        RenderSystem::createSprite(cache_resource(this->key), textures_path(this->path), "textured");
    }
}

void Button::on_hover() {
    // hover on
    hover = true;
    RenderSystem::createSprite(cache_resource(this->key), textures_path(this->hover_path), "textured");
}

void Button::off_hover() {
    // hover off
    hover = false;
    if (!clicked) {
        RenderSystem::createSprite(cache_resource(this->key), textures_path(this->path), "textured");
    } else {
        RenderSystem::createSprite(cache_resource(this->key), textures_path(this->clicked_path), "textured");
    }
}

void Button::on_select() {
    selected = true;

    // single button selector entity used for showGraphic func
    auto& bse = ECS::registry<ButtonSelector>.entities[0];
    if (ECS::registry<Graphic>.has(bse)) UISystem::destroyGraphic(bse);
    UISystem::showGraphic(bse, "buttons/selector.png", centre - vec2(width/2+10, 0), vec2(20, 20));
}

void Button::off_select() {
    selected = false;

    auto& bse = ECS::registry<ButtonSelector>.entities[0];
    UISystem::destroyGraphic(bse);
}

bool Button::notify(bool down, vec2 mouse_pos, std::function<void()> sound) {

    if(down) {
        if(on_click(mouse_pos, sound)) {
//            std::cout << "clicked: " << clicked_path << "\n";
            return true;
        }
        return false;
    }
    else on_release(mouse_pos);
    return false;
}

void Button::toggleOff() {
    if (!toggle) return;
    if (clicked == false) return;
    clicked = false;
    RenderSystem::createSprite(cache_resource(this->key), textures_path(this->path), "textured");
}

void Button::toggleOn() {
    if (!toggle) return;
    if (clicked == true) return;
    clicked = true;
    RenderSystem::createSprite(cache_resource(this->key), textures_path(this->clicked_path), "textured");
}

void Button::disable() {
    disabled = true;
}

void Button::enable() {
    disabled = false;
}

