//
// Created by Roark on 2021-03-15.
//
#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// base class for spells to extend
class Spell {

public:
    // add fancier effects if needed
    // pull should just be push with negative cells
    // Please append new spell effects to the end to avoid affecting all previous saved spells.
    enum SpellEffect { DAMAGE, HEAL, GRAPPLE_TO, PUSH, TELEPORT_TO, SWITCH };

    Spell(SpellEffect effect);

    // what the spell does
    SpellEffect spell_effect;

    // list of cells that the spell can target. This allows us to be flexible and
    // have e.g. spells that can only be cast diagonally
    // Should be relative to caster position ({1,0} means caster can only target the cell to the right)
    std::vector<vec2> targetable_cells;

    // list of affected cells for damaging spells (or even switch spells).
    // Should be relative to targeted cell ({0,0} means it can only affects one cell)
    std::vector<vec2> affected_cells;

    // whether the spell must be cast on an entity (e.g. switch spell). NOT YET IMPLEMENTED
    bool requires_target;

    // whether or not it can be cast through walls. NOT YET IMPLEMENTED
    bool requires_los;

    // amount of value the spell does. used for push, damage, heal..
    int effect_value;

    // max amount of effective cells. Used only for movement spells
    int max_movement;

    // max range for spells. 
    int range;

    // point cost of the spell.
    int cost;

    // sets all cells between minRange and maxRange (both inclusive) to be targetable
    // if aligned is true, will only select orthogonal cells
    // this function overwrites targetable_cells
    inline void setTargetableRange(int minRange, int maxRange, bool aligned) {
        std::vector<vec2> cells = std::vector<vec2>();

        // simple implementation: iterate over a larger
        // bounding box and only add relevant boxes
        for(int i = -maxRange; i <= maxRange; i++) {
            for(int j = -maxRange; j <= maxRange; j++) {
                if(aligned) {
                    if((i == 0 && abs(j) >= minRange && abs(j) <= maxRange)
                    || (j == 0 && abs(i) >= minRange && abs(i) <= maxRange))
                        cells.emplace_back(vec2(i, j));
                } else {
                    if(abs(i)+abs(j) >= minRange && abs(i)+abs(j) <= maxRange)
                        cells.emplace_back(vec2(i, j));
                }
            }
        }
        targetable_cells = cells;
    }

    // TODO: test these
    // marks cells as targetable, Does not overwrite all cells
    inline void setCellsTargetable(std::vector<vec2> cells) {
        for(auto& cell : cells) {
            // check that it does not already exist
            if(std::find(targetable_cells.begin(), targetable_cells.end(), cell) == targetable_cells.end()) {
                // add it to list of targetable cellss
                targetable_cells.emplace_back(cell);
            }
        }
    }

    // marks cells as targetable, Does not overwrite all cells
    inline void setCellsUntargetable(std::vector<vec2> cells) {
        for(auto& cell : cells) {
            auto finding = std::find(targetable_cells.begin(), targetable_cells.end(), cell);
            // check that it exists
            if(finding != targetable_cells.end()) {
                // remove it from the list of viable cells
                targetable_cells.erase(finding);
            }
        }
    }

    // returns true if the vector contains pos
    inline bool can_hit(vec2 pos) { return std::find(targetable_cells.begin(), targetable_cells.end(), pos) != targetable_cells.end(); }
    
    // Spell::Spell(SpellEffect effect, int val, int range, bool needsTarget);
    Spell(SpellEffect effect, int cost, int val, int minrange, int maxrange, bool aligned, bool needsTarget, bool throughWall);
};


