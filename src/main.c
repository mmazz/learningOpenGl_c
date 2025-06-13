#include "glad/gl.h"
#include "shader.h"
#include "texture.h"
#include "camera.h"
#include "physics.h"
#include <GLFW/glfw3.h>
#include <cglm/affine.h> // para funciones como glm_rotate, glm_scale
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES    100000
#define STEP_RAIN        1000
#define SCR_WIDTH        800
#define SCR_HEIGHT       600
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679
#define SPHERE_RADIUS 1.5

static const vec3 BOX_MIN = {-1.0f, -1.0f, -1.0f};
static const vec3 BOX_MAX = { 1.0f,  1.0f,  1.0f};

static Particles particles[MAX_PARTICLES];
static vec3 positions_buff[MAX_PARTICLES];
static float radius_buff[MAX_PARTICLES];
static GLuint vao, vbo_pos, vbo_rad;

static char debugTitle[256];

static float deltaTime = 0.f;
static float lastFrame = 0.0f;

static inline void random_unit_pos(vec3 out) {
    out[0] = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
    out[1] = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
    out[2] = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
}

static float lastX = 800.0f / 2.0f;
static float lastY = 600.0f / 2.0f;
static bool firstMouse = true;
static bool constrainPitch = true;
static bool isABox = false;

#define SPHERE_LAT_DIV 10    // anillos de latitud
#define SPHERE_LON_DIV 10    // meridianos
#define SPHERE_PTS_PER_LAT 100
#define SPHERE_PTS_PER_LON 100

static vec3 sphereLatPoints[SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT];
static vec3 sphereLonPoints[SPHERE_LON_DIV * SPHERE_PTS_PER_LON];
static GLuint sphereLatVAO, sphereLatVBO;
static GLuint sphereLonVAO, sphereLonVBO;


#define BOX_EDGE_DIV 100  // puntos por arista
static vec3 boxEdgePoints[12 * BOX_EDGE_DIV];
static GLuint boxVAO, boxVBO;
void init_box_enviroment(const vec3 boxMin, const vec3 boxMax) {

    int idx = 0;
    // definimos las 8 esquinas
    vec3 corners[8] = {
        {boxMin[0], boxMin[1], boxMin[2]}, {boxMax[0], boxMin[1], boxMin[2]},
        {boxMax[0], boxMax[1], boxMin[2]}, {boxMin[0], boxMax[1], boxMin[2]},
        {boxMin[0], boxMin[1], boxMax[2]}, {boxMax[0], boxMin[1], boxMax[2]},
        {boxMax[0], boxMax[1], boxMax[2]}, {boxMin[0], boxMax[1], boxMax[2]}
    };
    // índices de los 12 bordes (pares de vértices)
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // base inferior
        {4,5},{5,6},{6,7},{7,4}, // base superior
        {0,4},{1,5},{2,6},{3,7}  // verticales
    };
    for (int e = 0; e < 12; ++e) {
        float *a = corners[edges[e][0]];  // puntero a vec3
        float *b = corners[edges[e][1]];
        for (int i = 0; i < BOX_EDGE_DIV; ++i) {
            float t = (float)i / (BOX_EDGE_DIV - 1);
            boxEdgePoints[idx][0] = a[0] + (b[0] - a[0]) * t;
            boxEdgePoints[idx][1] = a[1] + (b[1] - a[1]) * t;
            boxEdgePoints[idx][2] = a[2] + (b[2] - a[2]) * t;
            idx++;
        }
    }
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxEdgePoints), boxEdgePoints, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glBindVertexArray(0);
}
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

void render_box_debug(GLuint shader) {
    glUseProgram(shader);
    GLint sizeLoc  = glGetUniformLocation(shader, "pointSize");
    GLint colorLoc = glGetUniformLocation(shader, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 1.0f, 0.3f, 0.3f); // rojo suave
    glBindVertexArray(boxVAO);
    glDrawArrays(GL_POINTS, 0, 12 * BOX_EDGE_DIV);
    glBindVertexArray(0);
}

