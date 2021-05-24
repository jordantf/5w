#pragma once

#include "common.hpp"
#include "Enemy3AI.hpp"
#include "tiny_ecs.hpp"
#include "animSystem.hpp"
#include "turn.hpp"
#include "tile.hpp"
#include "ui.hpp"

//std
#include <queue>
#include <iostream>



// Boss AI

class BossAI : public Enemy3AI {
public:
    BossAI(ECS::Entity enemy_p, WorldSystem *world_p) : Enemy3AI(enemy_p, world_p, true) {
        enemy = enemy_p;
        world = world_p;
    }

    void beginTurn() override {
        if (!ECS::registry<HurtQueue>.components.empty() || WorldSystem::stepping_turn) return;
        assert(ECS::registry<Turn>.has(enemy)); // Enemy entity has to have the current turn to begin turn!!
        Turn& turn = ECS::registry<Turn>.get(enemy);
        TilePosition& tile_position = ECS::registry<TilePosition>.get(enemy);
        vec2 closest_player_pos = findClosestPlayer(tile_position.grid_pos);
        if (state == DORMANT) {
            if (inAttackRange(closest_player_pos, tile_position.grid_pos, dormancy_detection_range)) {
                state = RUNNING;
                max_attack_range = 5;
                return;
            } else return endTurn();
        } else if (state == RUNNING) {
            kite(closest_player_pos, tile_position, turn);
            return;
        } else if (state == ENRAGE1) {
            barrage(closest_player_pos, tile_position, turn);
            return;
        } else if (state == CHARGING) {
            chargeAttack(closest_player_pos, tile_position, turn);
            return;
        } else if (state == ENRAGE2) {
            miniCharge(closest_player_pos, tile_position, turn);
            return;
        }

        return endTurn();
    }

    void endTurn() {
        thePath.clear();
        for(auto & i : processed) {
            free(i);
        }
        processed.clear();
        world->step_turn();
        no_paths = false;
    }

    void dropArtifact(int area, TilePosition& pos) {
        vec2 position = pos.grid_pos;
        switch(area) {
            case 1: Artifact::createArtifact(position, 1, "typewriter.png");
                break;
            case 2: Artifact::createArtifact(position, 2, "teapot.png");
                break;
            case 3:
                break;
            default: break;
        }
    }

    void kite(vec2 player_pos, TilePosition& tile_pos, Turn& t) {
        auto& c = ECS::registry<Character>.get(enemy);
        if (c.currentHealth/c.maxHealth <= 0.75) {
            c.meleeDamage = 4;
            c.rangedDamage = 40;
            c.max_attacks = 4;
            t.attacks = 4;
            max_attack_range = 1;
            chargeCounter = 0;
            changeState(ENRAGE1);
            dropArtifact(1, tile_pos);
            return;
        }
        if (t.attacks > 0) {
            if (inAttackRange(player_pos, tile_pos.grid_pos, max_attack_range)) {
                t.attacks --;
                Projectile::createProjectile(projectile_texture, tile_pos.grid_pos, player_pos,
                                             0.3f, 1, -1, enemy, -1);
                t.currentAction = t.ATTACKING;
                return;
            } else if (t.movement > 0 && !no_paths) {
                if(thePath.empty()) findPath(tile_pos.grid_pos);
                if (thePath.empty()) {
                    no_paths = true;
                    return;
                }
                world->move_character(t,tile_pos,thePath.front().vertex);
                thePath.erase(thePath.begin());
                return;
            }
        }
        if (t.movement > 0) {
            if (runAway(player_pos, tile_pos, t)) {
                return;
            } else {
                return endTurn();
            }
        }
        return endTurn();
    }

    void barrage(vec2 player_pos, TilePosition &tile_pos, Turn &t) {
        auto& c = ECS::registry<Character>.get(enemy);
        if (c.currentHealth/c.maxHealth <= 0.30) {
            c.meleeDamage = 20;
            c.rangedDamage = 20;
            c.max_attacks = 1;
            t.attacks = 1;
            max_attack_range = 4;
            changeState(ENRAGE2);
            dropArtifact(2, tile_pos);
            return;
        }
        if (chargeCounter >= chargeTime) {
            changeState(CHARGING);
            chargeCounter = 0;
            return endTurn();
        }
        if (t.attacks > 0) {
            if (inAttackRange(player_pos, tile_pos.grid_pos, max_attack_range)) {
                t.currentAction = t.ATTACKING;
                world->attack_target(enemy, player_pos, true);
            } else if (t.movement > 0 && !no_paths) {
                if(thePath.empty()) findPath(tile_pos.grid_pos);
                if (thePath.empty()) {
                    no_paths = true;
                    return;
                }
                world->move_character(t,tile_pos,thePath.front().vertex);
                thePath.erase(thePath.begin());
                return;
            }
        } if (t.movement > 0 && !no_paths) {
            if(thePath.empty()) findPath(tile_pos.grid_pos);
            if (thePath.empty()) {
                no_paths = true;
                return;
            }
            world->move_character(t,tile_pos,thePath.front().vertex);
            thePath.erase(thePath.begin());
            return;
        }
        chargeCounter ++;
        return endTurn();
    }

