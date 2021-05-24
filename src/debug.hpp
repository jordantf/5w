#pragma once

#include "common.hpp"
#include "hitbox.hpp"
#include "projectile.hpp"

// Data structure for pebble-specific information
namespace DebugSystem {
	extern bool in_debug_mode;

	// draw a red line for debugging purposes
	void createLine(vec2 position, vec2 size);
    void createLineAbs(vec2 position, vec2 scale);
    void createDottedLine(vec2 start, vec2 end, int n, int width);
    void createTriangle(vec2 v1, vec2 v2, vec2 v3);

	void createBox(vec2 position, vec2 size);

    void showAllHitbox();
    void showHitbox(Projectile projectile, Hitbox hitbox, TilePosition tilePosition);

	// Removes all debugging graphics in ECS, called at every iteration of the game loop
	void clearDebugComponents();
};
