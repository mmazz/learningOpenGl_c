#include "glad/gl.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/enviroment.h"
#include "render/camera.h"
#include "physics/physics.h"
#include "core/config.h"
#include <GLFW/glfw3.h>
#include <cglm/affine.h> // para funciones como glm_rotate, glm_scale
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES  1000000


static Particles particles[MAX_PARTICLES];
static vec3 positions_buff[MAX_PARTICLES];
static float radius_buff[MAX_PARTICLES];
static GLuint vao, vbo_pos, vbo_rad;

static char debugTitle[256];

static float deltaTime = 0.f;
static float lastFrame = 0.0f;


static inline void random_position_for_env(vec3 out, const Config* cfg) {
    float padding = cfg->ENV_SIZE * 0.4f;  // 10% del tamaño como margen

    while (1) {
        out[0] = ((float)rand() / RAND_MAX) * (2.0f * (cfg->ENV_SIZE - padding)) - (cfg->ENV_SIZE - padding);
        out[2] = ((float)rand() / RAND_MAX) * (2.0f * (cfg->ENV_SIZE - padding)) - (cfg->ENV_SIZE - padding);
        out[1] = ((float)rand() / RAND_MAX) * (cfg->ENV_SIZE - padding) + padding;
        //printf("%f,%f,%f\n", out[0],out[1],out[2]);
        if (cfg->ENV_TYPE == ENV_BOX) {
            return;
        } else if (cfg->ENV_TYPE == ENV_SPHERE) {
            float r2 = glm_vec3_norm2(out);
            if (r2 <= (cfg->ENV_SIZE - padding) * (cfg->ENV_SIZE - padding))
                return;
        } else {
            fprintf(stderr, "Tipo de entorno no soportado\n");
            return;
        }
    }
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

void do_physics(Config* config, Particles* particles, double deltaTime, int activeParticles);
void init_particles_and_buffers(GLuint* vao, GLuint* vbo_pos, GLuint* vbo_rad, Config* config);
void render(GLFWwindow* window, GLuint shader, GLuint shaderEnviroment, Camera *cam, unsigned int activeCount);

GLuint init_shader_program(const char *vertexPath, const char *fragmentPath);
void set_up_callbacks(GLFWwindow* window, Camera* camera);
GLFWwindow* setup_window(int width, int height, const char* title);

void init_particles(Particles* p, Config* config){
    for (int i = 0; i < MAX_PARTICLES; i++) {
        glm_vec3_copy(config->ACCELERATION, p[i].acceleration);
        radius_buff[i] = particles[i].radius;
    }
}

Config config;
unsigned int activeCount = 0;
float spawnTimer = 0.0f;

int main() {
    if (!load_config(&config, "data/config.txt")) {
        fprintf(stderr, "No se pudo cargar configuración\n");
        return 1;
    }
    if(config.INIT_PARTICLES > config.RENDER_PARTICLES && config.RENDER_PARTICLES > MAX_PARTICLES){
        fprintf(stderr, "No se puede iniciar con mas particulas que las maximas a renderizar ni superar el maximo de 1 000 000 particulas\n");
        return 1;
    }
    print_config(&config);
    GLFWwindow* window = setup_window(config.SCR_WIDTH, config.SCR_HEIGHT, "Simulator");
    if (!window) {
        printf("No windows created\n");
        return -1;
    }
    if (!init_glad()) {
        printf("Error at glad init\n");
        return -1;
    }
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Version:  %s\n", glGetString(GL_VERSION));
    Camera camera = camera_init(cameraPos, cameraUp, YAW, PITCH);
    set_up_callbacks(window, &camera);
    init_particles_and_buffers(&vao, &vbo_pos, &vbo_rad, &config);

    GLuint shaderProgram = init_shader_program("shaders/vertex_point.glsl", "shaders/fragment_point.glsl");
    GLuint shaderProgramEnviroment = init_shader_program("shaders/vertex_enviroment.glsl", "shaders/fragment_enviroment.glsl");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init_particles(particles, &config);
    switch (config.ENV_TYPE) {
        case ENV_BOX:
            init_box_environment(&config);
            break;
        case ENV_SPHERE:
            init_sphere_enviroment(config.ENV_SIZE);
            break;
        default:
            fprintf(stderr, "ENV_TYPE desconocido\n");
    }


    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        spawnTimer += deltaTime;

        if (spawnTimer > 0.5f && activeCount <= config.RENDER_PARTICLES) {
            spawnTimer = 0.0f;
            activeCount = fmin(activeCount + config.STEP_PARTICLES, MAX_PARTICLES);
        }

        do_physics(&config, particles, deltaTime, activeCount);
        update_buffers(vbo_pos, vbo_rad, activeCount);
        render(window, shaderProgram, shaderProgramEnviroment, &camera, activeCount);
        render_env(window, &shaderProgramEnviroment, &camera, &config);
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

void init_particles_and_buffers(GLuint* vao, GLuint* vbo_pos, GLuint* vbo_rad, Config* config) {
    srand((unsigned)time(NULL));
    for (int i = 0; i < MAX_PARTICLES; i++) {
        random_position_for_env(particles[i].current, config);
        glm_vec3_copy(particles[i].current, particles[i].previus);
        particles[i].radius = config->PARTICLE_RADIUS;
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
void reinit_simulation(Config *config){
    spawnTimer = 0.0f;
    activeCount = config->INIT_PARTICLES;
    init_particles_and_buffers(&vao, &vbo_pos, &vbo_rad, config);
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
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        load_config(&config, "data/config.txt");
        reinit_simulation(&config);
    }
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
        glm_vec3_copy(particles[i].current, positions_buff[i]);
        //radius_buff[i] = particles[i].radius;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(vec3), positions_buff);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_rad);
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(float), radius_buff);
}

void render(GLFWwindow* window, GLuint shader, GLuint shaderEnviroment, Camera *cam, unsigned int activeCount) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glUseProgram(shader);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    float aspect = (float)fbWidth / (float)fbHeight;
    mat4 proj, view;
    glm_perspective(glm_rad(cam->Zoom), aspect, 0.1f, 100.0f, proj);
    camera_get_view_matrix(cam, view);
    float fovRadians = glm_rad(cam->Zoom); // cam->Zoom en grados
    float pointScale = fbHeight / (2.0f * tanf(fovRadians / 2.0f));
    glUniform1f(glGetUniformLocation(shader, "pointScale"), pointScale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, (float*)view);
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, activeCount);
}

void do_physics(Config* config, Particles* particles, double deltaTime, int activeParticles){
    for (int i = 0; i < activeParticles; i++) {
        update_physics(config, &particles[i], deltaTime);
    }
    resolve_collisions(particles, activeParticles);
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
