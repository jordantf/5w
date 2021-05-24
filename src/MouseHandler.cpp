//
// Created by Roark on 2021-03-05.
//

#include "MouseHandler.hpp"
#include "button.hpp"
#include <iostream>

bool MouseHandler::handle_click(bool down, vec2 mouse_pos, std::function<void()> sound) {
//    std::cout << "handling\n";
    bool clicked = false;
    for(auto& b : ECS::registry<Button>.components) {
        clicked = clicked || b.notify(down, mouse_pos, sound);
    }
    return clicked;
}