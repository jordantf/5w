////
//// Created by Abdurahman Mudasiru on 2021-02-22.
////
//
#pragma once

#include "common.hpp"
#include "enemy_ai.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"
#include "world.hpp"
//
//

class Enemy1AI : public EnemyAI {
    public:


    Enemy1AI(ECS::Entity enemy_p, WorldSystem *world_p) : EnemyAI(enemy_p, world_p) {
        enemy = enemy_p;
        world = world_p;
    }

    void beginTurn() override  {
        assert(ECS::registry<Turn>.has(enemy)); // Enemy entity has to have the current turn to begin turn!!
        Turn& turn = ECS::registry<Turn>.get(enemy);
        if (turn.movement > 0) {
            TilePosition &tile_position = ECS::registry<TilePosition>.get(enemy);
            if (!world->move_character(turn, tile_position,
                                  {tile_position.grid_pos.x + 1, tile_position.grid_pos.y})) {
                for (auto player : ECS::registry<Player>.entities) {
                    TilePosition &player_pos = ECS::registry<TilePosition>.get(player);
                    if (player_pos.grid_pos.x == tile_position.grid_pos.x + 1)
                        WorldSystem::attack_target(enemy, {tile_position.grid_pos.x + 1, tile_position.grid_pos.y}, true);
                }

            }
            turn.movement = 0;
            return;
        }
        world->step_turn();
    }
};
//
//
//
//#endif //ENEMY1AI_HPP
