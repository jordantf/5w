// internal
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "turn.hpp"
#include "projectile.hpp"
#include "text.hpp"
#include "health.hpp"
#include "ui.hpp"
//#include "animation.hpp"
#include "utils.hpp"
#include "boomerang.hpp"
#include "raycast.hpp"

#include <iostream>
vec2 RenderSystem::camera_focus;
vec2 RenderSystem::cam_position = { 0, 0 };

vec2 RenderSystem::get_cam_offset() {
    return cam_position - camera_offset;
}

void RenderSystem::drawAnimatedMesh(ECS::Entity entity, const mat3& projection) {
    std::cout << "stub: draw animated mesh\n";
}

void RenderSystem::drawParticles(ECS::Entity e, const mat3& projection) {
    // Adapted from OpenGL tutorial 18
    // https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/

    auto& pe = ECS::registry<ParticleEmitter>.get(e);

    static GLfloat* particles_positions = new GLfloat[ParticleEmitter::MaxParticles * 3];
    static GLfloat* particles_opacity = new GLfloat[ParticleEmitter::MaxParticles];

    int count = 0;
    for(int i = 0; i < ParticleEmitter::MaxParticles; i++) {
        auto& p = pe.particleContainer[i];
        if(p.life > 0) {
            particles_positions[3*count+0] = p.motion.position.x;
            particles_positions[3*count+1] = p.motion.position.y;
            particles_positions[3*count+2] = p.motion.scale.x;
            particles_opacity[count] = p.opacity;
            count++;
        }
    }

    // If there are no particles and the emitter was a burst, then delete it
    if(count == 0) {
        if(!pe.persistent) ECS::registry<ParticleEmitter>.remove(e);
        return;
    }

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, ParticleEmitter::MaxParticles * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(GLfloat) * 3, particles_positions);

    glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_opacity_buffer);
    glBufferData(GL_ARRAY_BUFFER, ParticleEmitter::MaxParticles * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(GLfloat), particles_opacity);

    Transform transform;
    vec2 offset = cam_position - camera_offset;
    transform.translate(-offset);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shader
    glUseProgram(ParticleSystem::shaderContainer.effect.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pe.texture.reference_to_cache->texture.texture_id);

    GLuint tex_uloc  = glGetUniformLocation(ParticleSystem::shaderContainer.effect.program, "sampler0");
    glUniform1i(tex_uloc, 0);

    static const GLfloat g_vertex_buffer_data[] = {
            -0.5, -0.5, 0.0f,
            0.5, -0.5, 0.0f,
            -0.5,  0.5, 0.0f,
            0.5,  0.5, 0.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // load particle shader
    GLint transform_uloc = glGetUniformLocation(ParticleSystem::shaderContainer.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(ParticleSystem::shaderContainer.effect.program, "projection");

    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);


    // 1rst attribute buffer : vertices
//    GLint in_position_loc = glGetAttribLocation(ParticleSystem::shaderContainer.effect.program, "in_position");
//    glBindBuffer(GL_ARRAY_BUFFER, pe.texture.reference_to_cache->mesh.vbo);
//    glBindBuffer(GL_ARRAY_BUFFER, resource.mesh.vbo);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pe.texture.reference_to_cache->mesh.ibo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_position_buffer);
    glVertexAttribPointer(
            1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            3,                                // size : x + y + scale => 2
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            reinterpret_cast<void*>(0)        // array buffer offset
    );


    // 3rd attribute buffer : particle opacities
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_opacity_buffer);
    glVertexAttribPointer(
            2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            1,                                // size : opacity => 1
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            reinterpret_cast<void*>(0)        // array buffer offset
    );

    // These functions are specific to glDrawArrays*Instanced*.
    // The first parameter is the attribute buffer we're talking about.
    // The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
    // http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
    glVertexAttribDivisor(2, 1); // opacities : one per quad

    // Draw the particles !
    // This draws many times a small triangle_strip (which looks like a quad).
    // This is equivalent to :
    // for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4),
    // but faster.
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void RenderSystem::drawTileAnimatedMesh(ECS::Entity entity, const mat3& projection) {
    auto& tile_position = ECS::registry<TilePosition>.get(entity);
    // auto& motion = ECS::registry<Motion>.get(entity);
    auto& anim = ECS::registry<Animation>.get(entity);

    auto& texmesh = *anim.shadedMesh;
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    vec2 offset = cam_position - camera_offset;

    // offset so that top left occupies cell, with no shader (jk unused)
    vec2 spritesheetoffset = (anim.cols_rows/2.f)-0.5f;

    vec2 sprite_scale = { anim.spriteDims.x/16.f, anim.spriteDims.y/16.f};

    vec2 sprite_offset = {0, 0};//sprite_scale * (anim.spriteDims - vec2(16,16)) * 2.5f; // 2.5 meaning divide by difference by 4, then x 10 for pixel size
//    transform.scale({anim.flipped_x? -1:1 , anim.flipped_y? -1:1});
    transform.translate(tile_position.screen_pos - offset - sprite_offset - 7.f * anim.originOffset * (anim.flipped_x ? -1.f:1.f)); // I have no idea why 7.f works.

//    std::cout << "anim.texDims/anim.spriteDims: " << (anim.texDims/anim.spriteDims).x << ", " << (anim.texDims/anim.spriteDims).y << "\n";
    transform.scale(tile_position.scale * sprite_scale * anim.texDims/anim.spriteDims);
    //	transform.scale(motion.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
    GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
    if (in_texcoord_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
    }
    else if (in_color_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
	}
    else
    {
        throw std::runtime_error("This type of entity is not yet supported");
    }
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();
    // Pass in uniforms to animated shader
    GLint curFrame_uloc = glGetUniformLocation(texmesh.effect.program, "curFrame");
    glUniform1i(curFrame_uloc, anim.curFrame);
    GLint spriteSize_uloc = glGetUniformLocation(texmesh.effect.program, "spriteSize");
    glUniform2fv(spriteSize_uloc, 1, (float*)&anim.spriteDims);

    vec2 flip ={anim.flipped_x? -1:1 , anim.flipped_y? -1:1};
    GLint flip_uloc = glGetUniformLocation(texmesh.effect.program, "flipVec");
    glUniform2fv(flip_uloc, 1, (float*)&flip);


    GLint rc_uloc = glGetUniformLocation(texmesh.effect.program, "cols_rows");
    glUniform2fv(rc_uloc, 1, (float*)&anim.cols_rows);
    gl_has_errors();

    // Pass in coords of current frame to shader
    vec2 curFrameCoords;
    curFrameCoords.x = anim.curFrame % (int) anim.cols_rows.x;
    if(curFrameCoords.x == 0) curFrameCoords.x = anim.cols_rows.x;
    curFrameCoords.y = ceil(anim.curFrame / anim.cols_rows.x);
    curFrameCoords += anim.originOffset/16.f; // 16 pixels is the width of a tile
    GLint cfc_uloc = glGetUniformLocation(texmesh.effect.program, "curFrameCoords");
    glUniform2fv(cfc_uloc, 1, (float*)&curFrameCoords);

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::drawTilesheetTile(ECS::Entity entity, const mat3& projection) {
    auto& tile_position = ECS::registry<TilePosition>.get(entity);
    // auto& motion = ECS::registry<Motion>.get(entity);

    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    vec2 offset = cam_position - camera_offset;
    transform.translate(tile_position.screen_pos - offset);

//    std::cout << "anim.texDims/anim.spriteDims: " << (anim.texDims/anim.spriteDims).x << ", " << (anim.texDims/anim.spriteDims).y << "\n";
    transform.scale(tile_position.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
    GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");

    if (in_texcoord_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);

        // bind normals to texture slot 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texmesh.normal.texture_id);
        GLuint loc = glGetUniformLocation(texmesh.effect.program, "normals");
        glUniform1i(loc, 1);
    }
    else {
        throw std::runtime_error("This type of entity is not yet supported");
    }
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();
    // Pass in uniforms to tileset shader
    vec3 col = vec3(0, 0, 0);
    if(tile_position.lit) {
        // higlight cell colour
        col = vec3(0.2, 0.3, 0.6);
    }

    vec3 l_pos[20];
    int l_size = ECS::registry<LightSource>.entities.size();
    GLint light_size_uloc = glGetUniformLocation(texmesh.effect.program, "num_lights");
    glUniform1iv(light_size_uloc, 1, (int*)&l_size);
    gl_has_errors();
    assert (l_size <= 20);
    vec2 mypos = tile_position.screen_pos;

    for (int i = 0; i < l_size; i++) {
        auto& le = ECS::registry<LightSource>.entities[i];
        vec2 tpos = ECS::registry<TilePosition>.get(le).screen_pos;
        vec2 final = tpos - mypos;
        float ls = ECS::registry<LightSource>.get(le).strength;
        l_pos[i] = vec3(final.x, final.y, ls);
    }

    GLint light_position_uloc = glGetUniformLocation(texmesh.effect.program, "light_pos");
    glUniform3fv(light_position_uloc, 20, (float*)l_pos);
    gl_has_errors();

    // add highlight to cell
    GLint lit_uloc = glGetUniformLocation(texmesh.effect.program, "highlight_col");
    glUniform3fv(lit_uloc, 1, (float*)&col);
    gl_has_errors();

    vec2 tilesetSize, tilesheet_texcoord;
    if (ECS::registry<Door>.has(entity)) {
        auto& door = ECS::registry<Door>.get(entity);
        tilesetSize = {8, 1};
        tilesheet_texcoord = {(door.door_type * 2 + (door.horizontal?
                              (door.opened? 1 : 0) :
                              (door.opened? 0 : 1))) * 0.125, 0};
    } else {
        auto& tile = ECS::registry<TilesheetTile>.get(entity);
        tilesetSize = {6, 6};
        tilesheet_texcoord = {(tile.tilesheet%6)/tilesetSize.x, (floor(tile.tilesheet/6))/tilesetSize.y};
    }

    GLint tilesheet_uloc = glGetUniformLocation(texmesh.effect.program, "tilesetCoords");
    glUniform2fv(tilesheet_uloc, 1, (float*)&tilesheet_texcoord);
    gl_has_errors();

    GLint tilesize_uloc = glGetUniformLocation(texmesh.effect.program, "tilesetSize");
    glUniform2fv(tilesize_uloc, 1, (float*)&tilesetSize);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);

    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}


