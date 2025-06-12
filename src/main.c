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

#define MAX_PARTICLES    10000
#define STEP_RAIN        1000
#define SCR_WIDTH        800
#define SCR_HEIGHT       600

static const vec3 BOX_MIN = {-2.0f, -2.0f, -2.0f};
static const vec3 BOX_MAX = { 2.0f,  2.0f,  2.0f};

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

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window, float deltaTime);
void update_buffers(GLuint vbo_pos, GLuint vbo_rad, int N);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
bool init_glad();
void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit);
void do_physics(Particles* spheres, double deltaTime, int num_spheres);
void init_particles_and_buffers(GLuint* vao, GLuint* vbo_pos, GLuint* vbo_rad);
void render(GLuint shader, Camera *cam, int activeCount);

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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        render(shaderProgram, &camera, activeCount);

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

void render(GLuint shader, Camera *cam, int activeCount) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader);

    mat4 proj, view;
    glm_perspective(glm_rad(cam->Zoom), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 100.0f, proj);
    camera_get_view_matrix(cam, view);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, (float*)view);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, activeCount);
}

void do_physics(Particles* particles, double deltaTime, int num_spheres){
    for (int i = 0; i < num_spheres; i++) {
        update_physics(&particles[i], deltaTime, BOX_MIN, BOX_MAX);
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
