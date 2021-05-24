//
// Created by Roark on 2021-03-30.
//

#include "particles.hpp"
#include <iostream>

namespace ParticleSystem {
//    Particle ParticlesContainer[MaxParticles];
    GLuint particles_position_buffer;
    GLuint particles_opacity_buffer;
    GLuint particles_vertex_buffer;
    int numParticles;
    ShadedMesh shaderContainer;
    std::default_random_engine rng;

    void updateParticles(float elapsed_ms) {
        if(ECS::registry<ParticleEmitter>.size() <= 0) return;

        for(auto& e : ECS::registry<ParticleEmitter>.entities) {
            auto& pe = ECS::registry<ParticleEmitter>.get(e);
            // update current particles
            for (int i = 0; i < ParticleEmitter::MaxParticles; i++) {
                auto &cur = pe.particleContainer[i];
                if (cur.life > 0) {
                    cur.motion.position += (cur.motion.velocity * elapsed_ms / 1000.f);
                    cur.motion.velocity *= pe.drag;
                    cur.motion.velocity += pe.gravity * (elapsed_ms / 1000.f);
                    cur.motion.scale.x += pe.scaleChange * (elapsed_ms / 1000.f);
                    cur.opacity -= pe.opacityDecrease * (elapsed_ms / 1000.f);
                    cur.life -= elapsed_ms;
                } else {
                    // "hide" particle
                    cur.motion.position = {-1000, -1000};
                }
            }

            // don't spawn new particles if it isn't persistent
            if(!pe.persistent) continue;

            // spawn new particles
//            std::cout << "spawn new particles" << "\n";
            pe.timer += elapsed_ms;
            pe.totalTimer += elapsed_ms;
            if(pe.maxTime > 0 && pe.totalTimer > pe.maxTime) {
                ECS::registry<ParticleEmitter>.remove(e);
            }

            if(pe.timer > pe.nextSpawn) {

                // make sure we have no abrupt despawns; stop spawning things if we are approaching end of life
                if(pe.maxTime > 0 && pe.totalTimer > pe.maxTime-pe.lifeTimes.y) {
                    continue;
                }
                // spawn particle
                int index = pe.getUnusedParticle();

                std::uniform_real_distribution<float> uniform_dist; // number between 0..1
                std::normal_distribution<float> normal_dist{pe.angle, pe.angleSpread};
                // if we have space
                if(index >= 0) {
                    // spawn a particle
                    auto& cur = pe.particleContainer[index];

                    // set life
                    cur.life = uniform_dist(rng) * (pe.lifeTimes.y - pe.lifeTimes.x) + pe.lifeTimes.x;

                    // set position
                    int spawnx = uniform_dist(rng) * (pe.spawn2.x-pe.spawn1.x) + pe.spawn1.x;
                    int spawny = uniform_dist(rng) * (pe.spawn2.y-pe.spawn1.y) + pe.spawn1.y;
                    cur.motion.position = {spawnx, spawny};

                    // set velocity
                    cur.motion.velocity = {uniform_dist(rng) * (pe.maxSpeed - pe.minSpeed) + pe.minSpeed, 0};

                    /// rotate velocity by angle
                    float angle = normal_dist(rng);
                    float c = cos(angle);
                    float s = sin(angle);
                    mat2 rotation = {{c, s},{-s, c}};
                    cur.motion.velocity = rotation * cur.motion.velocity;

                    cur.opacity = (pe.startingOpacity.y - pe.startingOpacity.x) + pe.startingOpacity.x;
                    cur.motion.scale.x = uniform_dist(rng) * (pe.maxScale - pe.minScale) + pe.minScale;
                }

                // set next spawn duration
                pe.timer = 0.f;
                pe.nextSpawn = uniform_dist(rng) * (pe.period.y - pe.period.x) + pe.period.x;
            }
        }
    }

    void destroyAllEmitters() {
        if (ECS::registry<ParticleEmitter>.entities.size() == 0) return;
//        for(auto& pe : ECS::registry<ParticleEmitter>.components)
//            delete pe.texture.reference_to_cache;

        while (ECS::registry<ParticleEmitter>.entities.size()>0)
            ECS::ContainerInterface::remove_all_components_of(ECS::registry<ParticleEmitter>.entities.back());
    }