void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3& projection)
{
    auto& motion = ECS::registry<Motion>.get(entity);
    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    transform.translate(motion.position);
    transform.scale(motion.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
    GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
    if (in_texcoord_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
    }
    else if (in_color_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
    }
    else
    {
        throw std::runtime_error("This type of entity is not yet supported");
    }
    gl_has_errors();

    vec4 bg_col = vec4(0, 0, 0, 0);
    if (ECS::registry<TurnGraphic>.has(entity)) {
        int order = ECS::registry<TurnGraphic>.get(entity).order;
        float val = clamp((5 - order) * 0.1 + 0.5, 0.6, 1.0); // from 0.6 to 1.0
        bg_col = vec4(0.4, val - 0.2, val, 0.2);
    }
    // add background to texture
    GLint bg_uloc = glGetUniformLocation(texmesh.effect.program, "background_col");
    glUniform4fv(bg_uloc, 1, (float*)&bg_col);
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::drawTileTexturedMesh(ECS::Entity entity, const mat3& projection)
{
    auto& tile_position = ECS::registry<TilePosition>.get(entity);
    // auto& motion = ECS::registry<Motion>.get(entity);
    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    vec2 offset = cam_position - camera_offset;
    transform.translate(tile_position.screen_pos - offset);
    if (ECS::registry<Projectile>.has(entity)) {
        transform.rotate(ECS::registry<Projectile>.get(entity).rotation);
    } else if (ECS::registry<Boomerang>.has(entity)) {
        transform.rotate(ECS::registry<Boomerang>.get(entity).angle);
    }
    transform.scale(tile_position.scale);
    // transform.translate(motion.position);
    //	transform.scale(motion.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
    GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
    if (in_texcoord_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
    }
    else if (in_color_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
	}
    else
    {
        throw std::runtime_error("This type of entity is not yet supported");
    }
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();

    vec3 col = vec3(0, 0, 0);
    if(tile_position.lit) {
        // higlight cell colour
        col = vec3(0.2, 0.3, 0.6);
    }

    if(ECS::registry<Torch>.has(entity)) {
        auto& t = ECS::registry<Torch>.get(entity);

        // set colour
        GLint tint_uloc = glGetUniformLocation(texmesh.effect.program, "tint_col");
        glUniform3fv(tint_uloc, 1, (float*)&t.colour);
        gl_has_errors();
    }


    // add highlight to cell
    GLint lit_uloc = glGetUniformLocation(texmesh.effect.program, "highlight_col");
    glUniform3fv(lit_uloc, 1, (float*)&col);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::drawBackground(ECS::Entity entity, const mat3& projection)
{
    auto& motion = ECS::registry<Motion>.get(entity);
    auto& bg = ECS::registry<Background>.get(entity);

    vec2 offset = -cam_position + camera_offset;

    vec2 parallax_pos = bg.parallax*offset + (1-bg.parallax)*bg.base_position;

    motion.position = parallax_pos;
    drawTexturedMesh(entity, projection);
}

// Draws a texture map. White are all the places that are lit by lights, black where shadow.
// first casts rays and then creates geometries from intersections. Then renders to a texture.I
void RenderSystem::createAndDrawShadowMasks(vec2 windowSize, const mat3& projection) {

    if(ECS::registry<LightSource>.size() == 0) return;
//        std::cout << "===draw===\n";

    // do the drawing
    drawShadowMasks(windowSize);

    for (auto& e : ECS::registry<LightSource>.entities) {
        // raycast to all vertices
        auto verts = rayCastAllWallVertices(e);

        assert(verts.size() > 1);
        if(verts.size() < 2) continue; // if we don't have enough to make a triangle then don't

        auto &tpos = ECS::registry<TilePosition>.get(e);

        // sort by relative position to the light
        for (auto &v : verts) v -= tpos.screen_pos / 100.f;

        // lambda sort all vertices
        std::sort(verts.begin(), verts.end(),
                  [](const vec2 &a, const vec2 &b) -> bool {
                      return cartesianTheta(a) > cartesianTheta(b);
                  });

        for (auto &v : verts) v += tpos.screen_pos / 100.f; // untransform for triangles creation

        createTriangles(verts, projection, e);
    }

    // switch back to main framebuffer to draw screen
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
}

void RenderSystem::drawShadowMasks(vec2 windowSize) {
    // make sure to draw triangles to shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowframe_buffer);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rendered_shadow);

    int w, h;
    glfwGetFramebufferSize(&window, &w, &h);

    // start with empty texture (not initialized to colour yet!)
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, w, h, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);

    // set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // set texture for the frame buffer to the rendered shadow
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_shadow, 0);

    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);
    gl_has_errors();

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("draw shadow masks:  framebuffer broke");


    // Set colour to black
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    gl_has_errors();

    // draw to frame! jk this is done in createTriangle

}