void render_sphere_debug(GLuint shaderEnviroment) {
    glUseProgram(shaderEnviroment);
    GLint sizeLoc  = glGetUniformLocation(shaderEnviroment, "pointSize");
    GLint colorLoc = glGetUniformLocation(shaderEnviroment, "overrideColor");
    if (sizeLoc >= 0)  glUniform1f(sizeLoc, 2.0f);
    if (colorLoc >= 0) glUniform3f(colorLoc, 0.3f, 0.3f, 1.0f);

    glBindVertexArray(sphereLatVAO);
    glDrawArrays(GL_POINTS, 0, SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT);
    // dibujar longitudes
    glBindVertexArray(sphereLonVAO);
    glDrawArrays(GL_POINTS, 0, SPHERE_LON_DIV * SPHERE_PTS_PER_LON);
    glBindVertexArray(0);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window, float deltaTime);
void update_buffers(GLuint vbo_pos, GLuint vbo_rad, int N);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
bool init_glad();
void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit);
void do_physics(Particles* spheres, double deltaTime, int num_spheres);
void init_particles_and_buffers(GLuint* vao, GLuint* vbo_pos, GLuint* vbo_rad);
void render(GLFWwindow* window, GLuint shader, GLuint shaderEnviroment, Camera *cam, int activeCount, bool isABox);

GLuint init_shader_program(const char *vertexPath, const char *fragmentPath);
void set_up_callbacks(GLFWwindow* window, Camera* camera);
GLFWwindow* setup_window(int width, int height, const char* title);


int main() {
    GLFWwindow* window = setup_window(SCR_WIDTH, SCR_HEIGHT, "Simulator");
    if (!window) {
        printf("No windows created");
        return -1;
    }
    if (!init_glad()) {
        printf("Error at glad init");
        return -1;
    }
    Camera camera = camera_init(cameraPos, cameraUp, YAW, PITCH);
    set_up_callbacks(window, &camera);
    init_particles_and_buffers(&vao, &vbo_pos, &vbo_rad);

    GLuint shaderProgram = init_shader_program("src/shaders/vertex_point.glsl", "src/shaders/fragment_point.glsl");
    GLuint shaderProgramEnviroment = init_shader_program("src/shaders/vertex_enviroment.glsl", "src/shaders/fragment_enviroment.glsl");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(isABox)
        init_box_enviroment(BOX_MIN, BOX_MAX);
    else
        init_sphere_enviroment(SPHERE_RADIUS);
    int activeCount = 0;
    float spawnTimer = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        spawnTimer += deltaTime;

        if (spawnTimer > 0.5f && activeCount < MAX_PARTICLES) {
            spawnTimer = 0.0f;
            activeCount = fmin(activeCount + STEP_RAIN, MAX_PARTICLES);
        }

        do_physics(particles, deltaTime, activeCount);
        update_buffers(vbo_pos, vbo_rad, activeCount);
        render(window, shaderProgram, shaderProgramEnviroment, &camera, activeCount, isABox);

        processInput(window, deltaTime);
        snprintf(debugTitle, sizeof(debugTitle),
                             "Mi Simulación — Partículas: %d  FPS: %.1f",
                             activeCount,
                             1.0 / deltaTime);
        glfwSetWindowTitle(window, debugTitle);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void init_particles_and_buffers(GLuint* vao, GLuint* vbo_pos, GLuint* vbo_rad) {
    srand((unsigned)time(NULL));
    for (int i = 0; i < MAX_PARTICLES; i++) {
        random_unit_pos(particles[i].position);
        glm_vec3_zero(particles[i].velocity);
        particles[i].radius = 0.05f;
    }

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    glGenBuffers(1, vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(vec3), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, vbo_rad);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo_rad);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}

