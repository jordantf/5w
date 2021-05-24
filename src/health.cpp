// internal
#include "turn.hpp"
#include "render.hpp"
#include "health.hpp"


// TODO: For debug purposes for now
#include <iostream>
#include <utility>

void Health::createHealthBar(ECS::Entity& parent, const float healthPct, vec2 grid_position) {

    if(healthPct <= 0.f) return;
    ShadedMesh& resource = cache_resource(std::to_string(healthPct));

    if (resource.effect.program.resource == 0) {
        constexpr float borderWidth = 0.05f;
        constexpr float z = -0.1f;
        constexpr vec3 lowHealth = {0.6, 0.2, 0.2};
        constexpr vec3 midHealth = {1.0, 0.8, 0.2};
        constexpr vec3 highHealth = {0.1, 0.95, 0.4};
        constexpr float mid = 0.5f;
        vec3 colour = { 0.3,0.3,0.3 };
        vec3 hcolour = { 0.8, 0.2, 0.2 };

        if(healthPct > mid) { // blend between mid and high
            hcolour = (1.0f-healthPct) * midHealth + (healthPct - mid) * highHealth;
        } else {
            hcolour = (mid - healthPct) * lowHealth + (healthPct) * midHealth;
        }

        // Corner points
        ColoredVertex v;
        v.position = {-0.5,-0.75,z};
        v.color = colour;
        resource.mesh.vertices.push_back(v);
        v.position = { -0.5,-0.5,z };
        v.color = colour;
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5,-0.5,z };
        v.color = colour;
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5,-0.75,z };
        v.color = colour;
        resource.mesh.vertices.push_back(v);

        float totalWidth = 1.0 - borderWidth;

        v.position = {-0.5+borderWidth, -0.75+borderWidth, z};
        v.color = hcolour;
        resource.mesh.vertices.push_back(v);
        v.position = {-0.5+borderWidth, -0.5-borderWidth, z};
        v.color = hcolour;
        resource.mesh.vertices.push_back(v);
        v.position = {-0.5+totalWidth*healthPct, -0.5-borderWidth, z};
        v.color = hcolour;
        resource.mesh.vertices.push_back(v);
        v.position = {-0.5+totalWidth*healthPct, -0.75+borderWidth, z};
        v.color = hcolour;
        resource.mesh.vertices.push_back(v);

        // Two triangles
        resource.mesh.vertex_indices.push_back(0);
        resource.mesh.vertex_indices.push_back(1);
        resource.mesh.vertex_indices.push_back(3);
        resource.mesh.vertex_indices.push_back(1);
        resource.mesh.vertex_indices.push_back(2);
        resource.mesh.vertex_indices.push_back(3);

        resource.mesh.vertex_indices.push_back(4);
        resource.mesh.vertex_indices.push_back(5);
        resource.mesh.vertex_indices.push_back(6);
        resource.mesh.vertex_indices.push_back(6);
        resource.mesh.vertex_indices.push_back(7);
        resource.mesh.vertex_indices.push_back(4);

        RenderSystem::createColoredMesh(resource, "colored_mesh");
    }

    // store ShadedMeshRef
    ShadedMeshRef meshRef = ShadedMeshRef(resource);
    HealthBar &hb = ECS::registry<HealthBar>.emplace(parent, meshRef);
//    HealthBar &hb = ECS::registry<HealthBar>.get(parent);
    hb.grid_position = grid_position;
}

HealthBar::HealthBar(ShadedMeshRef barGraphic) : barGraphic(barGraphic) {

}
