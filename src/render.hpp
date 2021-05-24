#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include "animation.hpp"
#include "background.hpp"
#include "particles.hpp"
#include <random>
//#include "utils.hpp"

struct InstancedMesh;
struct ShadedMesh;

// OpenGL utilities
void gl_has_errors();

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem
{
public:
	// Initialize the window
	RenderSystem(GLFWwindow& window);

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	// Camera vars
	static vec2 camera_focus;

	void draw(vec2 window_size_in_game_units);
    void updateCamera(float elapsed_ms=0);
	static void moveCamera(vec2 pos);
	static void moveCamera(ECS::Entity en);
	// Expose the creating of visual representations to other systems
	static void createSprite(ShadedMesh& mesh_container, std::string texture_path, std::string shader_name, bool has_normal);
    static void createSprite(ShadedMesh& mesh_container, std::string texture_path, std::string shader_name);
	static void createColoredMesh(ShadedMesh& mesh_container, std::string shader_name);

	static vec2 get_cam_offset();

    static float cartesianTheta(vec2 pos);
	static vec2 cam_position;
    static vec2 camera_offset;
    ShadedMesh screen_sprite;
    ShadedMesh process_sprite;

private:
	// Initialize the screeen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water shader
	void initScreenTexture();

	// Internal drawing functions for each entity type
    void drawAnimatedMesh(ECS::Entity entity, const mat3& projection);
    void drawTileAnimatedMesh(ECS::Entity entity, const mat3& projection);
    void drawTexturedMesh(ECS::Entity entity, const mat3& projection);
    void drawTilesheetTile(ECS::Entity entity, const mat3 &projection);
    void drawTileTexturedMesh(ECS::Entity entity, const mat3& projection);
	void drawBackground(ECS::Entity entity, const mat3& projection);
	void drawToScreen(vec2 windowSize);
	void createAndDrawShadowMasks(vec2 windowSize, const mat3& projection);
	void drawShadowMasks(vec2 windowSize);
	void drawParticles(ECS::Entity e, const mat3& projection);

    // raycast utils
    std::vector<vec2> rayCastAllWallVertices(ECS::Entity light);
    vec2 rayCast(vec2 o, vec2 d, float max_dist);
    vec2 rayCastQ1(vec2 o, vec2 d, float max_dist);
    vec2 rayCastQ2(vec2 o, vec2 d, float max_dist);
    vec2 rayCastQ3(vec2 o, vec2 d, float max_dist);
    vec2 rayCastQ4(vec2 o, vec2 d, float max_dist);
    void createTriangles(const std::vector<vec2>& vs, const mat3& projection, ECS::Entity e);

    // health bar
    void drawHealthBars(ECS::Entity entity, const mat3& projection);

	// Window handle
	GLFWwindow& window;


	// Screen texture handles
	GLuint frame_buffer;
	GLResource<RENDER_BUFFER> depth_render_buffer_id;
	ECS::Entity screen_state_entity;

	// Shadow texture handles;
	GLuint shadowframe_buffer;
	GLuint rendered_shadow;
	GLuint preprocessing_buffer;
	GLuint rendered_screen;
	ShadedMesh shadow_sprite;


    // Camera vars
	float smoothing = 0.7; // 0 < 1

    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist; // number between 0..1

    void drawPostProcessing(vec2 windowSize);
};
