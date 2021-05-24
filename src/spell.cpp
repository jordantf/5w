//
// Created by Roark on 2021-03-15.
//

#include "spell.hpp"

Spell::Spell(SpellEffect effect) {
    this->spell_effect = effect;
    // list of cells that the spell can target. This allows us to be flexible and
    // have e.g. spells that can only be cast diagonally
    // Should be relative to caster position ({1,0} means caster can only target the cell to the right)
    std::vector<vec2> targetable_cells;

    // list of affected cells for damaging spells (or even switch spells).
    // Should be relative to targeted cell ({0,0} means it can only affects one cell)
    std::vector<vec2> affected_cells;

    // whether the spell must be cast on an entity (e.g. switch spell). NOT YET IMPLEMENTED
    requires_target = false ;

    // whether or not it can be cast through walls. NOT YET IMPLEMENTED
    requires_los = false;

    // amount of value the spell does. used for push, damage, heal..
    effect_value = 0;

    // max amount of effective cells. Used only for movement spells 
    // same as effect_value?
    max_movement = 0;

    // max range for spells. 
    range = 0;
}

Spell::Spell(SpellEffect effect, int cost, int val, int minrange, int maxrange, bool aligned, bool needsTarget, bool throughWall) {
    this->spell_effect = effect;
//    std::vector<vec2> targetable_cells;
//    std::vector<vec2> affected_cells;
    requires_target = needsTarget;
    requires_los = !throughWall;
    effect_value = val;
    this->setTargetableRange(minrange, maxrange, aligned);
    this->range = maxrange;
    this->cost = cost;

    // TODO not neccesary?
    max_movement = 0;
}
