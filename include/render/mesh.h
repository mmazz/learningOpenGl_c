#ifndef MESH_H
#define MESH_H

#include <cglm/cglm.h>
#include <stddef.h>
#include <glad/gl.h>
#include <stdlib.h>

// Estructura que representa una malla con atributos interleaved
typedef struct {
    float*    vertices;      // arreglo de floats: posición (3) + UV (2) interleaved
    unsigned* indices;       // arreglo de índices
    size_t    vertexCount;   // número de vértices
    size_t    indexCount;    // número de índices
    size_t    indexSize;    // número de índices
    size_t    vertexSize;    // número de índices
    GLuint    VAO, VBO, EBO; // identificadores OpenGL
} Mesh;

typedef struct {
    vec3 position;
    float scale;
    Mesh* mesh;
} RenderObject;


/**
 * Genera una malla de cubo centrado en el origen con lado 1.0.
 * - Allocates y llena m.vertices e m.indices.
 * - vertexCount = 36, indexCount = 36.
 *
 * El usuario debe llamar después a mesh_upload_to_gpu(&mesh)
 * para subirla a la GPU, y mesh_destroy(&mesh) para liberar.
 */
Mesh mesh_generate_cube(void);
Mesh mesh_generate_sphere(int latDiv, int lonDiv);
#endif // MESH_H