// Quiero poder subir y bajar por z
void processInput(GLFWwindow* window, float deltaTime) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;  // el eje Y está invertido

    lastX = (float)xpos;
    lastY = (float)ypos;
   camera_process_mouse(camera, xoffset, yoffset, constrainPitch);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);
    camera_process_scroll(camera, yoffset);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void init_buffers(unsigned int *VAO, unsigned int *VBO, unsigned int *EBO, float *vertices, size_t vertices_size, unsigned int *indices, size_t indices_size) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);

    glBindVertexArray(*VAO);

    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

}

bool init_glad() {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }
    return true;
}

GLuint init_shader_program(const char *vertexPath, const char *fragmentPath) {
    GLuint vertexShader, fragmentShader;
    compile_shader(&vertexShader, GL_VERTEX_SHADER, vertexPath);
    compile_shader(&fragmentShader, GL_FRAGMENT_SHADER, fragmentPath);
    return link_shader(vertexShader, fragmentShader);
}

void update_buffers(GLuint vbo_pos, GLuint vbo_rad, int N) {
    for (int i = 0; i < N; i++) {
        glm_vec3_copy(particles[i].position, positions_buff[i]);
        radius_buff[i] = particles[i].radius * 100.0f;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(vec3), positions_buff);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_rad);
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(float), radius_buff);
}

void render(GLFWwindow* window, GLuint shader, GLuint shaderEnviroment, Camera *cam, int activeCount, bool isABox) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glUseProgram(shader);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    mat4 proj, view;
    glm_perspective(glm_rad(cam->Zoom), aspect, 0.1f, 100.0f, proj);
    camera_get_view_matrix(cam, view);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, (float*)view);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, activeCount);

    glUseProgram(shaderEnviroment);
    glUniformMatrix4fv(glGetUniformLocation(shaderEnviroment, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(shaderEnviroment, "view"), 1, GL_FALSE, (float*)view);
    glUniform3f(glGetUniformLocation(shaderEnviroment, "overrideColor"), 1.0f, 1.0f, 1.0f); // Blanco, por ejemplo
    glUniform1f(glGetUniformLocation(shaderEnviroment, "pointSize"), 3.0f);
    if(isABox){
        glBindVertexArray(boxVAO);
        glDrawArrays(GL_POINTS, 0, 12 * BOX_EDGE_DIV);
        glBindVertexArray(0);
    }
    else{
        glBindVertexArray(sphereLatVAO);
        glDrawArrays(GL_POINTS, 0, SPHERE_LAT_DIV * SPHERE_PTS_PER_LAT);
        // dibujar longitudes
        glBindVertexArray(sphereLonVAO);
        glDrawArrays(GL_POINTS, 0, SPHERE_LON_DIV * SPHERE_PTS_PER_LON);
        glBindVertexArray(0);
    }
}

void do_physics(Particles* particles, double deltaTime, int num_spheres){
    for (int i = 0; i < num_spheres; i++) {
        update_physics(&particles[i], SPHERE_RADIUS, deltaTime, BOX_MIN, BOX_MAX, isABox);
    }
    resolve_sphere_collisions(particles, num_spheres);
}

void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit) {
    load_texture(tex, path);  // Esta debe bindear y configurar GL_TEXTURE_2D, típicamente a GL_TEXTURE0 + textureUnit

    glUseProgram(shaderProgram);
    GLint location = glGetUniformLocation(shaderProgram, uniformName);
    if (location == -1) {
        fprintf(stderr, "Warning: uniform '%s' not found in shader\n", uniformName);
    } else {
        glUniform1i(location, textureUnit);
    }
}

GLFWwindow* setup_window(int width, int height, const char* title) {
    if (!glfwInit()) return NULL;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) { glfwTerminate(); return NULL; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);
    return window;
}

void set_up_callbacks(GLFWwindow* window, Camera* camera) {
    glfwSetWindowUserPointer(window, camera);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