void RenderSystem::createTriangles(const std::vector<vec2>& vs, const mat3& projection, ECS::Entity lightSource) {

    unsigned int len = vs.size();
    std::vector<ColoredVertex> vertices = std::vector<ColoredVertex>(len+1);

    TilePosition t = ECS::registry<TilePosition>.get(lightSource);

    Transform transform;
    vec2 offset = cam_position - camera_offset;
    transform.translate(-offset);
    transform.scale({100.f, 100.f});

    std::string key = "tri";
    std::vector<ColoredVertex> vertices_;
    std::vector<uint16_t> vertex_indices_;

    // 0th vertex is just the light source
    vertices[0] = ColoredVertex();
    vec3 color = {0.5, 0.5, 0.5};
    vertices[0].position = {t.screen_pos.x/100.f, t.screen_pos.y/100.f, -0.1};
    vertices[0].color = color;
    vertices_.push_back(vertices[0]);

    // push subsequent vertices
    for(int i = 1; i < len+1; i++) {
        //vertices[i].position = vec3(vs[i-1].x, vs[i-1].y, -0.1);
        vertices[i] = ColoredVertex();
        vertices[i].position = {vs[i-1].x, vs[i-1].y, -0.1};
        vertices[i].color = color;
        vertices_.push_back(vertices[i]);
    }

    // make triangles; last one should loop back to first
//    uint16_t indices[len];
    for(int i = 1; i < len+1; i++) {
        if(i == len) { // loop back
            vertex_indices_.push_back(0);
            vertex_indices_.push_back(i);
            vertex_indices_.push_back(1);
        } else {
            vertex_indices_.push_back(0);
            vertex_indices_.push_back(i);
            vertex_indices_.push_back(i+1);
        }
    }

    GLResource<BUFFER> vbo;
    GLResource<BUFFER> ibo;
    GLResource<VERTEX_ARRAY> vao;

    // load things and such
    glGenVertexArrays(1, vao.data());
    glGenBuffers(1, vbo.data());
    glGenBuffers(1, ibo.data());
    //glBindVertexArray(resource.mesh.vao);
    gl_has_errors();

    glDisable(GL_DEPTH_TEST);

    // Vertex Buffer creation
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredVertex) * vertices_.size(), vertices_.data(), GL_STATIC_DRAW);
    gl_has_errors();

    // Index Buffer creation
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * vertex_indices_.size(), vertex_indices_.data(), GL_STATIC_DRAW);
    gl_has_errors();

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

    // set shaders
    glUseProgram(shadow_sprite.effect.program);
    glBindVertexArray(vao);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(shadow_sprite.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(shadow_sprite.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(shadow_sprite.effect.program, "in_position");
    GLint in_color_loc = glGetAttribLocation(shadow_sprite.effect.program, "in_color");

    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
//    glEnableVertexAttribArray(in_color_loc);
//    glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
    gl_has_errors();


    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = vertex_indices_.size() * sizeof(uint16_t);
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    auto& l = ECS::registry<LightSource>.get(lightSource);

    glUniform1fv(
            glGetUniformLocation(shadow_sprite.effect.program, "lightStrength"),
            1,
            (float*)&l.strength
            );

    // Drawing of num_indices specified in the index buffer
    glDrawElements(GL_TRIANGLES, vertex_indices_.size(), GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
    gl_has_errors();
}

float RenderSystem::cartesianTheta(vec2 pos) {
    if (pos.x == 0) {
        return (pos.y > 0)? M_PI/2 : M_PI * 1.5;
    }
    float angle = atan(pos.y/pos.x);
    // account for quadrants
    if(pos.x >= 0) {
        if(pos.y < 0)  angle += 2 * M_PI;
    } else {
        if (pos.y < 0)  angle += M_PI;
        else angle += M_PI; // second quadrant
    }
    return angle;
}

std::vector<vec2> RenderSystem::rayCastAllWallVertices(ECS::Entity light) {
    // Cast rays for every light source
    auto hits = std::vector<vec2>();
    auto& tpos = ECS::registry<TilePosition>.get(light);
    for(auto& vert : ECS::registry<V>.components) {
        float theta_e = 0.001;
        float c = cos(theta_e); float s = sin(theta_e);

        mat2 rot_cw = {{c,s},{-s, c}};
        mat2 rot_cc = {{c, -s},{s, c}};

        vec2 tscreen = tpos.screen_pos*0.01f;

        vert.pos -= tscreen;
        // rotate with small theta so we can get the wall and the wall behind
        vec2 v1 = rot_cw*vert.pos;
        vec2 v2 = rot_cc*vert.pos;
        v1 += tscreen;
        v2 += tscreen;

        vert.pos += tscreen;

        vec2 hit, hit2;

        if(false) 1+1;
        else if ((v1-tscreen).x > 0 && (v1-tscreen).y > 0) hit = rayCastQ1(tscreen, v1, 1000);
        else if ((v1-tscreen).x < 0 && (v1-tscreen).y > 0) hit = rayCastQ2(tscreen, v1, 1000);
        else if ((v1-tscreen).x < 0 && (v1-tscreen).y < 0) hit = rayCastQ3(tscreen, v1, 1000);
        else if ((v1-tscreen).x > 0 && (v1-tscreen).y < 0) hit = rayCastQ4(tscreen, v1, 1000);
        else hit = rayCast(tscreen, v1, 1000);

        if(false) 1+1;
        else if ((v2-tscreen).x > 0 && (v2-tscreen).y > 0) hit2 = rayCastQ1(tscreen, v2, 1000);
        else if ((v2-tscreen).x < 0 && (v2-tscreen).y > 0) hit2 = rayCastQ2(tscreen, v2, 1000);
        else if ((v2-tscreen).x < 0 && (v2-tscreen).y < 0) hit2 = rayCastQ3(tscreen, v2, 1000);
        else if ((v2-tscreen).x > 0 && (v2-tscreen).y < 0) hit2 = rayCastQ4(tscreen, v2, 1000);
        else hit2 = rayCast(tscreen, v2, 1000);

        hits.emplace_back(hit);
        hits.emplace_back(hit2);

        if(DebugSystem::in_debug_mode) {
            DebugSystem::createDottedLine(tpos.grid_pos*60.f + 200.f, hit*60.f + 200.f, 40, 3);
            DebugSystem::createDottedLine(tpos.grid_pos*60.f + 200.f, hit2*60.f + 200.f, 40, 3);
        }
    }
    return hits;
}

// casts a ray and returns the intersection. Direction should define the length of the raycast.
vec2 RenderSystem::rayCast(vec2 o, vec2 d, float max_dist) {
    return d;
}

// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawToScreen(vec2 windowSize)
{
    // Setting shaders
    glUseProgram(screen_sprite.effect.program);
    glBindVertexArray(screen_sprite.mesh.vao);
    gl_has_errors();

    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(&window, &w, &h);
    glBindFramebuffer(GL_FRAMEBUFFER, preprocessing_buffer);
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0);
    glClearDepth(1.f);
    gl_has_errors();


    // Bind our texture in Texture Unit 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, rendered_screen);
    // start with empty texture (not initialized to colour yet!)
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, w, h, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    // set filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // set texture for the frame buffer to the rendered shadow
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_screen, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);
    gl_has_errors();


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Disable alpha channel for mapping the screen texture onto the real screen
    glDisable(GL_BLEND); // we have a single texture without transparency. Areas with alpha <1 cab arise around the texture transparency boundary, enabling blending would make them visible.
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, screen_sprite.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen_sprite.mesh.ibo); // Note, GL_ELEMENT_ARRAY_BUFFER associates indices to the bound GL_ARRAY_BUFFER
    gl_has_errors();

    // Draw the screen texture on the quad geometry
    gl_has_errors();

    // Set clock
    GLuint time_uloc       = glGetUniformLocation(screen_sprite.effect.program, "time");
    glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
    gl_has_errors();

    // Set the vertex position and vertex texture coordinates (both stored in the same VBO)
    GLint in_position_loc = glGetAttribLocation(screen_sprite.effect.program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    GLint in_texcoord_loc = glGetAttribLocation(screen_sprite.effect.program, "in_texcoord");
    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
    gl_has_errors();

    // TODO: change block for multilight

    Transform screen_transform;
    screen_transform.scale(vec2(windowSize.x, windowSize.y));

    GLint screen_transform_uloc = glGetUniformLocation(screen_sprite.effect.program, "screen_transform");
    glUniformMatrix3fv(screen_transform_uloc, 1, GL_FALSE, (float*)&screen_transform.mat);

    vec3 l_pos[20];
    vec3 l_cols[20];
    int l_size = ECS::registry<LightSource>.entities.size();
    GLint light_size_uloc = glGetUniformLocation(screen_sprite.effect.program, "num_lights");
    glUniform1iv(light_size_uloc, 1, (int*)&l_size);
    gl_has_errors();
    assert (l_size <= 20);
    vec2 offset = cam_position - camera_offset;

    for (int i = 0; i < l_size; i++) {
        auto& le = ECS::registry<LightSource>.entities[i];
        vec2 tpos = ECS::registry<TilePosition>.get(le).screen_pos;
        vec2 final = tpos - offset;
        float ls = ECS::registry<LightSource>.get(le).strength;
        l_pos[i] = vec3(final.x, final.y, ls);
        l_cols[i] = ECS::registry<LightSource>.get(le).lightColour;
    }
    GLint light_position_uloc = glGetUniformLocation(screen_sprite.effect.program, "light_pos");
    glUniform3fv(light_position_uloc, 20, (float*)l_pos);
    gl_has_errors();

    GLint light_colors_uloc = glGetUniformLocation(screen_sprite.effect.program, "light_cols");
    glUniform3fv(light_colors_uloc, 20, (float*)l_cols);
    gl_has_errors();


    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, rendered_shadow);
    GLuint shadow_uloc = glGetUniformLocation(screen_sprite.effect.program, "renderedShadow");

    glUniform1i(shadow_uloc, rendered_shadow);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_sprite.texture.texture_id);

    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
    glBindVertexArray(0);
    gl_has_errors();
}

