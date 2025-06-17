#include "render/enviroment.h"
#define M_PI 3.14159265358979323846
#define M_PI_2 M_PI/2
//


static vec3*  envEdgePoints = NULL;
static GLuint envVAO = 0, envVBO = 0;
static size_t pointCount = 0;


void render_env(GLFWwindow* window, GLuint *shaderProgram, Camera* camera, Config* config){
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    mat4 proj, view;
    glm_perspective(glm_rad(camera->Zoom), aspect, 0.1f, 100.0f, proj);
    camera_get_view_matrix(camera, view);

    glUseProgram(*shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(*shaderProgram, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(*shaderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniform3f(glGetUniformLocation(*shaderProgram, "overrideColor"), 1.0f, 1.0f, 1.0f); // Blanco, por ejemplo
    glUniform1f(glGetUniformLocation(*shaderProgram, "pointSize"), 3.0f);
//    if(isABox){
        glBindVertexArray(envVAO);
        glDrawArrays(GL_POINTS, 0, 12 * config->ENV_DIV);
        glBindVertexArray(0);
 //   }
//    else{
//        glBindVertexArray(sphereLatVAO);
//        glDrawArrays(GL_POINTS, 0, SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT);
//        // dibujar longitudes
//        glBindVertexArray(sphereLonVAO);
//        glDrawArrays(GL_POINTS, 0, SPHERE_LON_DIV * SPHERE_PTS_PER_LON);
//        glBindVertexArray(0);
//    }
}
void init_box_environment(const Config* cfg) {
    // número total de puntos: 12 aristas * divisiones
    pointCount = 12 * cfg->ENV_DIV;

    // (re)aloca el buffer de puntos
    free(envEdgePoints);
    envEdgePoints = malloc(sizeof(vec3) * pointCount);
    if (!envEdgePoints) {
        fprintf(stderr, "Failed to alloc boxEdgePoints\n");
        exit(1);
    }
    float boxMin[3] = {-cfg->ENV_SIZE, -cfg->ENV_SIZE, -cfg->ENV_SIZE};
    float boxMax[3] = { cfg->ENV_SIZE,  cfg->ENV_SIZE,  cfg->ENV_SIZE};

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
        for (unsigned int i = 0; i < cfg->ENV_DIV; ++i) {
            float t = (float)i / (cfg->ENV_DIV - 1);
            // interpolación lineal
            envEdgePoints[idx][0] = a[0] + (b[0] - a[0]) * t;
            envEdgePoints[idx][1] = a[1] + (b[1] - a[1]) * t;
            envEdgePoints[idx][2] = a[2] + (b[2] - a[2]) * t;
            idx++;
        }
    }

    // (re)inicializar VAO/VBO
    if (envVAO) glDeleteVertexArrays(1, &envVAO);
    if (envVBO) glDeleteBuffers(1, &envVBO);

    glGenVertexArrays(1, &envVAO);
    glGenBuffers(1, &envVBO);
    glBindVertexArray(envVAO);
    glBindBuffer(GL_ARRAY_BUFFER, envVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec3) * pointCount,
                 envEdgePoints,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);
}

void render_box_debug(GLuint shader) {
    glUseProgram(shader);
    GLint sizeLoc  = glGetUniformLocation(shader, "pointSize");
    GLint colorLoc = glGetUniformLocation(shader, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 1.0f, 0.3f, 0.3f); // rojo suave
    glBindVertexArray(envVAO);
 //   glDrawArrays(GL_POINTS, 0, 12 * BOX_EDGE_DIV);
    glBindVertexArray(0);
}

#define SPHERE_LAT_DIV 20    // anillos de latitud
#define SPHERE_LON_DIV 0    // meridianos
#define SPHERE_PTS_PER_LAT 100
#define SPHERE_PTS_PER_LON 100

static vec3 sphereLatPoints[SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT];
static vec3 sphereLonPoints[SPHERE_LON_DIV * SPHERE_PTS_PER_LON];
static GLuint sphereLatVAO, sphereLatVBO;
static GLuint sphereLonVAO, sphereLonVBO;



void init_sphere_enviroment(float radius) {
    // generar latitudes
    int idx = 0;
    for (int i = 0; i < SPHERE_LAT_DIV; ++i) {
        float v = (float)i / (SPHERE_LAT_DIV - 1); // 0..1
        float phi = v * M_PI - M_PI_2;            // -pi/2..pi/2
        float y = radius * sinf(phi);
        float r = radius * cosf(phi);
        for (int j = 0; j < SPHERE_PTS_PER_LAT; ++j) {
            float u = (float)j / SPHERE_PTS_PER_LAT;
            float theta = u * 2.0f * M_PI;
            sphereLatPoints[idx][0] = r * cosf(theta);
            sphereLatPoints[idx][1] = y;
            sphereLatPoints[idx][2] = r * sinf(theta);
            idx++;
        }
    }
    // generar longitudes (meridianos)
    idx = 0;
    for (int i = 0; i < SPHERE_LON_DIV; ++i) {
        float u = (float)i / SPHERE_LON_DIV;
        float theta = u * 2.0f * M_PI;  // 0..2pi
        float cx = cosf(theta);
        float cz = sinf(theta);
        for (int j = 0; j < SPHERE_PTS_PER_LON; ++j) {
            float v = (float)j / (SPHERE_PTS_PER_LON - 1);
            float phi = v * M_PI - M_PI_2;
            float y = radius * sinf(phi);
            float r = radius * cosf(phi);
            sphereLonPoints[idx][0] = r * cx;
            sphereLonPoints[idx][1] = y;
            sphereLonPoints[idx][2] = r * cz;
            idx++;
        }
    }
    // lat VAO
    glGenVertexArrays(1, &sphereLatVAO);
    glGenBuffers(1, &sphereLatVBO);
    glBindVertexArray(sphereLatVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereLatVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereLatPoints), sphereLatPoints, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindVertexArray(0);
    // lon VAO
    glGenVertexArrays(1, &sphereLonVAO);
    glGenBuffers(1, &sphereLonVBO);
    glBindVertexArray(sphereLonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereLonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereLonPoints), sphereLonPoints, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindVertexArray(0);
}



void render_sphere_debug(GLuint shaderEnvironment) {
    glUseProgram(shaderEnvironment);
    GLint sizeLoc  = glGetUniformLocation(shaderEnvironment, "pointSize");
    GLint colorLoc = glGetUniformLocation(shaderEnvironment, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 0.3f, 0.3f, 1.0f);

    glBindVertexArray(sphereLatVAO);
    glDrawArrays(GL_POINTS, 0, SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT);
    // dibujar longitudes
    glBindVertexArray(sphereLonVAO);
    glDrawArrays(GL_POINTS, 0, SPHERE_LON_DIV * SPHERE_PTS_PER_LON);
    glBindVertexArray(0);
}

