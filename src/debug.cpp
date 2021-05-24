// Header
#include "debug.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"

#include <cmath>
#include <iostream>

#include "render_components.hpp"

namespace DebugSystem
{
	void createLine(vec2 position, vec2 scale) {
		auto entity = ECS::Entity();

		std::string key = "thick_line";
		ShadedMesh& resource = cache_resource(key);
		if (resource.effect.program.resource == 0) {
			// create a procedural circle
			constexpr float z = -0.1f;
			vec3 red = { 0.8,0.1,0.1 };

			// Corner points
			ColoredVertex v;
			v.position = {-0.5,-0.5,z};
			v.color = red;
			resource.mesh.vertices.push_back(v);
			v.position = { -0.5,0.5,z };
			v.color = red;
			resource.mesh.vertices.push_back(v);
			v.position = { 0.5,0.5,z };
			v.color = red;
			resource.mesh.vertices.push_back(v);
			v.position = { 0.5,-0.5,z };
			v.color = red;
			resource.mesh.vertices.push_back(v);

			// Two triangles
			resource.mesh.vertex_indices.push_back(0);
			resource.mesh.vertex_indices.push_back(1);
			resource.mesh.vertex_indices.push_back(3);
			resource.mesh.vertex_indices.push_back(1);
			resource.mesh.vertex_indices.push_back(2);
			resource.mesh.vertex_indices.push_back(3);

			RenderSystem::createColoredMesh(resource, "colored_mesh");
		}

		// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
		ECS::registry<ShadedMeshRef>.emplace(entity, resource);

		// Create motion
		auto& motion = ECS::registry<TilePosition>.emplace(entity);
//		motion.angle = 0.f;
//		motion.velocity = { 0, 0 };
		motion.screen_pos = position;
		motion.scale = scale;

		ECS::registry<DebugComponent>.emplace(entity);
	}

	void createTriangle(vec2 v1, vec2 v2, vec2 v3) {
        auto entity = ECS::Entity();

        std::string key = "thick_line";
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0) {
            // create a procedural circle
            constexpr float z = -0.1f;
            vec3 red = { 0.3,0.3,0.7 };

            // Corner points
            ColoredVertex v;
            v.position = {-0.5,-0.5,z};
            v.color = red;
            resource.mesh.vertices.push_back(v);
            v.position = { -0.5,0.5,z };
            v.color = red;
            resource.mesh.vertices.push_back(v);
            v.position = { 0.5,0.5,z };
            v.color = red;

            // Two triangles
            resource.mesh.vertex_indices.push_back(0);
            resource.mesh.vertex_indices.push_back(1);
            resource.mesh.vertex_indices.push_back(2);

            RenderSystem::createColoredMesh(resource, "colored_mesh");
        }

        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
        ECS::registry<ShadedMeshRef>.emplace(entity, resource);

        // Create motion
        auto& motion = ECS::registry<TilePosition>.emplace(entity);
//		motion.angle = 0.f;
//		motion.velocity = { 0, 0 };
        motion.screen_pos = {800, 500};
        motion.scale = vec2(3, 3);

        ECS::registry<DebugComponent>.emplace(entity);
	}

    void createDottedLine(vec2 start, vec2 end, int n, int width) {
        for(int i = 0; i <= n; i++) {
            float w = float(i)/float(n);

            vec2 interp = w*end + (1-w)*start;

            createLineAbs(interp, vec2(width, width));
        }
    }

    void createLineAbs(vec2 position, vec2 scale) {
        auto entity = ECS::Entity();

        std::string key = "thick_line";
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0) {
            // create a procedural circle
            constexpr float z = -0.1f;
            vec3 red = { 0.8,0.1,0.1 };

            // Corner points
            ColoredVertex v;
            v.position = {-0.5,-0.5,z};
            v.color = red;
            resource.mesh.vertices.push_back(v);
            v.position = { -0.5,0.5,z };
            v.color = red;
            resource.mesh.vertices.push_back(v);
            v.position = { 0.5,0.5,z };
            v.color = red;
            resource.mesh.vertices.push_back(v);
            v.position = { 0.5,-0.5,z };
            v.color = red;
            resource.mesh.vertices.push_back(v);

            // Two triangles
            resource.mesh.vertex_indices.push_back(0);
            resource.mesh.vertex_indices.push_back(1);
            resource.mesh.vertex_indices.push_back(3);
            resource.mesh.vertex_indices.push_back(1);
            resource.mesh.vertex_indices.push_back(2);
            resource.mesh.vertex_indices.push_back(3);

            RenderSystem::createColoredMesh(resource, "colored_mesh");
        }

        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
        ECS::registry<ShadedMeshRef>.emplace(entity, resource);

        // Create motion
        auto& motion = ECS::registry<Motion>.emplace(entity);
		motion.angle = 0.f;
		motion.velocity = { 0, 0 };
        motion.position = position;
        motion.scale = scale;

        ECS::registry<DebugComponent>.emplace(entity);
    }
	
	void createBox(vec2 position, vec2 size) {
		vec2 vertical_line = {5, size.y};
		vec2 horizontal_line = {size.x, 5};
		createLine(position + vec2(size.x/2.0, 0), vertical_line);
		createLine(position - vec2(size.x/2.0, 0), vertical_line);
		createLine(position + vec2(0, size.y/2.0), horizontal_line);
		createLine(position - vec2(0, size.y/2.0), horizontal_line);
	}

	void showAllHitbox() {
	    auto& projectiles = ECS::registry<Projectile>;
	    for (unsigned int a = 0; a < projectiles.size(); a++) {
	        auto& boomer = projectiles.components[a];
	        auto& entity = projectiles.entities[a];
	        auto& tilePosition = ECS::registry<TilePosition>.get(entity);
	        auto& hbox = ECS::registry<Hitbox>.get(entity);
            showHitbox(boomer, hbox, tilePosition);
	    }
	}

	void showHitbox(Projectile projectile, Hitbox hitbox, TilePosition tilePosition) {
        // show hitbox here, probably needs the hitbox entity instead but poo

        auto& vertices = hitbox.vertices;
        // draw a dot on all vertices
        for(unsigned int k = 0; k < vertices.size(); k++) {
            auto & v = vertices[k];
            auto a = projectile.rotation;
            // scale vertex position
            mat3 scale = {{tilePosition.scale.x, 0, 0},{0, tilePosition.scale.y, 0},{0, 0, 1}};
            // rotate vertex position
            mat3 rot = {{cos(a), sin(a), 0}, {-sin(a), cos(a), 0}, {0, 0, 1}};

            vec3 new_pos = rot * scale * v.position;

            DebugSystem::createLine(vec2(tilePosition.screen_pos.x + new_pos.x, tilePosition.screen_pos.y + new_pos.y), tilePosition.scale*0.1f);
        }
	}

	void clearDebugComponents() {
		// Clear old debugging visualizations
		while (ECS::registry<DebugComponent>.entities.size() > 0) {
			ECS::ContainerInterface::remove_all_components_of(ECS::registry<DebugComponent>.entities.back());
        }
	}

	bool in_debug_mode = false;
}
