
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <iostream>

// internal
#include "common.hpp"
#include "world.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "physics.hpp"
#include "ai.hpp"
#include "animSystem.hpp"
#include "debug.hpp"
#include "scene.hpp"
#include "particles.hpp"

using Clock = std::chrono::high_resolution_clock;

const ivec2 window_size_in_px = {1200, 800 };
const vec2 window_size_in_game_units = { 1200, 800 };
// Note, here the window will show a width x height part of the game world, measured in px.
// You could also define a window to show 1.5 x 1 part of your game world, where the aspect ratio depends on your window size.

struct Description {
	std::string name;
	Description(const char* str) : name(str) {};
};

// Entry point
int main()
{
	// Initialize the main systems
	WorldSystem world(window_size_in_px);
	SceneSystem scene(window_size_in_px, &world);
	RenderSystem renderer(*scene.window);
	PhysicsSystem physics;
	AISystem ai;
	AnimSystem anims;

	scene.awake();

	while(!world.in_game && !world.is_over()){
        // Processes system messages, if this wasn't present the window would become unresponsive
        glfwPollEvents();
        renderer.draw(window_size_in_game_units);
	};

	// Set all states to default
	if (!world.is_over() && scene.scene == scene.BATTLE) {}
//	    world.restart();
	auto t = Clock::now();
	// Variable timestep loop
	while (!world.is_over())
	{

		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000.f;
		t = now;

		DebugSystem::clearDebugComponents();

		// Pause:
		if (WorldSystem::isGamePaused) {
		    // pause
        } else if (WorldSystem::currentTurnDelay >= WorldSystem::turnDelayMs) {
			ai.step(elapsed_ms, window_size_in_game_units);
			scene.step(elapsed_ms, window_size_in_game_units);
			physics.step(elapsed_ms, window_size_in_game_units);
			ParticleSystem::updateParticles(elapsed_ms);
			world.handle_collisions();
            if (scene.scene == scene.BATTLE)
			    world.set_mouse_highlight_pos(renderer.get_cam_offset());
            anims.step(elapsed_ms);
		} else {
		    WorldSystem::currentTurnDelay += elapsed_ms; // suspicious code..
			ParticleSystem::updateParticles(elapsed_ms);
            anims.step(elapsed_ms);
		}

		renderer.draw(window_size_in_game_units);
		if (scene.scene == scene.BATTLE || scene.scene == scene.EDITOR)
            renderer.updateCamera(elapsed_ms);
		if (world.resetCamera == true) {
		    renderer.cam_position = {0,0};
		    world.resetCamera = false;
		}
	}

	return EXIT_SUCCESS;
}
