//
// Created by Roark on 2021-03-05.
//

#include <functional>
#include "common.hpp"
#include "tiny_ecs.hpp"

#ifndef FIVE_WONDERS_MOUSEHANDLER_HPP
#define FIVE_WONDERS_MOUSEHANDLER_HPP


class MouseHandler {
public:
    bool handle_click(bool down, vec2 mouse_pos, std::function<void()> sound);
};


#endif //FIVE_WONDERS_MOUSEHANDLER_HPP
