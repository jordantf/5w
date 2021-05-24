#pragma once

#include "common.hpp"
#include "enemy_ai.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"
#include "world.hpp"
#include "tile.hpp"
#include "player.hpp"

//std
#include <queue>
#include <iostream>



// BFS Pathfinding AI

class Enemy3AI : public EnemyAI {
    public:

    struct node_and_edge {
        vec2 vertex{};
        node_and_edge * link = nullptr; // used as vec2 * previous
        node_and_edge(vec2 v, node_and_edge* l) {
            vertex = v;
            link = l;
        }
    };

    Enemy3AI(ECS::Entity enemy_p, WorldSystem *world_p, bool airborne) : EnemyAI(enemy_p, world_p), airborne(airborne) {
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
        if (closest_player_pos.x != -1 && !no_paths) { // closest_player_pos x value set to -1 if no player found in range.
            if (inAttackRange(closest_player_pos, tile_position.grid_pos) && turn.attacks > 0) {
                // Attacks closest target
                world->attack_target(enemy, closest_player_pos, true);
                world->step_turn();
                no_paths = false;
            } else if (turn.movement > 0) {

                // if path hasn't been found, find a new path
                if(thePath.empty()) findPath(tile_position.grid_pos);
                if (thePath.empty()) {
                    no_paths = true; // if path still empty
                    return;
                }
//                std::cout << "= PATH =\n";
                std::cout << thePath.size() << "\n";
                for(auto & i : thePath) {
//                    std::cout << "cell " << i.vertex.x << ", " << i.vertex.y << "\n";
                }
                // follow the path
                world->move_character(turn,tile_position,thePath.front().vertex);
//                thePath.pop_back();
                thePath.erase(thePath.begin());

//                turn.movement--;

            } else {
                // Player is in detection range, but no more movement points; so end turn.
                thePath.clear();
                for(auto & i : processed) {
                    free(i);
                }
                processed.clear();
                world->step_turn();
                no_paths = false;
            }
            return;
        }

        in_up_position = !in_up_position;
        // clear path until next time
        thePath.clear();
        world->step_turn();
        no_paths = false;
    }

     vec2 findClosestPlayer(vec2 gridPos) {
        int closest = max_detection_range;
        vec2 closest_player_pos = {-1, 0}; // return x = -1 if no player found.
        for (auto player : ECS::registry<Player>.entities) {
            //look for closest player
            const TilePosition& player_tile = ECS::registry<TilePosition>.get(player);
            int dist = abs(player_tile.grid_pos.x - gridPos.x) + abs(player_tile.grid_pos.y - gridPos.y);
            if (dist < closest) {
                closest_player_pos = player_tile.grid_pos;
                closest = dist;
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

    virtual void findPath(vec2 gridPos) {
        auto paths = std::vector<vec2>();
        vec2 playerPos = findClosestPlayer(gridPos);

        std::queue<vec2> q2do = std::queue<vec2>();

        processed.clear();

        q2do.push(gridPos);

        while(!q2do.empty()) {
            // get the next node
            auto curNode = new vec2(q2do.front());
//            std::cout << "before: " << q2do.size() << "\n";
            q2do.pop();
//            std::cout << "after: " << q2do.size() << "\n";


//             stub
//            std::cout << "currentNode: " << curNode->x << ", " << curNode->y << "\n";

            // add first node with nullptr (so we know when to stop when backtracing)
            processed.emplace_back(new node_and_edge{*curNode, nullptr});

            //check tile above. Break if next to player
            auto nextNode = vec2(curNode->x, curNode->y-1);
//            std::cout << "checking top tile\n";
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile to the right. Break if next to player
            nextNode = vec2(curNode->x+1, curNode->y);
//            std::cout << "checking right tile\n";
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile below. Break if next to player
            nextNode = vec2(curNode->x, curNode->y+1);
//            std::cout << "checking down tile\n";
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile to the left. Break if next to player
            nextNode = vec2(curNode->x-1, curNode->y);
//            std::cout << "checking left tile\n";
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
        }
    }

    virtual // processes a cell. Returns true if the cell is the player cell.
    // false otherwise.
    bool processCell(vec2* curNode, vec2 cell, vec2 playerPos, std::queue<vec2> & q2do) {
//        std::cout << "processing cell " << cell.x << ", " << cell.y << "\n";
//        std::cout << cell.x << ", " << cell.y << " is somehow " << (isOccupied(cell)? "" : "not ") << "occupied\n";
        if(cell.x == playerPos.x && cell.y == playerPos.y) {
            setPath(*curNode);
            std::cout << "found path\n";
            return true;
        }
        else if(!isOccupied(cell)) {

            // make sure it hasn't already been processed
            for(auto & i : processed) {
                if(i->vertex.x == cell.x && i->vertex.y == cell.y) {
                    return false;
                }
            }

            // place current edge into the list of processed nodes and add nextNode (cell) to q2do
            q2do.push(cell);
            node_and_edge * prev;
            for(auto & i : processed) {
                if(i->vertex.x == curNode->x && i->vertex.y == curNode->y) {
                    prev = i;
                    break;
                }
            }
            auto newOne = new node_and_edge(cell, prev);
            processed.emplace_back(newOne);
        }
        return false;
    }

    // checks whether a cell is occupied
    bool isOccupied(vec2 cell) {
        if(WorldSystem::is_wall(cell)) {
            std::cout << cell.x << ", " << cell.y << " is wall\n";
            return true;
        }
        // if you can't fly, then holes are off limits
        if(!airborne && WorldSystem::is_hole(cell)) return true;

        for (auto entity : ECS::registry<TilePosition>.entities) {
            TilePosition& entity_pos = ECS::registry<TilePosition>.get(entity);
            if (entity_pos.grid_pos.x == cell.x && entity_pos.grid_pos.y == cell.y) {
                if (ECS::registry<Tile>.has(entity)) {
                    auto& tile = ECS::registry<Tile>.get(entity);
                    if (tile.isBlocking == 1) {
                        std::cout << cell.x << ", " << cell.y << " is a blocking tile\n";
                        return true;
                    }
                } else {
                    if(ECS::registry<Character>.has(entity))
                        return true;
                    return false;
                }
            }
        }
        return false;
    }

    virtual void setPath(vec2 goal) {
        node_and_edge * cur_node;

        for(auto & i : processed) {
            if(i->vertex.x == goal.x && i->vertex.y == goal.y) {
                cur_node = i;
                break;
            }
        }

        // if we get here, the path should be empty.
        assert(thePath.empty());

        while(cur_node->link != NULL && cur_node->link != nullptr) {
            thePath.emplace(thePath.begin(), *cur_node);
            cur_node = cur_node->link;
        }
    }

protected:
    int max_detection_range = 10; // Max range of player detection
    int max_attack_range = 1; // Max range of attack
    bool airborne = false;

    std::vector<node_and_edge> thePath;
    std::vector<node_and_edge*> processed = std::vector<node_and_edge*>();

private:
    bool in_up_position = true;
    bool no_paths = false;
};