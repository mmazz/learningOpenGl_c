#ifndef PHYSICS_H
#define PHYSICS_H


#include <cglm/cglm.h>
#include <stddef.h>
#include <glad/gl.h>

typedef struct {
    vec3 position;  // posición actual
    vec3 velocity;  // velocidad actual
    float radius;   // radio de la esfera
} SpherePhysics;

void update_physics(SpherePhysics* s, float dt, vec3 boxMin, vec3 boxMax);
void resolve_sphere_collisions(SpherePhysics* spheres, int count);
#endif
