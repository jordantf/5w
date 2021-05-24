// Header
#include "ui.hpp"
#include "tiny_ecs.hpp"
#include "player.hpp"
#include <sstream>


BattleUI::BattleUI(TYPE t) {
    type = t;
}

namespace UISystem
{
    ECS::Entity showText(const std::string& content, vec2 position, float textSize) {
        ECS::Entity entity = ECS::Entity();
        ECS::registry<Text>.insert(
                entity,
                Text(content, serif, {position.x, position.y}, textSize)
        );
        return entity;
    }

    void showText(ECS::Entity entity, const std::string& content, vec2 position, float textSize) {
        ECS::registry<Text>.insert(
                entity,
                Text(content, serif, {position.x, position.y}, textSize)
        );
    }

    void destroyText(ECS::Entity entity) {
        if (ECS::registry<Text>.has(entity)) {
            ECS::registry<Text>.remove(entity);
        }
    }

    void clearText() {
        while (ECS::registry<Text>.entities.size()>0)
            ECS::ContainerInterface::remove_all_components_of(ECS::registry<Text>.entities.back());
    }

    ECS::Entity showGraphic(const std::string& graphic_path, vec2 position, vec2 scale) {
        return Graphic::createGraphic(position, scale, graphic_path);
    }

    void showGraphic(ECS::Entity entity, const std::string& graphic_path, vec2 position, vec2 scale) {
        auto& graphic = ECS::registry<Graphic>.emplace(entity);

        std::string key = graphic_path;
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0)
            RenderSystem::createSprite(resource, textures_path(graphic_path), "textured");

        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
        auto& mesh = ECS::registry<ShadedMeshRef>.emplace_with_duplicates(entity, resource);

        auto& motion = ECS::registry<Motion>.emplace_with_duplicates(entity);
        motion.position = position;
        // Setting initial values, scale is negative to make it face the opposite way
        motion.scale = scale;

