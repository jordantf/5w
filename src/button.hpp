//
// Created by Roark on 2021-02-10.
//

#ifndef TEAM10_BUTTON_H
#define TEAM10_BUTTON_H

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <functional>


class Button
{
public:
    vec2 centre;
    int width;
    int height;
    bool clicked = false;
    bool hidden = false;

    std::string key;
    std::string path;
    std::string clicked_path;
    std::string hover_path;

    std::function<void()> click_func;

    std::function<void()> mouse_over_func = nullptr;
    std::function<void()> mouse_off_func = nullptr;

    bool hover = false;
    bool toggle;
    bool disabled = false;

    // for key selection
    bool selected = false;

    // Creates all the associated render resources and default transform
    static ECS::Entity createButton(vec2 centre, int width, int height, std::function<void()> func, std::string path,
                                    std::string clicked_path = "", std::string hover_path = "", bool toggle = false,
                                    bool unique = false, int id = 0); // no anchor for now
    static Button& createButton(ECS::Entity entity, vec2 centre, int width, int height, std::function<void()> func, std::string path,
                      std::string clicked_path = "", std::string hover_path = "", bool toggle = false,
                      bool unique = false, int id = 0);

    bool on_click(vec2 mouse_pos, std::function<void()> sound);
    void enable();
    void disable();
    void on_release(vec2 mouse_pos);
    void on_release();
    void on_hover();
    void off_hover();
    void on_select();
    void off_select();
    bool notify(bool down, vec2 mouse_pos, std::function<void()> sound);
    void toggleOff();

    void toggleOn();
};

#endif //TEAM10_BUTTON_H
