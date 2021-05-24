#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "world.hpp"
#include "spell.hpp"
#include <queue>

#include <vector>
#include <map>


class Player
{
public:
    bool hasArtifact;
    Player (ECS::Entity e, WorldSystem *w);
    
    std::map<WorldSystem::SpellButton, Spell*> spell_map;
    void move();

    void modify_spell_map(WorldSystem::SpellButton selected, Spell::SpellEffect spell_effect);
    void show_tiles();
    void on_click_tile(vec2 targetTile);
    void on_click_move();
    void on_click_attack();
    void on_click_end_turn();

    void on_click_melee_attack();

    void on_click_ranged_attack();

    void on_click_spell_cast(WorldSystem::SpellButton button); // testspell
    bool casting    = false; // testspell
    bool casted     = false; // testspell  // not used
    WorldSystem::SpellButton selected_spell;

    bool ranged     = false;
    bool attacking  = false;
    bool moving     = false;
    bool attacked   = false; // Bool for animation trigger
    vec2 attackDir  = {1,0};
    bool hasPath    = false;

private:
    ECS::Entity entity;
    WorldSystem* world;


    void findPath(vec2 gridPos, vec2 targetTile, int depth, bool ignoreOccupy = false);
    bool processCell(vec2* curNode, vec2 cell, vec2 goalPos, std::queue<vec2> & q2do, int depth);
    bool isOccupied(vec2 cell);
    void setPath(vec2 goal);

    bool inMeleeRange(vec2 charPos, vec2 targetPos);

    struct node_and_edge {
        vec2 vertex{};
        node_and_edge * link = nullptr; // used as vec2 * previous
        int depth;
        node_and_edge(vec2 v, node_and_edge* l, int d) {
            vertex = v;
            link = l;
            depth = d;
        }
    };

    std::vector<node_and_edge> thePath;
    std::vector<node_and_edge*> processed = std::vector<node_and_edge*>();

    std::vector<vec2> moveable_tiles = std::vector<vec2>();

    void clearPath();

    void show_move_tiles(vec2 playerPos, int movement);

    void show_ranged_tiles(vec2 playerPos, int range);

    void show_spell_tiles(vec2 playerPos);
};
