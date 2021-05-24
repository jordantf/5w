#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "animation.hpp"
#include "health.hpp"

struct Health
{
	int curHealth;
	int maxHealth;

	static void createHealthBar(ECS::Entity& parent, const float healthPct, vec2 grid_position);
};

struct HealthBar {
    HealthBar(ShadedMeshRef barGraphic);

    vec2 grid_position;
	ShadedMeshRef barGraphic;
};
