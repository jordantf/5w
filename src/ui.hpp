#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "text.hpp"
#include "button.hpp"

struct Tooltip {
    vec2 position;
    vec2 size = {300, 300};
    std::string content;
    Tooltip (vec2 pos, const std::string& cont) {
        position = pos ; content = cont;
    }
};

struct BattleUI {
    enum TYPE {M_ATTACK, R_ATTACK, MOVE, TURN, SPELL1, SPELL2, SPELL3, SPELL4, HUD} type;
    BattleUI(TYPE t);
};


struct ButtonSelector {
    vec2 position;
    vec2 size = {10, 17};
    ButtonSelector(vec2 pos) {
        position = pos;
    };
};

struct TurnGraphic {
    int order = 0;
};

struct Hidden {};

struct JODBUTTON {};

// Data structure for pebble-specific information
namespace UISystem {
    extern ECS::Entity tooltip;
    extern ECS::Entity player_info;
    extern ECS::Entity weapon_bg;
    extern ECS::Entity weapon_graphic;
    extern ECS::Entity weapon_name_text;
    extern std::shared_ptr<ECS::Entity> current_player;

    extern vec2 turn_order_position;
    extern std::list<ECS::Entity>* turn_queue;
    extern int turn_queue_size;

    extern std::shared_ptr<Font> header;
    extern std::shared_ptr<Font> sans;
    extern std::shared_ptr<Font> serif;

    extern enum UI_MODE {NONE, BATTLE} mode;

    // Returns an entity that is attached to the text component (or attaches text component to given entity)
    ECS::Entity showText(const std::string& content, vec2 position, float textSize);
    void showText(ECS::Entity entity, const std::string& content, vec2 position, float textSize);
    void destroyText(ECS::Entity entity);
    // Clears ALL text
    void clearText();

    // Returns an entity that is attached to a graphic component (or attaches graphic component to given entity)
    // The entity must not have a motion component already.
    // destroyGraphic will remove the motion component associated with the entity (as well as the shadedMeshRef)
    ECS::Entity showGraphic(const std::string& graphic_path, vec2 position, vec2 scale);
    void showGraphic(ECS::Entity entity, const std::string& graphic_path, vec2 position, vec2 scale);
    void destroyGraphic(ECS::Entity entity);
    // Clears all (non-tile) graphics
    void clearGraphics();

    void showTileGraphic(ECS::Entity entity, const std::string& graphic_path, vec2 gridPosition);
    void destroyTileGraphic(ECS::Entity entity);
    void clearTileGraphic();

    void showUI(UI_MODE new_mode);
    void showBattleUI();

    // call when start only
    void clearBattleUI();

    void hideUI();

    void updateUI();

    void updateButtons();

    // normal
    void createTooltip(Tooltip tt, vec2 position);
    void moveTooltip(vec2 position);

    void clearTooltip();

    void updateTurn();

    struct Highlight {}; // empty struct for highlight
    void enableHighlights(std::vector<vec2> positions, int mode, bool has_non_targetable = false,
                          std::vector<vec2> non_targetable_positions = std::vector<vec2>());
    void addHighlight(vec2 position, int mode, bool non_targetable);
    void disableHighlights();
};