// Draw the intermediate texture to the screen, with postprocessing
void RenderSystem::drawPostProcessing(vec2 windowSize)
{
    // Setting shaders
    glUseProgram(process_sprite.effect.program);
    glBindVertexArray(process_sprite.mesh.vao);
    gl_has_errors();

    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(&window, &w, &h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Disable alpha channel for mapping the screen texture onto the real screen
    glDisable(GL_BLEND); // we have a single texture without transparency. Areas with alpha <1 cab arise around the texture transparency boundary, enabling blending would make them visible.
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, process_sprite.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, process_sprite.mesh.ibo); // Note, GL_ELEMENT_ARRAY_BUFFER associates indices to the bound GL_ARRAY_BUFFER
    gl_has_errors();

    // Draw the screen texture on the quad geometry
    gl_has_errors();

    auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);

    GLuint bloom_uloc = glGetUniformLocation(process_sprite.effect.program, "bloom_factor");
    GLuint bloom_rad_uloc = glGetUniformLocation(process_sprite.effect.program, "bloom_radius");
    glUniform1f(bloom_uloc, screen.bloom_factor);
    glUniform1f(bloom_rad_uloc, screen.bloom_radius);

    GLuint brightness_uloc = glGetUniformLocation(process_sprite.effect.program, "brightness");
    glUniform1f(brightness_uloc, screen.brightness);

    GLuint saturation_uloc = glGetUniformLocation(process_sprite.effect.program, "saturation");
    glUniform1f(saturation_uloc, screen.saturation);

    GLuint chrom_uloc = glGetUniformLocation(process_sprite.effect.program, "chromatic_aberration");
    glUniform1f(chrom_uloc, screen.chrom_abb);

    gl_has_errors();

    // Set the vertex position and vertex texture coordinates (both stored in the same VBO)
    GLint in_position_loc = glGetAttribLocation(process_sprite.effect.program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    GLint in_texcoord_loc = glGetAttribLocation(process_sprite.effect.program, "in_texcoord");
    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
    gl_has_errors();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rendered_screen);
    GLuint loc = glGetUniformLocation(process_sprite.effect.program, "renderedScreen");
    glUniform1i(loc, 1);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, process_sprite.texture.texture_id);

    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
    glBindVertexArray(0);
    gl_has_errors();
}

