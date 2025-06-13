#include "physics.h"
#include <string.h>
#include <math.h>
#include <cglm/cglm.h>

#define HASH_TABLE_SIZE 2097152     // primo cercano a 2ⁿ para buen hashing
#define MAX_BUCKET_SIZE 32          // max partículas por celda

// Tabla de hash: por simplicidad, un arreglo fijo de buckets
static int hashCount[HASH_TABLE_SIZE];
static int hashTable[HASH_TABLE_SIZE][MAX_BUCKET_SIZE];

// Constantes de hashing
static const unsigned int p1 = 73856093u;
static const unsigned int p2 = 19349663u;
static const unsigned int p3 = 83492791u;

// Función de hash espacial
static inline unsigned int spatial_hash(int x, int y, int z) {
    unsigned int h = (unsigned int)(x * p1 ^ y * p2 ^ z * p3);
    return h & (HASH_TABLE_SIZE - 1);  // asume HASH_TABLE_SIZE potencia de 2
}

// Calcula coords de celda
static inline void get_cell_coords(const vec3 pos, float cellSize, int *ix, int *iy, int *iz) {
    *ix = (int)floorf(pos[0] / cellSize);
    *iy = (int)floorf(pos[1] / cellSize);
    *iz = (int)floorf(pos[2] / cellSize);
}

void collision_box(Particles* p, const vec3 boxMin, const vec3 boxMax) {
    for (int i = 0; i < 3; ++i) {
        float min = boxMin[i] + p->radius;
        float max = boxMax[i] - p->radius;

        if (p->current[i] < min) {
            p->current[i] = min;
            p->previus[i] *= -0.8f; // rebote simple
        } else if (p->current[i] > max) {
            p->current[i] = max;
            p->previus[i] *= -0.8f;
        }
    }
}

void collision_sphere(Particles* p, float radius) {
    // Colisión con esfera de radio fijo centrada en el origen
    float dist = glm_vec3_norm(p->current);
    float max_dist = radius - p->radius;

    if (dist > max_dist) {
        // Reubicar en la superficie
        vec3 normal;
        glm_vec3_normalize_to(p->current, normal);  // dirección desde el centro

        glm_vec3_scale(normal, max_dist, p->current);

        // Reflejar velocidad (rebote simple)
        float v_dot_n = glm_vec3_dot(p->previus, normal);
        vec3 v_reflected;
        glm_vec3_scale(normal, 2.0f * v_dot_n, v_reflected);
        glm_vec3_sub(p->previus, v_reflected, p->previus);
        glm_vec3_scale(p->previus, 0.8f, p->previus); // pérdida de energía
    }
}

void update_physics(Particles* p, float radius, float dt, const vec3 boxMin, const vec3 boxMax, bool isABox) {
    float dt2 = dt*dt;
    vec3 res;

    // res = 2 * current - previous + acceleration * dt^2
    glm_vec3_scale(p->current, 2.0f, res);
    glm_vec3_sub(res, p->previus, res);

    vec3 accTerm;
    glm_vec3_scale(p->acceleration, dt2, accTerm);
    glm_vec3_scale(accTerm, 0.5, accTerm);
    glm_vec3_add(res, accTerm, res);

    // Update
    glm_vec3_copy(p->current, p->previus);
    glm_vec3_copy(res, p->current);

    if(isABox)
        collision_box(p, boxMin, boxMax);
    else
        collision_sphere( p, radius);
}

void resolve_sphere_collisions(Particles* spheres, int count) {
    // Parámetro: tamaño de celda = doble del radio máximo
    float maxRadius = 0.0f;
    for (int i = 0; i < count; i++)
        if (spheres[i].radius > maxRadius) maxRadius = spheres[i].radius;
    float cellSize = maxRadius * 2.0f;

    // 1) Limpiar hash
    memset(hashCount, 0, sizeof(hashCount));

    // 2) Insertar cada partícula en su celda
    for (int i = 0; i < count; i++) {
        int cx, cy, cz;
        get_cell_coords(spheres[i].current, cellSize, &cx, &cy, &cz);
        unsigned int h = spatial_hash(cx, cy, cz);
        if (hashCount[h] < MAX_BUCKET_SIZE)
            hashTable[h][hashCount[h]++] = i;
    }

    // 3) Varias iteraciones de corrección (evita que queden atrapadas)
    const int iterations = 4;
    const float restitution = 0.8f;  // coeficiente de restitución
    const float padding = 1e-3f;

    for (int it = 0; it < iterations; it++) {
        // Para cada partícula
        for (int i = 0; i < count; i++) {
            // Localizar su celda base
            int cx, cy, cz;
            get_cell_coords(spheres[i].current, cellSize, &cx, &cy, &cz);

            // Chequear vecinos en las 27 celdas alrededor
            for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                unsigned int h = spatial_hash(cx+dx, cy+dy, cz+dz);
                int bucketSize = hashCount[h];
                for (int bi = 0; bi < bucketSize; bi++) {
                    int j = hashTable[h][bi];
                    if (j <= i) continue;  // evita duplicados y self

                    vec3 diff;
                    glm_vec3_sub(spheres[j].current, spheres[i].current, diff);
                    float dist = glm_vec3_norm(diff);
                    float minDist = spheres[i].radius + spheres[j].radius;
                    if (dist <= 0.0f || dist >= minDist + padding) continue;

                    // 3a) Separación de posiciones
                    float overlap = (minDist + padding - dist);
                    vec3 normal;
                    glm_vec3_divs(diff, dist, normal);  // normaliza diff
                    vec3 correction;
                    glm_vec3_scale(normal, overlap * 0.5f, correction);
                    // desplaza i y j en direcciones opuestas
                    glm_vec3_sub(spheres[i].current, correction, spheres[i].current);
                    glm_vec3_add(spheres[j].current, correction, spheres[j].current);

                    // 3b) Impulso de colisión elástica
                    // calcula componente normal de la velocidad relativa
                    vec3 relVel;
                    glm_vec3_sub(spheres[j].previus, spheres[i].previus, relVel);
                    float vRel = glm_vec3_dot(relVel, normal);
                    if (vRel > 0.0f) continue;  // se alejan, no aplica impulso

                    // masa = 1 para ambas → impulso simple
                    float jImpulse = -(1.0f + restitution) * vRel * 0.5f;
                    vec3 impulse;
                    glm_vec3_scale(normal, jImpulse, impulse);
                    glm_vec3_sub(spheres[i].previus, impulse, spheres[i].previus);
                    glm_vec3_add(spheres[j].previus, impulse, spheres[j].previus);
                }
            } } }
        }
    }
}

