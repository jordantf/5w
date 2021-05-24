#pragma once

#include "common.hpp"
#include "enemy_ai.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"
#include "world.hpp"
#include "tile.hpp"

//std
#include <queue>
#include <iostream>



// Pirate
// BFS Pathfinding AI with grapple spell

class Enemy5AI : public EnemyAI {
public:
    struct node_and_edge {
        vec2 vertex{};
        node_and_edge * link = nullptr; // used as vec2 * previous
        node_and_edge(vec2 v, node_and_edge* l) {
            vertex = v;
            link = l;
        }
    };

    Enemy5AI(ECS::Entity enemy_p, WorldSystem *world_p) : EnemyAI(enemy_p, world_p) {
        enemy = enemy_p;
        world = world_p;
    }

    void beginTurn() override  {
        assert(ECS::registry<Turn>.has(enemy)); // Enemy entity has to have the current turn to begin turn!!
        Turn& turn = ECS::registry<Turn>.get(enemy);
        TilePosition& tile_position = ECS::registry<TilePosition>.get(enemy);

        // AI: DECISION TREE STRUCTURE
        // FIND PLAYER IF IN RANGE
        std::cout << "===============find closest player===================\n";
        vec2 closest_player_pos = findClosestPlayer(tile_position.grid_pos);
        if (closest_player_pos.x != -1 && !no_paths) { // closest_player_pos x value set to -1 if no player found in range.
            if (inAttackRange(closest_player_pos, tile_position.grid_pos) && turn.attacks > 0) {
                // Attacks closest target
                world->attack_target(enemy, closest_player_pos, true);
                world->step_turn();
                no_paths = false;
            } else if (turn.movement > 0) {
                vec2 closest = findClosestDirection(tile_position.grid_pos, closest_player_pos);
                if(length(closest - tile_position.grid_pos) > 0.5 && turn.spells > 0) {
                    // if grapple is better
                    auto& dude = ECS::registry<Character>.get(enemy);
                    dude.castSpell(enemy, *GRAPPLE, closest);
                    turn.spells--;
                } else {
                    // if walking is better

                    // if path hasn't been found, find a new path
                    if (thePath.empty()) findPath(tile_position.grid_pos);
                    if (thePath.empty()) {
                        no_paths = true; // if path still empty
                        return;
                    }
//                std::cout << "= PATH =\n";
                    std::cout << thePath.size() << "\n";
                    for (auto &i : thePath) {
//                    std::cout << "cell " << i.vertex.x << ", " << i.vertex.y << "\n";
                    }
                    // follow the path
                    world->move_character(turn, tile_position, thePath.front().vertex);
//                thePath.pop_back();
                    thePath.erase(thePath.begin());

                turn.movement--;
                }

            } else if (turn.spells > 0) {

                vec2 closest = findClosestDirection(tile_position.grid_pos, closest_player_pos);
                if(length(closest - tile_position.grid_pos) > 0.5) {
                    auto& dude = ECS::registry<Character>.get(enemy);
                    dude.castSpell(enemy, *GRAPPLE, closest);
                }
                turn.spells--;
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
            // clear path until next time
            thePath.clear();
            world->step_turn();
            no_paths = false;
        }
    }

    vec2 findClosestDirection(vec2 gridPos, vec2 playerPos) {
        vec2 currentpos;
        vec2 nextpos;
        vec2 best[4];
        // find endpoint for all directions
        for(int i = 0; i < 4; i++) {
            currentpos = gridPos;
            while (true) {
                nextpos = currentpos;
                if (i == 0)
                    nextpos.x += 1;
                if (i == 1)
                    nextpos.y += 1;
                if (i == 2)
                    nextpos.x -= 1;
                if (i == 3)
                    nextpos.y -= 1;

                if (isOccupied(nextpos) == 2) {
                    best[i] = gridPos;
                    break;
                }
                if (isOccupied(nextpos) == 1) {
                    best[i] = currentpos;
                    break;
                }
                // if nothing found, proceed
                currentpos = nextpos;
            }
        }

        float dist[4];
        // find shortest one to player
        dist[0] = length(best[0] - playerPos);
        dist[1] = length(best[1] - playerPos);
        dist[2] = length(best[2] - playerPos);
        dist[3] = length(best[3] - playerPos);
//        std::cout << "east is " << best[0].x << ", " << best[0].y << "\n";
//        std::cout << "south is " << best[1].x << ", " << best[1].y << "\n";
//        std::cout << "west is " << best[2].x << ", " << best[2].y << "\n";
//        std::cout << "north is " << best[3].x << ", " << best[3].y << "\n";
        if(dist[0] <= dist[1] && dist[0] <= dist[2] && dist[0] <= dist[3]) return best[0];
        if(dist[1] <= dist[0] && dist[1] <= dist[2] && dist[1] <= dist[3]) return best[1];
        if(dist[2] <= dist[0] && dist[2] <= dist[1] && dist[2] <= dist[3]) return best[2];
        if(dist[3] <= dist[0] && dist[3] <= dist[1] && dist[3] <= dist[2]) return best[3];
        return gridPos;
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

    void findPath(vec2 gridPos) {
        auto paths = std::vector<vec2>();
        vec2 playerPos = findClosestPlayer(gridPos);

        std::queue<vec2> q2do = std::queue<vec2>();

        processed.clear();

        q2do.push(gridPos);

        while(!q2do.empty()) {
            // get the next node
            auto curNode = new vec2(q2do.front());
            q2do.pop();

            // add first node with nullptr (so we know when to stop when backtracing)
            processed.emplace_back(new node_and_edge{*curNode, nullptr});

            //check tile above. Break if next to player
            auto nextNode = vec2(curNode->x, curNode->y-1);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile to the right. Break if next to player
            nextNode = vec2(curNode->x+1, curNode->y);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile below. Break if next to player
            nextNode = vec2(curNode->x, curNode->y+1);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;

            //check tile to the left. Break if next to player
            nextNode = vec2(curNode->x-1, curNode->y);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
        }
    }

// processes a cell. Returns true if the cell is the player cell.
// false otherwise.
    bool processCell(vec2* curNode, vec2 cell, vec2 playerPos, std::queue<vec2> & q2do) {
//        std::cout << "processing cell " << cell.x << ", " << cell.y << "\n";
//        std::cout << cell.x << ", " << cell.y << " is somehow " << (isOccupied(cell)? "" : "not ") << "occupied\n";
        if(cell.x == playerPos.x && cell.y == playerPos.y) {
            setPath(*curNode);
            std::cout << "found path\n";
            return true;
        }
        else if(isOccupied(cell) == 0) {

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
    int isOccupied(vec2 cell) {
        if(WorldSystem::is_wall(cell) || WorldSystem::is_hole(cell)) {
            std::cout << cell.x << ", " << cell.y << " is wall\n";
            return 1;
        }
        for (auto entity : ECS::registry<TilePosition>.entities) {
            TilePosition& entity_pos = ECS::registry<TilePosition>.get(entity);
            if (entity_pos.grid_pos.x == cell.x && entity_pos.grid_pos.y == cell.y) {
                if (ECS::registry<Tile>.has(entity)) {
                    auto& tile = ECS::registry<Tile>.get(entity);
                    if (tile.isBlocking == 1) {
//                        std::cout << cell.x << ", " << cell.y << " is a blocking tile\n";
                        return 1;
                    }
                } else {
//                    std::cout << cell.x << ", " << cell.y << " is a blocking character\n";
                    if(ECS::registry<Character>.has(entity))
                        return 2;
                    return 0;
                }
            }
        }
        return 0;
    }

    void setPath(vec2 goal) {
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

    std::vector<node_and_edge> thePath;
    std::vector<node_and_edge*> processed = std::vector<node_and_edge*>();

    Spell* GRAPPLE = new Spell(Spell::SpellEffect::GRAPPLE_TO, 1, 3, 0, 4, true, false, false);

private:
    bool in_up_position = true;
    bool no_paths = false;
};