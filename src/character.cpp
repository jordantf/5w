#pragma once
// internal
#include "turn.hpp"
#include "character.hpp"
#include "render.hpp"
#include "lightSource.hpp"
// TODO: For debug purposes for now
#include <iostream>
#include <utility>
#include <math.h>

void print_loc(std::string str, vec2 pos) {
    std::cout << str << ": "<< pos.x << "," << pos.y << std::endl;
}
// check if target_pos is within spell casting range, eg: push must be 1 cell from player.
bool isValidCastLocation(vec2 self_pos, Spell spell, vec2 target_pos) {
    vec2 dist = target_pos - self_pos;

    if(WorldSystem::is_hole(target_pos)) return false; // CHANGE: fixed arr out of bounds

    for (auto grid_pos : spell.targetable_cells) {
        if (grid_pos.x == dist.x && grid_pos.y == dist.y) return true;
    }
    return false;
//    switch (spell.spell_effect) {
//    case Spell::SpellEffect::DAMAGE:
//        break;
//    case Spell::SpellEffect::HEAL:
//        break;
//    case Spell::SpellEffect::TELEPORT_TO:
//        return abs(dist.x) < spell.range && abs(dist.y) < spell.range;
//    case Spell::SpellEffect::GRAPPLE_TO:
//        // Can only target in the cardinal directions (directly up, left, right or down)
//        return self_pos.x == target_pos.x || self_pos.y == target_pos.y;
//    case Spell::SpellEffect::PUSH:
//        // both ok.
//        // return spell.can_hit(target_pos);
//        return abs(dist.x) + abs(dist.y) <= spell.range;  // spell.range = 1
//    case Spell::SpellEffect::SWITCH:
//        return abs(dist.x) < spell.range && abs(dist.y) < spell.range;
//    }
//    return false;
}
// For Moving.step
vec2 calculateMovingStep(vec2 src, vec2 dest, float factor = 0.05) {
    return (dest - src) * 0.05f;
}
// determine the furthest position it can travel.
vec2 findTheFurthestLoc(vec2 src_loc, vec2 direction, int steps = 30) {
    assert(abs(direction.x + direction.y) == 1);
    vec2 final_pos = src_loc;
    vec2 next_pos = src_loc;
    // determine the furthest  position it can be pushed to.
    while (steps > 0) {
        next_pos = next_pos + direction;
        // not occupied by other entity, and not wall
        bool not_occupied = !WorldSystem::isOccupied(next_pos);
        bool not_wall = !WorldSystem::is_wall(next_pos);
        bool not_hole = !WorldSystem::is_hole(next_pos);

        if (not_occupied && not_wall && not_hole) {
            final_pos = next_pos;
            steps--;
        } else if (not_occupied && not_wall) {
            // step but don't update final position
            steps--;
        } else {
            break;
        }
    }
    return final_pos;
}
bool push_enemy(ECS::Entity enemy, vec2 enemy_pos, vec2 direction, int push_steps) {
    // one cell from orgin. eg: right:(0,1) up:(-1,0), down:(1,0)..
    vec2 final_pos = findTheFurthestLoc(enemy_pos, direction, push_steps);
    
    // if enemy needs to move   
    if (enemy_pos != final_pos) {
        // handle movement
        TilePosition& enemy_tile = ECS::registry<TilePosition>.get(enemy);
        auto& moving = ECS::registry<Moving>.emplace(enemy);
        moving.oldPos = enemy_pos;
        moving.step = calculateMovingStep(enemy_pos, final_pos, 0.5);
            
        // update enemy pos in TILEPOSITION;
        enemy_tile.grid_pos = final_pos;

    }
    SoundSystem::play("spell_push");
    return true;
}
// required: range is check
bool switch_position(ECS::Entity self, ECS::Entity enemy) {
    TilePosition& self_tile = ECS::registry<TilePosition>.get(self);
    TilePosition& enemy_tile = ECS::registry<TilePosition>.get(enemy);

    TilePosition temp = enemy_tile;
    enemy_tile = self_tile;
    self_tile = temp;
    SoundSystem::play("spell_switch");
    return true;
}
// required: range is check
bool grapple(ECS::Entity& self, vec2 self_pos, vec2 cursor_loc) {
    // step1. check if straight up, down, left, right // handled in isValidCastLocation()
    
    // step2. check if cursor is selected on, or adjacent to, a wall or an enemy;
    std::vector<vec2> adj_cells({ 
        cursor_loc,
        { cursor_loc.x,     cursor_loc.y + 1},
        { cursor_loc.x,     cursor_loc.y - 1}, 
        { cursor_loc.x + 1, cursor_loc.y},
        { cursor_loc.x - 1, cursor_loc.y},
        });
    bool isNearWallOrEnemy = false;
    for (vec2 loc : adj_cells) {
        if (WorldSystem::is_wall(loc)) {
            isNearWallOrEnemy = true;
            break;
        }
    }
    if (!isNearWallOrEnemy) {
        print_loc("invalid location: not near wall or enemy", cursor_loc);
        return false;
    }
    // step3. find the furthurest it can travel
    // find direction vector by dividing the displacement by itself and avoiding division by zero.
    vec2 direction = (cursor_loc - self_pos);
    int dist;
    if (direction.x != 0) {
        dist = abs(direction.x);
        direction.x = direction.x / abs(direction.x);
        
    }
    else {
        dist = abs(direction.y);
        direction.y = direction.y / abs(direction.y);
        
    }

    vec2 final_pos = findTheFurthestLoc(self_pos, direction, dist);

    // step4. move player
    if (self_pos != final_pos) {
        // handle movement
        TilePosition& tile = ECS::registry<TilePosition>.get(self);
        auto& moving = ECS::registry<Moving>.emplace(self);
        moving.oldPos = self_pos;
        moving.step = calculateMovingStep(self_pos, final_pos, 0.5);

        // update enemy pos in TILEPOSITION;
        tile.grid_pos = final_pos;

        print_loc("moved", {0,0});
    }
    SoundSystem::play("spell_hook");
    return true;
}
bool heal(ECS::Entity self, Spell spell) {
    auto& thing = ECS::registry<Character>.get(self);
    thing.currentHealth += (spell.effect_value);
    SoundSystem::play("spell");
    return true;
}
bool Character::castSpell(ECS::Entity self, Spell spell, vec2 target_pos) {
    TilePosition& self_pos = ECS::registry<TilePosition>.get(self);
    std::cout << "trying to cast spell\n";

    vec2 target_cell = target_pos-self_pos.grid_pos;
    
    if(isValidCastLocation(self_pos.grid_pos, spell, target_pos)) {
        ECS::Entity enemy;
        if(spell.requires_target) {
            // check if there is target
            try {
                enemy = WorldSystem::findCharacterAt(target_pos);
            }
            catch (std::runtime_error& error) {
                std::cout << "No enemy at location" << std::endl;
                return false;
            }
        }
        if(spell.requires_los) {
            //TODO: Check line of sight
            if (!WorldSystem::hasLoS(self_pos.grid_pos, target_pos))
                return false; // if no line of sight
        }

        // TODO: Implement spell effects
        // For spells that affect an entity, you can find the entity with
        // tileposition = target pos + self grid pos
        
        switch (spell.spell_effect) {
            case Spell::SpellEffect::DAMAGE:
                break;
            case Spell::SpellEffect::HEAL: {
                // self heal
                return heal(self, spell);
            }    
            case Spell::SpellEffect::TELEPORT_TO:
                break;
            case Spell::SpellEffect::GRAPPLE_TO:
                return grapple(self, self_pos.grid_pos, target_pos);
            case Spell::SpellEffect::PUSH:
                return push_enemy(enemy, target_pos, target_cell, spell.effect_value);
               
            case Spell::SpellEffect::SWITCH:
                return switch_position(self, enemy);
        }
    }
    else {
        // not within cast range.
        std::cout << "== Unable to cast spell ==\n";
    }
    // if we can't target the cell, return false
    return false;
}

