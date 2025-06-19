#include "render/enviroment.h"
#define M_PI 3.14159265358979323846
#define M_PI_2 M_PI/2

static vec3*  envBoxPoints = NULL;
static GLuint envBoxVAO = 0, envBoxVBO = 0;
static size_t pointCount = 0;

static vec3* envSpherePoints = NULL;
static GLuint envSphereVAO = 0, envSphereVBO = 0;
static int spherePointCount = 0;

void render_env(GLFWwindow* window, GLuint *shaderProgram, Camera* camera, Config* config){
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    float angle = glfwGetTime() * 0.1f; // rotación continua con el tiempo
    mat4 model, proj, view;
    glm_mat4_identity(model);
    glm_rotate(model, angle, (vec3){0.0f, 1.0f, 0.0f});
    glm_perspective(glm_rad(camera->Zoom), aspect, 0.1f, 100.0f, proj);
    camera_get_view_matrix(camera, view);

    glUseProgram(*shaderProgram);

    GLint locModel = glGetUniformLocation(*shaderProgram, "model");
    glUniformMatrix4fv(locModel, 1, GL_FALSE, (float*)model);
    glUniformMatrix4fv(glGetUniformLocation(*shaderProgram, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(*shaderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniform3f(glGetUniformLocation(*shaderProgram, "overrideColor"), 1.0f, 1.0f, 1.0f); // Blanco, por ejemplo
    glUniform1f(glGetUniformLocation(*shaderProgram, "pointSize"), 10.0f);

    switch (config->ENV_TYPE) {
        case ENV_BOX:
            glBindVertexArray(envBoxVAO);
            glDrawArrays(GL_POINTS, 0, pointCount);
            break;
        case ENV_SPHERE:
            glBindVertexArray(envSphereVAO);
            glDrawArrays(GL_POINTS, 0, spherePointCount);
            break;
    }
    glBindVertexArray(0);
}


void init_box_environment(const Config* config) {
    // número total de puntos: 12 aristas * divisiones
    pointCount = 12 * config->ENV_DIV;

    // (re)aloca el buffer de puntos
    free(envBoxPoints);
    envBoxPoints = malloc(sizeof(vec3) * pointCount);
    if (!envBoxPoints) {
        fprintf(stderr, "Failed to alloc boxEdgePoints\n");
        exit(1);
    }
    float boxMin[3] = {-config->ENV_SIZE, -config->ENV_SIZE, -config->ENV_SIZE};
    float boxMax[3] = { config->ENV_SIZE,  config->ENV_SIZE,  config->ENV_SIZE};

    vec3 corners[8] = {
        {boxMin[0], boxMin[1], boxMin[2]},
        {boxMax[0], boxMin[1], boxMin[2]},
        {boxMax[0], boxMax[1], boxMin[2]},
        {boxMin[0], boxMax[1], boxMin[2]},
        {boxMin[0], boxMin[1], boxMax[2]},
        {boxMax[0], boxMin[1], boxMax[2]},
        {boxMax[0], boxMax[1], boxMax[2]},
        {boxMin[0], boxMax[1], boxMax[2]},
    };

    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    size_t idx = 0;
    for (int e = 0; e < 12; ++e) {
        vec3 a;
        glm_vec3_copy(corners[edges[e][0]],a);
        vec3 b;
        glm_vec3_copy(corners[edges[e][1]],b);
        for (unsigned int i = 0; i < config->ENV_DIV; ++i) {
            float t = (float)i / (config->ENV_DIV - 1);
            // interpolación lineal
            envBoxPoints[idx][0] = a[0] + (b[0] - a[0]) * t;
            envBoxPoints[idx][1] = a[1] + (b[1] - a[1]) * t;
            envBoxPoints[idx][2] = a[2] + (b[2] - a[2]) * t;
            idx++;
        }
    }

    if (envBoxVAO) glDeleteVertexArrays(1, &envBoxVAO);
    if (envBoxVBO) glDeleteBuffers(1, &envBoxVBO);

    glGenVertexArrays(1, &envBoxVAO);
    glGenBuffers(1, &envBoxVBO);
    glBindVertexArray(envBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, envBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * pointCount, envBoxPoints, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);
}


void init_sphere_enviroment(const Config* config) {
    // Radio de la esfera (igual que en tu renderizado)
    float R = 2.0f * config->ENV_SIZE;

    // Número de anillos (paralelos) que queremos
    int  numRings      = config->ENV_DIV/2;            // p. e. 10 anillos
    // Cuántos puntos por anillo (circunferencia)
    int  ptsPerRing    = config->ENV_DIV * 2;        // p. e. 20 puntos

    // Total de puntos
    spherePointCount   = numRings * ptsPerRing;

    // (Re)alocar el array
    free(envSpherePoints);
    envSpherePoints = malloc(sizeof(vec3) * spherePointCount);
    if (!envSpherePoints) {
        fprintf(stderr, "malloc envSpherePoints failed\n");
        exit(1);
    }

    int idx = 0;
    for (int ring = 0; ring < numRings; ++ring) {
        // v va de 0 a 1 a lo largo de latitud
        float v   = (float)ring / (numRings - 1);
        // phi ∈ [-π/2 .. +π/2]
        float phi = v * M_PI - M_PI_2;
        // Coordenada Y del anillo
        float y   = R * sinf(phi);
        // Radio del círculo en ese plano
        float r   = R * cosf(phi);

        // Generar los ptsPerRing puntos en este anillo
        for (int j = 0; j < ptsPerRing; ++j) {
            float u     = (float)j / ptsPerRing;            // ∈ [0..1)
            float theta = u * 2.0f * M_PI;                  // ∈ [0..2π)
            envSpherePoints[idx][0] = r * cosf(theta);
            envSpherePoints[idx][1] = y;
            envSpherePoints[idx][2] = r * sinf(theta);
            idx++;
        }
    }
    // Asegurarse idx == spherePointCount

    // (Re)crear VAO/VBO
    if (envSphereVAO) glDeleteVertexArrays(1, &envSphereVAO);
    if (envSphereVBO) glDeleteBuffers(1, &envSphereVBO);

    glGenVertexArrays(1, &envSphereVAO);
    glGenBuffers(1, &envSphereVBO);
    glBindVertexArray(envSphereVAO);
      glBindBuffer(GL_ARRAY_BUFFER, envSphereVBO);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(vec3) * spherePointCount,
                   envSpherePoints,
                   GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,
                            3,
                            GL_FLOAT,
                            GL_FALSE,
                            sizeof(vec3),
                            (void*)0);
    glBindVertexArray(0);
}

void render_box(GLuint shader, Config *config) {
    glUseProgram(shader);
    GLint sizeLoc  = glGetUniformLocation(shader, "pointSize");
    GLint colorLoc = glGetUniformLocation(shader, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 1.0f, 0.3f, 0.3f); // rojo suave
    glBindVertexArray(envBoxVAO);
    glDrawArrays(GL_POINTS, 0, 12 * config->ENV_DIV);
    glBindVertexArray(0);
}





void render_sphere(GLuint shaderEnvironment, Config *config) {
    glUseProgram(shaderEnvironment);
    GLint sizeLoc  = glGetUniformLocation(shaderEnvironment, "pointSize");
    GLint colorLoc = glGetUniformLocation(shaderEnvironment, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 0.3f, 0.3f, 1.0f);
    glBindVertexArray(envSphereVAO);
    glDrawArrays(GL_POINTS, 0, config->ENV_DIV*config->ENV_DIV);
    glBindVertexArray(0);
}

