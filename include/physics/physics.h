#ifndef PHYSICS_H
#define PHYSICS_H


#include <cglm/cglm.h>
#include <stddef.h>
#include <glad/gl.h>
#include "core/config.h"

typedef struct {
    vec3 current;  // posición actual
    vec3 previus;  //
    vec3 acceleration;
    float radius;   // radio de la esfera
} Particles;

void update_physics(Config *config, Particles* s, float dt);
void resolve_collisions(Particles* spheres, int count);

void collision_sphere(Config *config, Particles* p);
void collision_box(Config *config, Particles* p);
#endif