ECS::Entity Character::createCharacter(const std::string& character_name, vec2 grid_position, int movement_points) {
    auto entity = ECS::Entity();

    // Below code is based off of salmon.cpp, for rendering
    ShadedMesh& resource = cache_resource(character_name);
    std::string texture_path = character_name + "_sprite.png";

    if (resource.effect.program.resource == 0) {
        try {
            RenderSystem::createSprite(resource, textures_path(texture_path), "textured");
        } catch (std::runtime_error& error) {
            // TODO: Remove when we have enemy sprites
            std::cout << "Entity sprite doesn't exist for " << character_name << std::endl;
            std::cout << error.what() << std::endl;
        }
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    // TODO: Resize this maybe
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = {0, 0};

    ECS::registry<Character>.emplace(entity);
    auto& character = ECS::registry<Character>.get(entity);
    character.name = character_name;
    character.max_movement = movement_points;

    return entity;
}


ECS::Entity Character::createAnimatedCharacter(const std::string& character_name, vec2 grid_position,
                                               Animation starting_anim, int movement_points, int HP, int attack) {

    auto entity = ECS::Entity();
    auto animation = std::move(starting_anim);

//    std::cout << "cols: " << animation.cols_rows.x << "rows: " << animation.cols_rows.y << "\n";

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<Animation>.emplace(entity, animation);


    TilePosition& tile_position = ECS::registry<TilePosition>.emplace(entity);
    tile_position.grid_pos = grid_position;
    // TODO: Resize this maybe
    tile_position.scale = { 100, 100 };
    tile_position.screen_pos = {0, 0};

    ECS::registry<Character>.emplace(entity);
    auto& character = ECS::registry<Character>.get(entity);
    character.name = character_name;
    character.max_movement = movement_points;
    character.maxHealth = HP;
    character.currentHealth = HP;
    character.meleeDamage = attack;
    character.anim_state = IDLE;
    
    return entity;
}
