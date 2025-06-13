#ifndef PHYSICS_H
#define PHYSICS_H


#include <cglm/cglm.h>
#include <stddef.h>
#include <glad/gl.h>

typedef struct {
    vec3 current;  // posición actual
    vec3 previus;  //
    vec3 acceleration;
    float radius;   // radio de la esfera
} Particles;

void update_physics(Particles* s,float radius, float dt, const vec3 boxMin, const vec3 boxMax, bool isABox);
void resolve_sphere_collisions(Particles* spheres, int count);

void collision_sphere(Particles* p, float radius);
void collision_box(Particles* p, const vec3 boxMin, const vec3 boxMax);
#endif
