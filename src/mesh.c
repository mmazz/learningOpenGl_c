#include "mesh.h"
#include <string.h>
#include<math.h>

static const float vertexs[24*5] = {
    // Cara trasera
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,

    // Cara delantera
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,

    // Cara izquierda
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,

    // Cara derecha
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f,

    // Cara inferior
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    // Cara superior
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
};
static const unsigned int indexes[6*6] = {
    // Cara trasera
    0, 1, 2,  2, 3, 0,

    // Cara delantera
    4, 5, 6,  6, 7, 4,

    // Cara izquierda
    8, 9,10, 10,11, 8,

    // Cara derecha
    12,13,14, 14,15,12,

    // Cara inferior
    16,17,18, 18,19,16,

    // Cara superior
    20,21,22, 22,23,20
};

Mesh mesh_generate_cube(void) {
    Mesh m = {0};

    m.vertexCount = 24;       // cantidad de vértices
    m.indexCount = 6 * 6;     // cantidad de índices (6 caras * 2 triángulos * 3 vértices = 36)
    m.vertexSize = sizeof(vertexs);
    m.indexSize = sizeof(indexes);
    m.vertices = malloc(m.vertexCount * 5 * sizeof(float));    // 5 floats por vértice
    m.indices = malloc(m.indexCount * sizeof(unsigned int));   // índices simples

    memcpy(m.vertices, vertexs, m.vertexCount * 5 * sizeof(float));
    memcpy(m.indices, indexes, m.indexCount * sizeof(unsigned int));

    return m;
}
Mesh mesh_generate_sphere(int latDiv, int lonDiv) {
    Mesh m = {0};

    const float PI = 3.1415926f;
    int vertCount = (latDiv + 1) * (lonDiv + 1);
    int idxCount = latDiv * lonDiv * 6;

    m.vertexCount = vertCount;
    m.indexCount = idxCount;

    // 5 floats por vértice (3 pos + 2 UV)
    m.vertices = malloc(sizeof(float) * 5 * vertCount);
    m.indices  = malloc(sizeof(unsigned int) * idxCount);

    int vi = 0;
    for (int y = 0; y <= latDiv; ++y) {
        float v = (float)y / latDiv;
        float phi = v * PI;

        for (int x = 0; x <= lonDiv; ++x) {
            float u = (float)x / lonDiv;
            float theta = u * 2.0f * PI;

            float xpos = sinf(phi) * cosf(theta);
            float ypos = cosf(phi);
            float zpos = sinf(phi) * sinf(theta);

            m.vertices[vi++] = xpos;
            m.vertices[vi++] = ypos;
            m.vertices[vi++] = zpos;
            m.vertices[vi++] = u;
            m.vertices[vi++] = v;
        }
    }

    int ii = 0;
    for (int y = 0; y < latDiv; ++y) {
        for (int x = 0; x < lonDiv; ++x) {
            int i0 = y * (lonDiv + 1) + x;
            int i1 = i0 + 1;
            int i2 = i0 + (lonDiv + 1);
            int i3 = i2 + 1;

            m.indices[ii++] = i0;
            m.indices[ii++] = i2;
            m.indices[ii++] = i1;

            m.indices[ii++] = i1;
            m.indices[ii++] = i2;
            m.indices[ii++] = i3;
        }
    }

    // ✅ Tamaños reales de buffers (en bytes)
    m.vertexSize = vertCount * 5 * sizeof(float);     // 5 floats por vértice
    m.indexSize  = idxCount * sizeof(unsigned int);

    return m;
}


