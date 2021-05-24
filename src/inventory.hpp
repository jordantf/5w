#pragma once
#include "common.hpp"
#include "tiny_ecs.hpp"
#include <functional>
#include "artifact.hpp"
#include "power.hpp"
#include "spell.hpp"
#include <map>

struct Key {
    int id;
    Key (int id);
    static ECS::Entity createKey (vec2 gridPosition, int id);
};

struct Weapon {
    explicit Weapon(std::basic_string<char> theName, int damage);
    std::string name;
    int damage;
};

struct Inventory {
    std::vector<Artifact> artifacts{};
    std::vector<Weapon> weapons{};
    std::list<Key> keys{}; // this should not be saved
    int gold = 0;
    int hp, ap, mp, sp;
    enum SpellButton { NONE, ONE, TWO, THREE, FOUR };
    std::map<SpellButton, Spell::SpellEffect> spell_map;
    ~Inventory();
};
