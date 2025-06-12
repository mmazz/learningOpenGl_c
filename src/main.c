#include "glad/gl.h"
#include "shader.h"
#include "texture.h"
#include "camera.h"
#include "mesh.h"
#include "physics.h"
#include <GLFW/glfw3.h>
#include <cglm/affine.h> // para funciones como glm_rotate, glm_scale
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES 10000
int StepRain = 1000;
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
vec3 boxMax = {2,2,2};
vec3 boxMin = {-2,-2,-2};

char debugTitle[256];
float deltaTime = 0.f;
float lastFrame = 0.f;

// Vectores de cámara
vec3 cameraPos   = {0.0f, 0.0f, 3.0f};
vec3 cameraFront = {0.0f, 0.0f, -1.0f};
vec3 cameraUp    = {0.0f, 1.0f,  0.0f};

// Campo de visión
static float lastX = 800.0f / 2.0f;
static float lastY = 600.0f / 2.0f;
static bool firstMouse = true;
static bool constrainPitch = false;

vec3 cubePositions[] = {
    {  0.0f,  0.0f,  0.0f },
    {  2.0f,  5.0f, -15.0f },
    { -1.5f, -2.2f, -2.5f },
    { -3.8f, -2.0f, -12.3f },
    {  2.4f, -0.4f, -3.5f },
    { -1.7f,  3.0f, -7.5f },
    {  1.3f, -2.0f, -2.5f },
    {  1.5f,  2.0f, -2.5f },
    {  1.5f,  0.2f, -1.5f },
    { -1.3f,  1.0f, -1.5f }
};

static vec3 positions_contiguous[MAX_PARTICLES];
static float radii[MAX_PARTICLES];


void processInput(GLFWwindow* window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
bool init_glad();
void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit);
void render_scene(GLuint shaderProgram, Camera *camera, Mesh mesh, int num_to_render, GLuint texture1, GLuint texture2);
GLuint init_shader_program(const char *vertexPath, const char *fragmentPath);
GLFWwindow *setup_window(int width, int height, const char* title, Camera* camera);

void init_instance(GLuint* VAO, GLuint* VBO,  GLuint* radiusVBO, SpherePhysics* spheres);
void randomPos(vec3 pos);
void render_scene_blur(GLuint shaderProgram, GLuint* VAO, GLuint* VBO, GLuint* radiusVBO, Camera *camera, SpherePhysics* spheres, int num_to_render);
void do_physics(SpherePhysics* spheres, double deltaTime, int num_spheres);

int main() {
    srand(time(NULL));
    Camera camera = camera_init(cameraPos, cameraUp, YAW, PITCH);
    GLFWwindow* window = setup_window(SCR_WIDTH, SCR_HEIGHT, "Simulator", &camera);
    if (!window) {
        printf("No windows created");
        return -1;
    }
    if (!init_glad()) {
        printf("Error at glad init");
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    SpherePhysics spheres[MAX_PARTICLES];
    GLuint VAO_points, VBO_points, radiusVBO;
    init_instance(&VAO_points, &VBO_points, &radiusVBO,  spheres);

    GLuint shaderProgram = init_shader_program("src/shaders/vertex_point.glsl", "src/shaders/fragment_point.glsl");

    double step = 0;
    int num_spheres = 0;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        step+=deltaTime;
        if(step>0.5 && num_spheres<MAX_PARTICLES){
            step=0;
            num_spheres+=StepRain;
        }
        do_physics(spheres, deltaTime, num_spheres);

        processInput(window, deltaTime);

        render_scene_blur(shaderProgram, &VAO_points, &VBO_points, &radiusVBO, &camera, spheres, num_spheres);
        snprintf(debugTitle, sizeof(debugTitle),
                             "Mi Simulación — Partículas: %d  FPS: %.1f",
                             num_spheres,
                             1.0 / deltaTime);
        glfwSetWindowTitle(window, debugTitle);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
void init_instance(GLuint* VAO, GLuint* VBO, GLuint* radiusVBO, SpherePhysics* spheres) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, radiusVBO);

    glBindVertexArray(*VAO);

    vec3 positions[MAX_PARTICLES] = {0};
    for (int i = 0; i < MAX_PARTICLES; i++) {
        randomPos(spheres[i].position);
        glm_vec3_copy(spheres[i].position, positions[i]);
        glm_vec3_zero(spheres[i].velocity);
        spheres[i].radius = 0.05f;
    }

    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(vec3), positions, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);

    float radii[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        radii[i] = spheres[i].radius * 10.0f;
    }

    glBindBuffer(GL_ARRAY_BUFFER, *radiusVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(float), radii, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void processInput(GLFWwindow* window, float deltaTime) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
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

    // Asume que el layout es: vec3 posición + vec2 texcoords
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

GLFWwindow *setup_window(int width, int height, const char* title, Camera* camera){
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (window == NULL) {
    printf("Failed to create GLFW window");
    glfwTerminate();
    return NULL;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); //Usá V-Sync para que GLFW sincronice el swap de buffers con la frecuencia de actualización del monitor
  glfwSetWindowUserPointer(window, camera);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  return window;
}

GLuint init_shader_program(const char *vertexPath, const char *fragmentPath) {
    GLuint vertexShader, fragmentShader;
    compile_shader(&vertexShader, GL_VERTEX_SHADER, vertexPath);
    compile_shader(&fragmentShader, GL_FRAGMENT_SHADER, fragmentPath);
    return link_shader(vertexShader, fragmentShader);
}


void render_scene_blur(GLuint shaderProgram, GLuint* VAO, GLuint* VBO, GLuint* radiusVBO, Camera *camera, SpherePhysics* spheres, int num_to_render) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // Proyección y vista
    mat4 projection;
    glm_perspective(glm_rad(camera->Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f, projection);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, (float *)projection);

    mat4 view;
    camera_get_view_matrix(camera, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, (float *)view);

    // === ACTUALIZAR POSICIONES ===
    for (int i = 0; i < MAX_PARTICLES; i++) {
        glm_vec3_copy(spheres[i].position, positions_contiguous[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, MAX_PARTICLES * sizeof(vec3), positions_contiguous);

    // === ACTUALIZAR RADIOS ===
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        radii[i] = spheres[i].radius * 100.0f;  // Escalado a píxeles (ajustá el factor si querés)
    }
    glBindBuffer(GL_ARRAY_BUFFER, *radiusVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, MAX_PARTICLES * sizeof(float), radii);

    // === DIBUJAR ===
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(*VAO);
    glDrawArrays(GL_POINTS, 0, num_to_render);
    glBindVertexArray(0);
}

void randomPos(vec3 pos) {
    pos[0] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    pos[1] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    pos[2] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

void do_physics(SpherePhysics* spheres, double deltaTime, int num_spheres){
    for (int i = 0; i < num_spheres; i++) {
        update_physics(&spheres[i], deltaTime, boxMin, boxMax);
    }

    // Chequear y resolver colisiones entre esferas
    resolve_sphere_collisions(spheres, num_spheres);

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