    void initParticles() {
        glGenBuffers(1, &ParticleSystem::particles_vertex_buffer);

        // create buffer for particle positions
        glGenBuffers(1, &ParticleSystem::particles_position_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_position_buffer);
        glBufferData(GL_ARRAY_BUFFER, ParticleEmitter::MaxParticles * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

        // create buffer for particle opacities
        glGenBuffers(1, &ParticleSystem::particles_opacity_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, ParticleSystem::particles_opacity_buffer);
        glBufferData(GL_ARRAY_BUFFER, ParticleEmitter::MaxParticles * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


        ParticleSystem::shaderContainer.effect.load_from_file(shader_path("particle.vs.glsl"), shader_path("particle.fs.glsl"));
    }

    ECS::Entity createParticleBurst(std::string path,
            int numParticles, float minSpeed, float maxSpeed,
            vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread, vec2 gravity, float drag,
            float minScale, float maxScale, float scaleChange,
            vec2 startingOpacity, float opacityDecrease) {
        ShadedMesh& resource = cache_resource(path);

        ParticleEmitter pe = ParticleEmitter(ShadedMeshRef(resource));
        RenderSystem::createSprite(*pe.texture.reference_to_cache, textures_path(path), "particle");

        pe.persistent = false;
        pe.stillAlive = true;
        pe.lifeTimes = lifeTimes;
        pe.gravity = gravity;
        pe.drag = drag;
        pe.minScale = minScale;
        pe.maxScale = maxScale;
        pe.scaleChange = scaleChange;
        pe.startingOpacity = startingOpacity;
        pe.opacityDecrease = opacityDecrease;

        // initialize all to dead
        for(int i = 0; i < ParticleEmitter::MaxParticles; i++) {
            auto& cur = pe.particleContainer[i];
            cur = Particle();
            cur.motion = Motion();
            cur.opacity = 1.0;
            cur.life = -1.f;
        }

        std::uniform_real_distribution<float> uniform_dist; // number between 0..1
        std::normal_distribution<float> normal_dist{angle, angleSpread};

        // revbive some pariclses
        for (int i = 0; i < (numParticles/2 + 1); i++) {
            auto& cur = pe.particleContainer[i];
            cur.life = uniform_dist(rng) * (pe.lifeTimes.y - pe.lifeTimes.x) + pe.lifeTimes.x;
            int spawnx = uniform_dist(rng) * (spawn2.x-spawn1.x) + spawn1.x;
            int spawny = uniform_dist(rng) * (spawn2.y-spawn1.y) + spawn1.y;

            cur.motion.position = {spawnx, spawny};

            cur.motion.velocity = {uniform_dist(rng) * (maxSpeed - minSpeed) + minSpeed, 0};
            float angle = normal_dist(rng);
            float c = cos(angle);
            float s = sin(angle);
            mat2 rotation = {{c, s},{-s, c}};
            cur.motion.velocity = rotation * cur.motion.velocity;

            cur.motion.scale.x = uniform_dist(rng) * (maxScale - minScale) + minScale;
        }

        ECS::Entity e = ECS::Entity();
        ECS::registry<ParticleEmitter>.emplace(e, pe);
        return e;
    }

    ECS::Entity createParticleEmitter(std::string path,
                                          float minSpeed, float maxSpeed,
                                          vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread,
                                          vec2 period, vec2 gravity, float drag,
                                          float minScale, float maxScale, float scaleChange,
                                          vec2 startingOpacity, float opacityDecrease) {
        ShadedMesh& resource = cache_resource(path);

        ParticleEmitter pe = ParticleEmitter(ShadedMeshRef(resource));
        RenderSystem::createSprite(*pe.texture.reference_to_cache, textures_path(path), "particle");

        pe.persistent = true;
        pe.lifeTimes = lifeTimes;
        pe.minSpeed = minSpeed;
        pe.maxSpeed = maxSpeed;
        pe.spawn1 = spawn1; pe.spawn2 = spawn2;
        pe.angle = angle; pe.angleSpread = angleSpread;
        pe.gravity = gravity;
        pe.drag = drag;
        pe.minScale = minScale;
        pe.maxScale = maxScale;
        pe.scaleChange = scaleChange;
        pe.startingOpacity = startingOpacity;
        pe.opacityDecrease = opacityDecrease;

        pe.period = period;

        // initialize all to dead
        for(auto & cur : pe.particleContainer) {
            cur = Particle();
            cur.motion = Motion();
            cur.opacity = 1.0;
            cur.life = -1.f;
        }

        ECS::Entity e = ECS::Entity();
        ECS::registry<ParticleEmitter>.emplace(e, pe);
        return e;
    }

    ECS::Entity createTimedParticleEmitter(std::string path,
                                               float minSpeed, float maxSpeed,
                                          vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread,
                                          vec2 period, vec2 gravity, float drag,
                                          float minScale, float maxScale, float scaleChange,
                                          vec2 startingOpacity, float opacityDecrease, float maxTime) {

        ShadedMesh& resource = cache_resource(path);

        ParticleEmitter pe = ParticleEmitter(ShadedMeshRef(resource));
        RenderSystem::createSprite(*pe.texture.reference_to_cache, textures_path(path), "particle");

        pe.persistent = true;
        pe.lifeTimes = lifeTimes;
        pe.minSpeed = minSpeed;
        pe.maxSpeed = maxSpeed;
        pe.spawn1 = spawn1; pe.spawn2 = spawn2;
        pe.angle = angle; pe.angleSpread = angleSpread;
        pe.gravity = gravity;
        pe.drag = drag;
        pe.minScale = minScale;
        pe.maxScale = maxScale;
        pe.scaleChange = scaleChange;
        pe.startingOpacity = startingOpacity;
        pe.opacityDecrease = opacityDecrease;
        pe.totalTimer = 0;
        pe.maxTime = maxTime;
        pe.period = period;

        // initialize all to dead
        for(auto & cur : pe.particleContainer) {
            cur = Particle();
            cur.motion = Motion();
            cur.opacity = 1.0;
            cur.life = -1.f;
        }

        ECS::Entity e = ECS::Entity();
        ECS::registry<ParticleEmitter>.emplace(e, pe);
        return e;
    }
}

ParticleEmitter::ParticleEmitter(ShadedMeshRef texture) : texture(texture) {
    this->timer = 0.0f;
}

int ParticleEmitter::getUnusedParticle() {
    // adapted from openGL tutorial 18
    // https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/

    // search to end
    for(int i=lastUsedParticle; i<ParticleEmitter::MaxParticles; i++){
        if (i >= 0) { // temporary addition to stop crashing
            if (this->particleContainer[i].life < 0) {
                lastUsedParticle = i;
                return i;
            }
        }
    }

    // search from beginning
    for(int i=0; i<lastUsedParticle; i++){
        if (this->particleContainer[i].life < 0){
            lastUsedParticle = i;
            return i;
        }
    }

    return -1;
}
