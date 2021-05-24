// internal
#include "physics.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"
#include "turn.hpp"
#include "projectile.hpp"
#include "world.hpp"
#include "boomerang.hpp"
#include "hitbox.hpp"
#include "wall.hpp"
#include "ui.hpp"
#include "power.hpp"
#include "artifact.hpp"
#include "player.hpp"
#include <iostream>

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool collides(const Motion& motion1, const Motion& motion2)
{
	auto dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x/2.0f, 2.f) + std::pow(get_bounding_box(motion1).y/2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x/2.0f, 2.f) + std::pow(get_bounding_box(motion2).y/2.0f, 2.f));
	float r = max(other_r, my_r);
	if (dist_squared < r * r)
		return true;
	return false;
}

bool compare_float(float x, float y, float epsilon = 0.01f) {
	if (fabs(x - y) < epsilon)
		return true;
	return false;
}

bool withinRange(vec2 pos1, vec2 pos2, int range) {
    return (abs(pos1.x - pos2.x) + abs(pos1.y - pos2.y)) <= range;
}

void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    // Set entities screenPos based on tilePos
    // Right now is just basic placeholder
    // TODO: Add compatibility for movement
    for (auto& entity : ECS::registry<TilePosition>.entities)
    {
        auto& tilePos = ECS::registry<TilePosition>.get(entity);
        // Check here if entity has Moving component
        if (ECS::registry<Moving>.has(entity)) {
			auto& moving = ECS::registry<Moving>.get(entity);

			moving.oldPos += moving.step;

			tilePos.screen_pos = moving.oldPos * tilePos.scale;
			// Projectile movement
            if (ECS::registry<Projectile>.has(entity)) {
                auto& projectile = ECS::registry<Projectile>.get(entity);
                projectile.dist_traveled += projectile.speed;
                if (projectile.dist_traveled > projectile.max_dist) {
                    auto& turn = ECS::registry<Turn>.get(projectile.thrower_entity);
                    // Do an attack at final tile
                    if (projectile.max_pierce > 0)
                        WorldSystem::attack_target(projectile.thrower_entity, tilePos.grid_pos, false);
                    turn.currentAction = turn.IDLE;
                    if (ECS::registry<Player>.has(projectile.thrower_entity)) {
                        auto& player = ECS::registry<Player>.get(projectile.thrower_entity);
                        player.show_tiles();
                    }
                    // delete the projectile and its components
                    ECS::ContainerInterface::remove_all_components_of(entity);
                }

            // Character movement
            } else if (abs(moving.oldPos.x - tilePos.grid_pos.x) < 0.05f && abs(moving.oldPos.y - tilePos.grid_pos.y) < 0.05f) {
				ECS::registry<Moving>.remove(entity);
				if (ECS::registry<Turn>.has(entity)) {
				    auto& turn = ECS::registry<Turn>.get(entity);
				    tilePos.screen_pos = tilePos.grid_pos * tilePos.scale;
				    turn.currentAction = turn.IDLE;
                }
			}
		}
		else {
			// If not, just set screen position based on grid position since no movement.
			tilePos.screen_pos = tilePos.grid_pos * tilePos.scale;
		}
    }

    // spin the boomerangs
    for (auto& boomerang_entity : ECS::registry<Boomerang>.entities)
    {
        auto& boomerang = ECS::registry<Boomerang>.get(boomerang_entity);
        auto& hitbox = ECS::registry<Hitbox>.get(boomerang_entity);
        auto& tilePos = ECS::registry<TilePosition>.get(boomerang_entity);
        auto& moving = ECS::registry<Moving>.get(boomerang_entity);
        boomerang.angle += boomerang.spin;
        boomerang.decayTime -= elapsed_ms;
        if (boomerang.decayTime <= 0 || boomerang.spin < 0.05f) {
            auto& turn = ECS::registry<Turn>.get(boomerang.thrower);
            turn.currentAction = turn.IDLE;
            if (ECS::registry<Player>.has(boomerang.thrower)) {
                auto& player = ECS::registry<Player>.get(boomerang.thrower);
                UISystem::disableHighlights();
                player.show_tiles();
            }
            ECS::ContainerInterface::remove_all_components_of(boomerang_entity);
            break;
        }
        for (auto vertex : hitbox.vertices) {
            vec2 pos = vertex.position;
            pos += moving.oldPos;
            for (auto& t_e : ECS::registry<Tile>.entities) {
                auto &tile = ECS::registry<TilePosition>.get(t_e);
                if (tile.grid_pos.x == floor(pos.x + 0.5) && tile.grid_pos.y == floor(pos.y + 0.5)) {
                    if (ECS::registry<Tile>.get(t_e).isBlocking) {
                        vec2 collisionVec = moving.oldPos - tile.grid_pos;
                        if (abs(collisionVec.x) > abs(collisionVec.y)) {
                            moving.step = {-moving.step.x, moving.step.y};
                        } else {
                            moving.step = {moving.step.x, -moving.step.y};
                        }
                        tilePos.grid_pos = tilePos.grid_pos + moving.step * boomerang.speed;
                        std::cout << "Boomerang bounced off wall" << std::endl;
                        boomerang.spin -= 0.02f;
                        break;
                    } else {
                        vec2 boomerang_pos = tile.grid_pos;
                        bool already_hit = false;
                        for (vec2 pos : boomerang.entered) {
                            if (WorldSystem::isOccupied(pos, tile.grid_pos)) {
                                already_hit = true;
                                break;
                            }
                        }
                        if (!already_hit) {
                            boomerang.entered.push_back(boomerang_pos);
                            UISystem::addHighlight(boomerang_pos, 2, false);
                            WorldSystem::attack_target(boomerang.thrower, tile.grid_pos, false);
                        }
                    }
                }
            }
        }
        if (boomerang.angle > 2 * PI) {
            boomerang.angle = boomerang.angle - 2 * PI;
        }
    }


	for (auto& projectile_entity : ECS::registry<Projectile>.entities)
	{
	    auto& projectile = ECS::registry<Projectile>.get(projectile_entity);
	    projectile.pierce_counter -= elapsed_ms;
	    if (projectile.pierce_counter <= 0) {
            auto& moving = ECS::registry<Moving>.get(projectile_entity);
            // TODO: basic collision right now, so it just attacks every cell (x_x @ console)
            vec2 quantize_tile = {(int)lround(moving.oldPos.x), (int)lround(moving.oldPos.y)};
            if (WorldSystem::attack_target(projectile.thrower_entity, quantize_tile, false)) {
                projectile.max_pierce--;
            }
            if (projectile.max_pierce <= 0) {
                //destroy projectile
                auto& turn = ECS::registry<Turn>.get(projectile.thrower_entity);
                turn.currentAction = turn.IDLE;
                ECS::ContainerInterface::remove_all_components_of(projectile_entity);
            }
	        projectile.pierce_counter = projectile.pierce_interval;
	    }
	}

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
//	    std::cout << "in_debug_mode";
		for (auto& entity : ECS::registry<TilePosition>.entities)
		{
		    auto& tilePos = ECS::registry<TilePosition>.get(entity);
		    if (ECS::registry<Tile>.has(entity) && ECS::registry<Tile>.get(entity).isBlocking) {
                DebugSystem::createBox(tilePos.grid_pos * tilePos.scale, tilePos.scale);
		    } else if (tilePos.isSolid) {
                DebugSystem::createBox(tilePos.screen_pos, tilePos.scale);
            }
		}
		DebugSystem::showAllHitbox();
	}

	// Check for collisions between all moving entities
	auto& motion_container = ECS::registry<Motion>;
	// for (auto [i, motion_i] : enumerate(motion_container.components)) // in c++ 17 we will be able to do this instead of the next three lines
	for (unsigned int i=0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		ECS::Entity entity_i = motion_container.entities[i];
		for (unsigned int j=i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			ECS::Entity entity_j = motion_container.entities[j];

			if (collides(motion_i, motion_j))
			{
				// Create a collision event
				// Note, we are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity, hence, emplace_with_duplicates
				ECS::registry<Collision>.emplace_with_duplicates(entity_i, entity_j);
				ECS::registry<Collision>.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}