    bool runAway(vec2 player_pos, TilePosition& tile_pos, Turn& t) {
        vec2 dest_pos;
        vec2 pos = tile_pos.grid_pos;
        vec2 toVec = tile_pos.grid_pos - player_pos;
        dest_pos = pos + vec2(toVec.x > 0 ? 1 : -1, 0);

        if (abs(toVec.x) > abs(toVec.y)) {
            // Try running in X axis
            dest_pos = pos + vec2(toVec.x > 0 ? 1 : -1, 0);
            if (world->move_character(t, tile_pos, dest_pos))
                return true;
            else {
                // Try running in less optimal Y axis
                dest_pos = pos + vec2(0, toVec.y > 0 ? 1 : -1);
                if (world->move_character(t, tile_pos, dest_pos)) {
                    return true;
                } else {
                    dest_pos = pos + vec2(0, toVec.y > 0 ? -1 : 1);
                    if (length(dest_pos - player_pos) < length(toVec))
                        return false;
                    else return world->move_character(t, tile_pos, dest_pos);
                }
            }
        } else {
            // Try running in Y axis
            dest_pos = pos + vec2(0, toVec.y > 0 ? 1 : -1);
            if (world->move_character(t, tile_pos, dest_pos)) {
                return true;
            } else {
                // Try running in less optimal X axis
                dest_pos = pos + vec2(toVec.x > 0 ? 1 : -1, 0);
                if (world->move_character(t, tile_pos, dest_pos)) {
                    return true;
                } else {
                    dest_pos = pos + vec2(toVec.x > 0 ? -1 : 1, 0);
                    if (length(dest_pos - player_pos) < length(toVec))
                        return false;
                    else return world->move_character(t, tile_pos, dest_pos);
                }
            }
        }
    }

    void chargeAttack(vec2 player_pos, TilePosition &tile_pos, Turn &t) {
        if (chargeCounter >= chargeTime) {

            // Big Boom
            if (WorldSystem::hasLoS(tile_pos.grid_pos, player_pos)) {
                t.currentAction = t.ATTACKING;
                t.attacks = 0;
                t.movement = 0;
                Projectile::createProjectile(blast, tile_pos.grid_pos, player_pos,
                                             0.03f, 1, -1, enemy, -1);
            }
            ParticleSystem::createParticleBurst("particles/destroying_energy.png",60, 200.f, 5000.f,
                                                100.f * tile_pos.grid_pos - 50.f, 100.f * tile_pos.grid_pos + vec2(50, 50), {2000.f, 3000.f},
                                                0, 2.0, {0.f, 10.f}, 0.93, 20.0, 50.f, 5.f, {0.5, 1.0}, 0.4);

            chargeCounter = 0;
            changeState(ENRAGE1);
            return;
        }
        chargeCounter++;
        if (chargeCounter == chargeTime) {
            ECS::registry<TempText>.emplace(UISystem::showText(
                    "Charge attack ready!", {400, 400}, 0.4f), 4000.f);
        }
        endTurn();
        return;
    }

    void miniCharge(vec2 player_pos, TilePosition &tile_pos, Turn &t) {
        if (charged) {
            if (t.attacks > 0 && inAttackRange(player_pos, tile_pos.grid_pos, max_attack_range)) {
                t.attacks --;
                t.currentAction = t.ATTACKING;
                charged = false;
                Projectile::createProjectile(mini_blast, tile_pos.grid_pos, player_pos,
                                             0.09f, 1, -1, enemy, -1);
                return;
            } else if (t.movement > 0 && !no_paths) {
                charged = false;
                ECS::registry<TempText>.emplace(UISystem::showText(
                        "Charge lost!", {400, 400}, 0.4f), 2000.f);
                if(thePath.empty()) findPath(tile_pos.grid_pos);
                if (thePath.empty()) {
                    no_paths = true;
                    return;
                }
                world->move_character(t,tile_pos,thePath.front().vertex);
                thePath.erase(thePath.begin());
                return;
            } else {
                endTurn();
                return;
            }
        } else {
            if(!thePath.empty() && t.movement > 0) {
                world->move_character(t, tile_pos, thePath.front().vertex);
                thePath.erase(thePath.begin());
                return;
            }
            charged = true;
            ECS::registry<TempText>.emplace(UISystem::showText(
                    "Ernest has charged \nhis attack!", {400, 400}, 0.5f), 4000.f);
            endTurn();
            return;
        }
    }

