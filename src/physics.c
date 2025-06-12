#include "physics.h"

void update_physics(SpherePhysics* s, float dt, vec3 boxMin, vec3 boxMax) {
    // Gravedad
    s->velocity[1] -= 9.81f * dt;

    // Movimiento
    glm_vec3_muladds(s->velocity, dt, s->position);

    // Colisiones con caja invisible
    for (int i = 0; i < 3; ++i) {
        float min = boxMin[i] + s->radius;
        float max = boxMax[i] - s->radius;

        if (s->position[i] < min) {
            s->position[i] = min;
            s->velocity[i] *= -0.8f; // rebote simple
        } else if (s->position[i] > max) {
            s->position[i] = max;
            s->velocity[i] *= -0.8f;
        }
    }
}

void resolve_sphere_collisions(SpherePhysics* spheres, int count) {
    float padding =.001f;
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            vec3 diff;
            glm_vec3_sub(spheres[j].position, spheres[i].position, diff);
            float dist = glm_vec3_norm(diff);
            float minDist = spheres[i].radius + spheres[j].radius;

            if (dist < minDist && dist > 0.0f) {
                // Separar las esferas para que no se superpongan
                float overlap = minDist - dist + padding;
                vec3 correction;
                glm_vec3_scale(diff, overlap / dist / 2.0f, correction);

                // Mover cada esfera la mitad de la corrección
                glm_vec3_sub(spheres[i].position, correction, spheres[i].position);
                glm_vec3_add(spheres[j].position, correction, spheres[j].position);

                // Ajustar velocidades (simple rebote elástico)
                vec3 relativeVel;
                glm_vec3_sub(spheres[j].velocity, spheres[i].velocity, relativeVel);

                float dot = glm_vec3_dot(relativeVel, diff) / (dist * dist);

                if (dot > 0) {  // Solo si se acercan
                    vec3 normal;
                    glm_vec3_normalize_to(diff, normal);
                    float v1n = glm_vec3_dot(spheres[i].velocity, normal);
                    float v2n = glm_vec3_dot(spheres[j].velocity, normal);

                    float m1 = 1.0f; // asumamos masa 1 para ambas
                    float m2 = 1.0f;

                    float optimizedP = (2.0f * (v1n - v2n)) / (m1 + m2);

                    vec3 v1Change, v2Change;
                    glm_vec3_scale(normal, optimizedP * m2, v1Change);
                    glm_vec3_scale(normal, optimizedP * m1, v2Change);

                    glm_vec3_sub(spheres[i].velocity, v1Change, spheres[i].velocity);
                    glm_vec3_add(spheres[j].velocity, v2Change, spheres[j].velocity);
                }
            }
        }
    }
}