void RenderSystem::updateCamera(float elapsed_ms) {
    float factor = (smoothing + elapsed_ms / 3.f);
    factor = -1.f/(factor+1.f) + 1.f;
    cam_position = (factor) * cam_position + (1-(factor)) * camera_focus;
    if(ECS::registry<HurtQueue>.size() > 0) {
        auto hq = ECS::registry<HurtQueue>.components[0];
        if(!hq.q.empty()) {
            auto hurt = hq.q.front();
            float shakeFactor = hurt.damage * (hurt.counterMs - 800)/80;
            if(hurt.counterMs > 800)
                cam_position += uniform_dist(rng) * shakeFactor - shakeFactor/2;
        }
    }
}
void RenderSystem::moveCamera(vec2 pos) {
    float factor = 10;
    factor = -1.f / (factor + 1.f) + 1.f;
    // cam_position = (factor)*cam_position + (1 - (factor)) * pos;
    cam_position = pos;
}
void RenderSystem::moveCamera(ECS::Entity en) {
    vec2 pos = ECS::registry<TilePosition>.get(en).screen_pos;
    float factor = 10;
    factor = -1.f / (factor + 1.f) + 1.f;
    cam_position = (factor)*cam_position + (1 - (factor)) * pos;
    //cam_position = pos;
}
void RenderSystem::drawHealthBars(ECS::Entity entity, const mat3& projection) {
    auto& tile_position = ECS::registry<TilePosition>.get(entity);
    // auto& motion = ECS::registry<Motion>.get(entity);
    auto& texmesh = *ECS::registry<HealthBar>.get(entity).barGraphic.reference_to_cache;
    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    vec2 offset = cam_position - camera_offset;
    transform.translate(tile_position.screen_pos - offset);
    transform.scale(tile_position.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
    GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
    if (in_texcoord_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
    }
    else if (in_color_loc >= 0)
    {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
    }
    else
    {
        throw std::runtime_error("This type of entity is not yet supported");
    }
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw(vec2 window_size_in_game_units)
{
    // Getting size of window
    ivec2 frame_buffer_size; // in pixels
    glfwGetFramebufferSize(&window, &frame_buffer_size.x, &frame_buffer_size.y);

    // First render to the custom framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();

    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 1.0); // Set background to black
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Fake projection matrix, scales with respect to window coordinates
    float left = 0.f;
    float top = 0.f;
    float right = window_size_in_game_units.x;
    float bottom = window_size_in_game_units.y;

    camera_offset = {right/2, bottom/2};

    float sx = 2.f / (right - left);
    float sy = 2.f / (top - bottom);
    float tx = -(right + left) / (right - left);
    float ty = -(top + bottom) / (top - bottom);
    mat3 projection_2D{ { sx, 0.f, 0.f },{ 0.f, sy, 0.f },{ tx, ty, 1.f } };

    // there may be an absence of turns in title screens and such
//    assert(ECS::registry<Turn>.entities.size() == 1);

    ECS::Entity snipe_cursor;
    // testcam
    if (WorldSystem::isDraggingCamera) {
        // Camera movement: free movement 
        camera_focus = WorldSystem::mouse_position;
        
        // Camera movement: pan camera
//        camera_focus.x = WorldSystem::mouse_position.x;
    }else {
        if (!ECS::registry<Turn>.entities.empty())
            camera_focus = ECS::registry<TilePosition>.get(ECS::registry<Turn>.entities[0]).screen_pos;
    }
    
	// Draw all textured meshes that have a position and size component
	for (ECS::Entity entity : ECS::registry<ShadedMeshRef>.entities) {
	    if (ECS::registry<Hidden>.has(entity)) {
	        // Do not render
	    } else if (ECS::registry<Motion>.has(entity)) {
	        if(ECS::registry<Background>.has(entity)) {
                drawBackground(entity, projection_2D);
                continue;
            }
        } else if (ECS::registry<TilesheetTile>.has(entity) || ECS::registry<Door>.has(entity)) {
	        drawTilesheetTile(entity, projection_2D);
        } else if (ECS::registry<TilePosition>.has(entity) && !ECS::registry<Turn>.entities.empty()) {
            if(ECS::registry<Cursor>.size()>0 && ECS::registry<Cursor>.has(entity)) {
                snipe_cursor = entity;
                continue;
            }
            drawTileTexturedMesh(entity, projection_2D);
            gl_has_errors();
        }
    }
    int i = 0;
    // Draw all animated meshes
    for (ECS::Entity entity : ECS::registry<Animation>.entities) {
        if (ECS::registry<Motion>.has(entity)) {
            drawAnimatedMesh(entity, projection_2D);
            gl_has_errors();
        } else if (ECS::registry<TilePosition>.has(entity) && !ECS::registry<Turn>.entities.empty()) {
            drawTileAnimatedMesh(entity, projection_2D);
            if(ECS::registry<HealthBar>.size() > 0 && ECS::registry<HealthBar>.has(entity)) {
                drawHealthBars(entity, projection_2D);
            }
            gl_has_errors();
        }
    }

    // draw shadow
    if (ECS::registry<V>.size() > 0) createAndDrawShadowMasks(window_size_in_game_units, projection_2D);

    // Truly render to the screen
    drawToScreen(window_size_in_game_units);

    drawPostProcessing(window_size_in_game_units);

    // Draw text components to the screen
    // NOTE: for simplicity, text components are drawn in a second pass,
    // on top of all texture mesh components. This should be reasonable
    // for nearly all use cases. If you need text to appear behind meshes,
    // consider using a depth buffer during rendering and adding a
    // Z-component or depth index to all rendererable components.
    for (const Text& text: ECS::registry<Text>.components) {
        drawText(text, window_size_in_game_units);
    }

    // set frameBuffer to the screen so we can draw over it
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render the particles
    if(ECS::registry<ParticleEmitter>.size() > 0) {
        for(auto& e : ECS::registry<ParticleEmitter>.entities) {
            drawParticles(e, projection_2D);
        }
    }

    // draw cursor
    if(ECS::registry<Cursor>.size()>0)
        drawTileTexturedMesh(snipe_cursor, projection_2D);


    // draw ui
    for (ECS::Entity entity : ECS::registry<ShadedMeshRef>.entities) {
        if (ECS::registry<Motion>.has(entity)) {
            if (ECS::registry<Background>.has(entity) || ECS::registry<Hidden>.has(entity)) {
                continue;
            }
            // Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
            drawTexturedMesh(entity, projection_2D);
            gl_has_errors();
        }
    }

    gl_has_errors();

    // flicker-free display with a double buffer
    glfwSwapBuffers(&window);
}


void gl_has_errors()
{
    GLenum error = glGetError();

    if (error == GL_NO_ERROR)
        return;

    const char* error_str = "";
    while (error != GL_NO_ERROR)
    {
        switch (error)
        {
            case GL_INVALID_OPERATION:
                error_str = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                error_str = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error_str = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                error_str = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error_str = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        std::cerr << "OpenGL:" << error_str << std::endl;
        error = glGetError();
    }
    throw std::runtime_error("last OpenGL error:" + std::string(error_str));
}
