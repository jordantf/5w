#pragma once

#include "common.hpp"
#include "enemy_ai.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"
#include "world.hpp"

//
// Basic pathfinding AI
class Enemy2AI : public EnemyAI {
    public:

    Enemy2AI(ECS::Entity enemy_p, WorldSystem *world_p) : EnemyAI(enemy_p, world_p) {
        enemy = enemy_p;
        world = world_p;
    }

    void beginTurn() override  {
        assert(ECS::registry<Turn>.has(enemy)); // Enemy entity has to have the current turn to begin turn!!
        Turn& turn = ECS::registry<Turn>.get(enemy);
        TilePosition& tile_position = ECS::registry<TilePosition>.get(enemy);

        // AI: DECISION TREE STRUCTURE
        // FIND PLAYER IF IN RANGE
        vec2 closest_player_pos = findClosestPlayer(tile_position.grid_pos);

        if (closest_player_pos.x != -1) { // closest_player_pos x value set to -1 if no player found in range.
            if (inAttackRange(closest_player_pos, tile_position.grid_pos) && turn.attacks > 0) {
                // Attacks closest target
                WorldSystem::attack_target(enemy, closest_player_pos, true);
            } else if (turn.movement > 0) {
                // Move towards target
                vec2 direction = closest_player_pos - tile_position.grid_pos;
                if (!world->move_character(turn, tile_position,
                        tile_position.grid_pos + quantizeDirection(direction))) {
                    turn.movement--; // If movement fails, decrement movement
                                     // points anyway (to avoid getting stuck forever).
                }
            } else {
                // Player is in detection range, but no more movement points; so end turn.
                world->step_turn();
            }
            return;
        }
        // IF NO PLAYER IN RANGE, MOVE UP (or down) AND END TURN
        if (turn.movement > 0) {
            if (!world->move_character(turn, tile_position,
                                  {tile_position.grid_pos.x, tile_position.grid_pos.y +
                                           (in_up_position ? 1 : -1)})) {// Alternates up and down
                in_up_position = !in_up_position; //if movement fails, reverse direction
                turn.movement--;
            }
        } else {
            in_up_position = !in_up_position;
            world->step_turn();
        }
    }

     vec2 findClosestPlayer(vec2 gridPos) {
        int closest = max_detection_range;
        vec2 closest_player_pos = {-1, 0}; // return x = -1 if no player found.
        for (auto player : ECS::registry<Player>.entities) {
            //look for closest player
            const TilePosition& player_tile = ECS::registry<TilePosition>.get(player);
            int dist = abs(player_tile.grid_pos.x - gridPos.x) + abs(player_tile.grid_pos.y - gridPos.y);
            if (dist < closest) {
                if(WorldSystem::hasLoS(gridPos, player_tile.grid_pos)) {
                    closest_player_pos = player_tile.grid_pos;
                    closest = dist;
                } else {
                    std::cout << "Enemy2: No line of sight to player!\n";
                }
            }
        }
        return closest_player_pos;
    }

    // Simple implementation for 1 ranged attack; if expand to more range, make sure to check if can see
    bool inAttackRange(vec2 closestPlayerGridPos, vec2 gridPos) {
        int dist = abs(closestPlayerGridPos.x - gridPos.x) + abs(closestPlayerGridPos.y - gridPos.y);
        return dist <= max_attack_range;
    }

    static vec2 quantizeDirection(vec2 direction) {
        if (abs(direction.x) > abs(direction.y)) {
            return {direction.x > 0? 1 : -1, 0};
        } else {
            return {0, direction.y > 0? 1 : -1};
        }
    }

protected:
    int max_detection_range = 10; // Max range of player detection
    int max_attack_range = 1; // Max range of attack

private:
    bool in_up_position = true;
};