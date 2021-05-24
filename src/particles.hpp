//
// Created by Roark on 2021-03-30.
//

#pragma once

#include "common.hpp"
#include "render.hpp"
#include <random>

struct Particle {
    Motion motion; // motion must be used because we need rotation scale etc.
    float opacity;
    float life;
};

struct ParticleEmitter {
    ParticleEmitter(ShadedMeshRef texture);

    ShadedMeshRef texture;

    bool stillAlive;

    bool persistent;
    int burstNum;

    // for persistent emitters
    float nextSpawn;
    float timer;
    float totalTimer;
    float maxTime = -1;
    // period between spawns
    vec2 period;

    // vec2 used as min, max
    vec2 lifeTimes;

    // speed is not vec2 to avoid confusion
    // note: not velocity!!
    float minSpeed;
    float maxSpeed;

    float minScale;
    float maxScale;
    float scaleChange;

    // angles in which the particle will travel
    float angle;
    float angleSpread; // parameter for normal distribution

    vec2 spawn1;
    vec2 spawn2;

    vec2 gravity;
    float drag;

    vec2 startingOpacity;
    float opacityDecrease;

    int getUnusedParticle();

    int lastUsedParticle;

    static const int MaxParticles = 1000;
    Particle particleContainer[MaxParticles];
};

namespace ParticleSystem {
    void initParticles();
    void updateParticles(float elapsed_ms);

    void destroyAllEmitters();

    ECS::Entity createParticleBurst(std::string path,
            int numParticles, float minSpeed, float maxSpeed,
            vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread,
            vec2 gravity, float drag,
            float minScale, float maxScale, float scaleChange,
            vec2 startingOpacity, float opacityDecrease
            );

    ECS::Entity createParticleEmitter(std::string path,
                                          float minSpeed, float maxSpeed,
                                          vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread,
                                          vec2 period, vec2 gravity, float drag,
                                          float minScale, float maxScale, float scaleChange, vec2 startingOpacity, float opacityDecrease);


    ECS::Entity createTimedParticleEmitter(std::string path,
                                               float minSpeed, float maxSpeed,
                                          vec2 spawn1, vec2 spawn2, vec2 lifeTimes, float angle, float angleSpread,
                                          vec2 period, vec2 gravity, float drag,
                                          float minScale, float maxScale, float scaleChange, vec2 startingOpacity, float opacityDecrease,
                                          float maxTime);

//    static const int MaxParticles = 1000;
//    extern Particle ParticlesContainer[MaxParticles];
    extern int numParticles;
    // global shader
    extern ShadedMesh shaderContainer;

    extern GLuint particles_position_buffer;
    extern GLuint particles_opacity_buffer;
    extern GLuint particles_vertex_buffer;

};
