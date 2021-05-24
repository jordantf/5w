//
// Created by Roark on 2021-02-18.
//

#ifndef ENEMY_AI_HPP
#define ENEMY_AI_HPP

#include "ai.hpp"
#include "world.hpp"
#include <memory>


class EnemyAI {
public:
    EnemyAI(ECS::Entity enemy_p, WorldSystem * world_p) {
        enemy = enemy_p;
        world = world_p;
    }

/* TODO: since the enemy needs to know about the map,
    should the area map and entity locations be passed in here, or should EnemyAI know about world.cpp? */
    virtual void beginTurn() = 0;

    ~EnemyAI() = default;
protected:
    ECS::Entity enemy;
    WorldSystem* world;
};

typedef std::unique_ptr<EnemyAI> EnemyAIContainer;

#endif //ENEMY_AI_HPP