    void findPath(vec2 gridPos) override {
        auto paths = std::vector<vec2>();
        vec2 playerPos = findClosestPlayer(gridPos);

        std::queue<vec2> q2do = std::queue<vec2>();

        processed.clear();

        q2do.push(gridPos);

        while(!q2do.empty()) {
            auto curNode = new vec2(q2do.front());
            q2do.pop();
            processed.emplace_back(new node_and_edge{*curNode, nullptr});
            auto nextNode = vec2(curNode->x, curNode->y-1);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
            nextNode = vec2(curNode->x+1, curNode->y);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
            nextNode = vec2(curNode->x, curNode->y+1);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
            nextNode = vec2(curNode->x-1, curNode->y);
            if(processCell(curNode, nextNode, playerPos, q2do)) return;
        }
    }

    bool processCell(vec2* curNode, vec2 cell, vec2 playerPos, std::queue<vec2> & q2do) override {
        if(inAttackRange(playerPos, *curNode, max_attack_range)) {
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

    void setPath(vec2 goal) {
        node_and_edge * cur_node;
        for(auto & i : processed) {
            if(i->vertex.x == goal.x && i->vertex.y == goal.y) {
                cur_node = i;
                break;
            }
        }
        assert(thePath.empty());
        while(cur_node->link != NULL && cur_node->link != nullptr) {
            thePath.emplace(thePath.begin(), *cur_node);
            cur_node = cur_node->link;
        }
    }


    int max_detection_range = 10; // Max range of player detection
    int dormancy_detection_range = 7;
    int max_attack_range = 1; // Max range of attack


    enum STATE {DORMANT, RUNNING, ENRAGE1, CHARGING, CHASING, ENRAGE2} state = DORMANT;

    void changeState(STATE newState) {
        state = newState;
        Animation* newAnim;
        switch (newState) {
            case DORMANT: case RUNNING:
                newAnim = setAnim("shopkeeper_1", "shopkeeper_1.png",
                                  { 16, 16 }, { 64, 16 },
                                  4, 12, true, false, false, nullptr);
                break;
            case ENRAGE1: case CHASING:
                newAnim = setAnim("shopkeeper_2", "shopkeeper_2.png",
                                  { 16, 16 }, { 64, 16 },
                                  4, 12, true, false, false, nullptr);
                break;
            case CHARGING:
                newAnim = setAnim("shopkeeper_3", "shopkeeper_3.png",
                                  { 16, 16 }, { 64, 16 },
                                  4, 12, true, false, false, nullptr);
                ECS::registry<TempText>.emplace(UISystem::showText(
                        "Charging attack! \nGet out of his line of sight!", {400, 400}, 0.4f), 4000.f);
                if(ECS::registry<EnemyAIContainer>.size() < 10) {

                    auto *wraithAnim = setAnim("wraith_idle", "wraith_idle.png", {16, 16},
                                               {16, 32}, 2, 2, true, false, false, nullptr);
                    world->spawn_character("wraith" + std::to_string(minionCount), ECS::registry<TilePosition>.get(enemy).grid_pos + vec2(0, 1), 6, Animated, wraithAnim, 6, 30,
                                    30,5);

                }
                break;
            case ENRAGE2:
                newAnim = setAnim("shopkeeper_4", "shopkeeper_4.png",
                                  { 16, 16 }, { 64, 16 },
                                  4, 10, true, false, false, nullptr);
                break;
        }
        AnimSystem::switchAnim(enemy, *newAnim);
    }

    bool inAttackRange(vec2 closestPlayerGridPos, vec2 gridPos, int range) {
        int dist = abs(closestPlayerGridPos.x - gridPos.x) + abs(closestPlayerGridPos.y - gridPos.y);
        if (needs_los && !WorldSystem::hasLoS(gridPos, closestPlayerGridPos)) return false;
        return dist <= range;
    }

protected:
    std::vector<node_and_edge> thePath;
    std::vector<node_and_edge*> processed = std::vector<node_and_edge*>();

    Spell* GRAPPLE = new Spell(Spell::SpellEffect::GRAPPLE_TO, 1, 3, 0, 4, true, false, false);

private:
    bool in_up_position = true;
    bool no_paths = false;
    bool needs_los;
    int chargeTime = 2;
    int chargeCounter = 0;
    bool charged = false;
    int minionCount = 0;
    std::string projectile_texture = "cashthrow";
    std::string blast = "blast";
    std::string mini_blast = "miniblast";
};