        graphic.shadedMeshRef = &mesh;
        graphic.motion = &motion;
    }


    void destroyGraphic(ECS::Entity entity) {
        if (ECS::registry<Graphic>.has(entity)) {
            ECS::registry<ShadedMeshRef>.remove(entity);
            ECS::registry<Motion>.remove(entity);
            ECS::registry<Graphic>.remove(entity);
        }
    }

    void clearGraphics() {
        for (auto e : ECS::registry<Graphic>.entities) {
            ECS::registry<ShadedMeshRef>.remove(e);
            ECS::registry<Motion>.remove(e);
            ECS::registry<Graphic>.remove(e);
        }
    }

    void showTileGraphic(ECS::Entity entity, const std::string& graphic_path, vec2 gridPosition) {
        auto& graphic = ECS::registry<TileGraphic>.emplace_with_duplicates(entity);

        std::string key = graphic_path;
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0)
            RenderSystem::createSprite(resource, textures_path(graphic_path), "textured");

        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
        auto& mesh = ECS::registry<ShadedMeshRef>.emplace_with_duplicates(entity, resource);

        auto& tilePos = ECS::registry<TilePosition>.emplace_with_duplicates(entity);
        tilePos.grid_pos = gridPosition;
        // Setting initial values, scale is negative to make it face the opposite way
        tilePos.scale = {100, 100};
        tilePos.isSolid = false;

        graphic.shadedMeshRef = &mesh;
        graphic.pos = &tilePos;
    }

    void destroyTileGraphic(ECS::Entity entity) {
        if (ECS::registry<TileGraphic>.has(entity)) {
            ECS::registry<ShadedMeshRef>.remove(entity);
            ECS::registry<Motion>.remove(entity);
            ECS::registry<TileGraphic>.remove(entity);
        }
    }
    void clearTileGraphic() {
        for (auto e : ECS::registry<TileGraphic>.entities) {
            ECS::registry<ShadedMeshRef>.remove(e);
            ECS::registry<TilePosition>.remove(e);
            ECS::registry<TileGraphic>.remove(e);
        }
    }


    void createTooltip(Tooltip tt, vec2 position) {
        UISystem::clearTooltip();
        ECS::registry<Text>.insert(
                tooltip,
                Text(tt.content, serif, {position.x, position.y + 50.f}, 0.5f)
        );
    }

    void moveTooltip(vec2 position) {
        if (ECS::registry<Text>.has(tooltip)) {
            Text &text = ECS::registry<Text>.get(tooltip);
            text.position = {position.x, position.y + 50.f};
        }
    }

    void clearTooltip() {
        if (ECS::registry<Text>.has(tooltip)) {
            ECS::registry<Text>.remove(tooltip);
        }
    }

    void showUI(UI_MODE new_mode) {
        switch (new_mode) {
            case UISystem::NONE:
                UISystem::hideUI();
                break;
            case UISystem::BATTLE:
                showBattleUI();
                break;
        }
    }

    void updateUI() { std::cout << "updateing ui\n";
        switch (mode) {
            case UISystem::NONE:
                break;
            case UISystem::BATTLE:
                showBattleUI();
                break;
        }
    }

    void hideUI() {
        mode = UISystem::NONE;
        for (auto& button : ECS::registry<Button>.entities) {
            if (!ECS::registry<Hidden>.has(button))
                ECS::registry<Hidden>.emplace(button);
            auto& b = ECS::registry<Button>.get(button);
            b.hidden = false;
        }
        for (auto& e : ECS::registry<BattleUI>.entities) {
            if (ECS::registry<Text>.has(e)) {
                std::string& content = ECS::registry<Text>.get(e).content;
                content.clear();
            }
        }
    }

    void showBattleUI() {
        if (mode != UISystem::BATTLE) {
            //toggle visibility of buttons
            for (auto& e : ECS::registry<BattleUI>.entities) {
                if (ECS::registry<Hidden>.has(e)) {
                    ECS::registry<Hidden>.remove(e);
                }
                if (ECS::registry<Button>.has(e)) {
                    auto& b = ECS::registry<Button>.get(e);
                    b.hidden = false;
                }
            }
        }
        mode = UISystem::BATTLE;
        if (current_player != nullptr) {
            if (!ECS::registry<Text>.has(player_info)) {
                ECS::registry<Text>.insert(
                        player_info,
                        Text("", header, {220, 630}, 0.55f)
                );
                ECS::registry<BattleUI>.emplace(player_info, BattleUI::HUD);
            }
            std::stringstream stream;
            Character& character = ECS::registry<Character>.get(*current_player);
            stream << character.currentHealth << "/" << character.maxHealth << "HP\n";
            if (ECS::registry<Turn>.has(*current_player)) {
                Turn& turn = ECS::registry<Turn>.get(*current_player);                
                Inventory& i = ECS::registry<Inventory>.get(*current_player);

                stream << "Gold: ";
                stream << i.gold;
                stream << "\n";

                stream << "MP ";
                for(int i = 0; i < turn.movement; i++) stream << "#";
                stream << "\n";

                stream << "AP ";
                for(int i = 0; i < turn.attacks; i++) stream << "@";
                stream << "\n";

                stream << "SP ";
                for(int i = 0; i < turn.spells; i++) stream << "|";
                stream << "\n";

                stream << "} x" << i.keys.size();
            }
            auto& text = ECS::registry<Text>.get(player_info);
            text.content.clear();
            text.content = stream.str();

            // set player weapon
            if(!ECS::registry<Graphic>.has(weapon_bg)) {
                weapon_bg = showGraphic("weapons/background.png", {100, 700}, {160, 160});
                ECS::registry<BattleUI>.emplace(weapon_bg, BattleUI::HUD);
            }
            if(ECS::registry<Weapon>.size() > 0) {
                std::string w = ECS::registry<Weapon>.get(*current_player).name;
                if (ECS::registry<Graphic>.has(weapon_graphic)) destroyGraphic(weapon_graphic);
                std::cout << "graphic string is weapons/" + w + ".png\n";

                weapon_graphic = showGraphic("weapons/" + w + ".png", {100, 700}, {160, 160});
                destroyText(weapon_name_text);
                weapon_name_text = showText(w, {20, 580}, 0.5f);
                ECS::registry<BattleUI>.emplace(weapon_name_text, BattleUI::HUD);
            }
        }
        updateButtons();
    }

    void updateButtons() {
        if (current_player == nullptr) return;

        Player& player = ECS::registry<Player>.get(*current_player);
        for (auto& e : ECS::registry<BattleUI>.entities) {
            auto& bui = ECS::registry<BattleUI>.get(e);
            if(bui.type == BattleUI::HUD) continue;

            auto &button = ECS::registry<Button>.get(e);
            bool hold = true;
            switch (bui.type) {
                case BattleUI::MOVE:
                    hold = player.moving || player.hasPath;
                    break;
                case BattleUI::M_ATTACK:
                    hold = player.attacking && !player.ranged;
                    break;
                case BattleUI::R_ATTACK:
                    hold = player.attacking && player.ranged;
                    break;
                case BattleUI::SPELL1:
                    hold = player.casting && player.selected_spell == WorldSystem::SpellButton::ONE;
                    break;
                case BattleUI::SPELL2:
                    hold = player.casting && player.selected_spell == WorldSystem::SpellButton::TWO;
                    break;
                case BattleUI::SPELL3:
                    hold = player.casting && player.selected_spell == WorldSystem::SpellButton::THREE;
                    break;
                case BattleUI::SPELL4:
                    hold = player.casting && player.selected_spell == WorldSystem::SpellButton::FOUR;
                    break;
                case BattleUI::TURN:
                    break;
                default: break;
            }
            if (hold == false) {
                button.toggleOff();
            }
        }
    }

    void updateTurn() {
        if (turn_queue == nullptr || turn_queue->empty()) return;
        while (ECS::registry<TurnGraphic>.components.size() < turn_queue_size) {
            ECS::Entity entity = ECS::Entity();
            ECS::registry<TurnGraphic>.emplace(entity);
            ECS::registry<BattleUI>.emplace(entity, BattleUI::HUD);
        }
        int i = 0; // iterator through turn_queue
        int j = 0; // 0th j position would be display at top, so this is the order of displaying.
                   // This ranges from 0 to turn_queue_size - 1 (inclusive)
        // Create a copy
        auto turns_vec = std::vector<ECS::Entity>();
        for (const ECS::Entity e : *turn_queue) {
            turns_vec.push_back(e);
        }
        for (auto e : ECS::registry<TurnGraphic>.entities) {
            if (i >= turn_queue->size())
                i %= turn_queue->size();
            ECS::Entity char_e = turns_vec[i];
            auto& character = ECS::registry<Character>.get(char_e);

            auto& t_graphic = ECS::registry<TurnGraphic>.get(e);
            t_graphic.order = j;
            std::stringstream graphic_path;
            auto name = std::string(character.name);
            if (name != "player")
                name.pop_back();
            graphic_path << "turn_graphics/" << name << "_turn.png";
            destroyGraphic(e);
            showGraphic(e, graphic_path.str(), {turn_order_position.x,turn_order_position.y + j * 67}, {225, 67});
            i++; j++;
        }
    }

    void clearBattleUI() {
        while (ECS::registry<BattleUI>.entities.size()>0)
            ECS::ContainerInterface::remove_all_components_of(ECS::registry<BattleUI>.entities.back());
    }

    void enableHighlights(std::vector<vec2> positions, int mode, bool has_non_targetable, std::vector<vec2> non_targetable_positions) {
        assert(ECS::registry<Highlight>.components.size() == 0); // There should be no existing highlights at this point
        std::string path;
        if(mode == 1) path = "ui/highlights/move.png";
        if(mode == 2) path = "ui/highlights/attack.png";
        if(mode == 3) path = "ui/highlights/spell.png";

        for(int i = 0; i < positions.size(); i++) {
            ECS::Entity highlightContainer = ECS::Entity();
            showTileGraphic(highlightContainer, path, positions[i]);
            // mark the entity as highlight
            ECS::registry<Highlight>.emplace(highlightContainer);
        }
        if (!has_non_targetable) return;
        path = "ui/highlights/spell_blocked.png";
        for(int i = 0; i < non_targetable_positions.size(); i++) {
            ECS::Entity highlightContainer = ECS::Entity();
            showTileGraphic(highlightContainer, path, non_targetable_positions[i]);
            // mark the entity as highlight
            ECS::registry<Highlight>.emplace(highlightContainer);
        }
    }

    void addHighlight(vec2 position, int mode, bool non_targetable) {
        std::string path;
        if(mode == 1) path = "ui/highlights/move.png";
        if(mode == 2) path = "ui/highlights/attack.png";
        if(mode == 3) path = "ui/highlights/spell.png";
        if (non_targetable) path = "ui/highlights/spell_blocked.png";
        ECS::Entity highlightContainer = ECS::Entity();
        showTileGraphic(highlightContainer, path, position);
        // mark the entity as highlight
        ECS::registry<Highlight>.emplace(highlightContainer);
    }

    void disableHighlights() {
        while (ECS::registry<Highlight>.entities.size() > 0) {
            destroyTileGraphic(ECS::registry<Highlight>.entities.back());
            ECS::registry<Highlight>.remove(ECS::registry<Highlight>.entities.back());
        }
    }

    ECS::Entity tooltip = ECS::Entity();
    ECS::Entity player_info = ECS::Entity();
    ECS::Entity weapon_bg;
    ECS::Entity weapon_graphic;
    ECS::Entity weapon_name_text;
    std::shared_ptr<ECS::Entity> current_player = nullptr;

    vec2 turn_order_position = {0,0};
    std::list<ECS::Entity>* turn_queue = nullptr;
    int turn_queue_size = 8;

    std::shared_ptr<Font> header =  nullptr;
    std::shared_ptr<Font> sans =  nullptr;
    std::shared_ptr<Font> serif = nullptr;
    UI_MODE mode = UISystem::NONE;
}
