#include "glad/gl.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/enviroment.h"
#include "render/mesh.h"
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

static char debugTitle[256];

static float deltaTime = 0.f;
static float lastFrame = 0.0f;

static inline void random_position_for_env(vec3 out, const Config* cfg) {
    float padding = cfg->ENV_SIZE * 0.4f;
    float max_r = cfg->ENV_SIZE - padding;

    if (cfg->ENV_TYPE == ENV_BOX) {
        for (int i = 0; i < 3; i++) {
            out[i] = ((float)rand() / RAND_MAX) * (2.0f * max_r) - max_r;
        }
        return;
    }

    else if (cfg->ENV_TYPE == ENV_SPHERE) {
        while (1) {
            // Coordenadas x, y, z aleatorias en [-1,1] pero solo Y ≥ 0
            float x = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
            float y =       ((float)rand() / RAND_MAX);  // solo positivo
            float z = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;

            float r2 = x*x + y*y + z*z;
            if (r2 <= 1.0f) {
                // Distribución uniforme en el volumen usando raíz cúbica
                float scale = cbrtf((float)rand() / RAND_MAX);
                out[0] = x * max_r * scale;
                out[1] = y * max_r * scale;  // ya está en la parte superior
                out[2] = z * max_r * scale;
                return;
            }
        }
    }

    else {
        fprintf(stderr, "Tipo de entorno no soportado\n");
        out[0] = out[1] = out[2] = 0.0f;
    }
}

static float lastX = 800.0f / 2.0f;
static float lastY = 600.0f / 2.0f;
static bool firstMouse = true;
static bool constrainPitch = true;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
bool init_glad();
void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void do_physics(Config* config, Particles* particles, double deltaTime, int activeParticles);
void init_vertex_buffers(Config* config, GLuint* vaoPoint, GLuint* vaoMesh,
                         GLuint* meshVBO, GLuint* meshEBO, GLuint* instanceVBO,
                         GLuint* pointVBO, GLuint* shaderPoint, GLuint* shaderMesh);
void init_mesh_vao(GLuint* vaoMesh, GLuint* meshVBO, GLuint* meshEBO,
                   GLuint* instanceVBO, GLuint* shaderMesh);
void init_point_vao(GLuint* vaoPoint, GLuint* pointVBO, GLuint* shaderPoint);
void render(GLFWwindow* window, Config *config, GLuint shaderPoint, GLuint shaderMesh,
            GLuint vaoPoint, GLuint vaoMesh, Camera *cam, unsigned int activeCount);
void update_buffers(Config* config, GLuint *pointVBO, GLuint* instanceVBO, int N);
GLuint init_shader_program(const char *vertexPath, const char *fragmentPath);
void set_up_callbacks(GLFWwindow* window, Camera* camera);
GLFWwindow* setup_window(int width, int height, const char* title);

void processInputMovement(GLFWwindow* window, float deltaTime);
void init_particles(Particles* p, Config* config);
void init_particle_buffers(GLuint* vao, GLuint* vbo, GLuint* ebo, Config* config,  int N);
void update_particle_buffers(Config* config, Particles* particles, int N);
void change_env(Config* config);
void reinit_simulation(Config *config, bool resetAll);
Config config;
unsigned int activeCount = 0;
float spawnTimer = 0.0f;
GLuint shaderPoint, shaderMesh;
GLuint vaoPoint, vaoMesh, meshVBO, meshEBO, instanceVBO, pointVBO;
static GLuint indexCount = 0;  // para mesh
bool isPause = false;
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

    GLuint shaderProgramEnviroment = init_shader_program("shaders/vertex_enviroment.glsl", "shaders/fragment_enviroment.glsl");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    init_vertex_buffers(&config,& vaoPoint, &vaoMesh, &meshVBO, &meshEBO, &instanceVBO,
                         &pointVBO, &shaderPoint,  &shaderMesh);
    init_particles(particles, &config);
    switch (config.ENV_TYPE) {
        case ENV_BOX:
            init_box_environment(&config);
            break;
        case ENV_SPHERE:
            init_sphere_enviroment(&config);
            break;
        default:
            fprintf(stderr, "ENV_TYPE desconocido\n");
    }


    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        spawnTimer += deltaTime;

        if (!isPause && spawnTimer > 0.5f && activeCount <= config.RENDER_PARTICLES) {
            spawnTimer = 0.0f;
            activeCount = fmin(activeCount + config.STEP_PARTICLES, MAX_PARTICLES);
        }

        processInputMovement(window, deltaTime);
        do_physics(&config, particles, deltaTime, activeCount);
        update_buffers(&config, &pointVBO, &instanceVBO, activeCount);
        render(window, &config, shaderPoint,shaderMesh,
            vaoPoint, vaoMesh, &camera, activeCount);
        render_env(window, &shaderProgramEnviroment, &camera, &config);

        snprintf(debugTitle, sizeof(debugTitle),
                             "Mi Simulación — Partículas: %d  FPS: %.1f",
                             activeCount,
                             1.0 / deltaTime);
        glfwSetWindowTitle(window, debugTitle);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteProgram(shaderPoint);
    glDeleteProgram(shaderMesh);
    glDeleteProgram(shaderProgramEnviroment);
    glfwTerminate();
    return 0;
}

void change_env(Config* config){
    switch (config->ENV_TYPE) {
        case ENV_BOX:
            init_sphere_enviroment(config);
            config->ENV_TYPE = ENV_SPHERE;
            break;
        case ENV_SPHERE:
            init_box_environment(config);
            config->ENV_TYPE = ENV_BOX;
            break;
        default:
            fprintf(stderr, "ENV_TYPE desconocido\n");
    }
    reinit_simulation(config, false);
}
void init_particles(Particles* p, Config* config){
    srand((unsigned)time(NULL));
    for (int i = 0; i < MAX_PARTICLES; i++) {
        random_position_for_env(particles[i].current, config);
        glm_vec3_copy(particles[i].current, particles[i].previus);
        particles[i].radius = config->PARTICLE_RADIUS;
        glm_vec3_copy(config->ACCELERATION, p[i].acceleration);
    }
}

void init_point_vao(GLuint* vaoPoint, GLuint* pointVBO, GLuint* shaderPoint) {
    // 1) Generar VAO + VBO
    glGenVertexArrays(1, vaoPoint);
    glGenBuffers(1, pointVBO);

    // 2) Configurar VAO
    glBindVertexArray(*vaoPoint);
      glBindBuffer(GL_ARRAY_BUFFER, *pointVBO);
      glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(vec3), NULL, GL_DYNAMIC_DRAW);
      // atributo 0 = posición
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glBindVertexArray(0);

    // 3) Compilar shader
    *shaderPoint = init_shader_program("shaders/billboard.vertex",
                                      "shaders/billboard.frag");
}

void init_mesh_vao(GLuint* vaoMesh, GLuint* meshVBO, GLuint* meshEBO,
                   GLuint* instanceVBO, GLuint* shaderMesh) {
    // 1) Generar VAO + buffers
    glGenVertexArrays(1, vaoMesh);
    glGenBuffers(1, meshVBO);
    glGenBuffers(1, meshEBO);
    glGenBuffers(1, instanceVBO);

    // 2) Generar geometría de la esfera
    Mesh particle_shape = mesh_generate_sphere(20, 20);
    indexCount = particle_shape.indexCount;

    // 3) Configurar VAO
    glBindVertexArray(*vaoMesh);

      // 3a) VBO de la malla
      glBindBuffer(GL_ARRAY_BUFFER, *meshVBO);
      glBufferData(GL_ARRAY_BUFFER, particle_shape.vertexSize,
                   particle_shape.vertices, GL_STATIC_DRAW);
      // EBO de índices
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *meshEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, particle_shape.indexSize,
                   particle_shape.indices, GL_STATIC_DRAW);

      // atributos de la esfera
      // posición (location = 0)
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
      // UV (location = 1)
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

      // 3b) VBO de instancias
      glBindBuffer(GL_ARRAY_BUFFER, *instanceVBO);
      glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(vec3), NULL, GL_DYNAMIC_DRAW);
      // posición‑instancia (location = 2)
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
      glVertexAttribDivisor(2, 1);  // cambia por instancia

    glBindVertexArray(0);

    // 4) Compilar shader
    *shaderMesh = init_shader_program("shaders/vertexInstance.glsl",
                                     "shaders/fragmentInstance.glsl");
}

void init_vertex_buffers(Config* config, GLuint* vaoPoint, GLuint* vaoMesh,
                         GLuint* meshVBO, GLuint* meshEBO, GLuint* instanceVBO,
                         GLuint* pointVBO, GLuint* shaderPoint, GLuint* shaderMesh) {
    if (config->PARTICLE_TYPE == POINT_TYPE) {
        init_point_vao(vaoPoint, pointVBO, shaderPoint);
    } else if (config->PARTICLE_TYPE == MESH_TYPE) {
        init_mesh_vao(vaoMesh, meshVBO, meshEBO, instanceVBO, shaderMesh);
    }
}

void update_buffers(Config* config, GLuint* pointVBO, GLuint* instanceVBO, int N){
    float angle = glfwGetTime() * 0.1f;
    mat4 model;
    glm_mat4_identity(model);
    glm_rotate(model, angle, (vec3){0.0f, 1.0f, 0.0f});

    // —– Prepara tu array de posiciones —–
    for (int i = 0; i < N; i++) {
        vec4 pos4 = { particles[i].current[0], particles[i].current[1], particles[i].current[2], 1.0f };
        vec4 rotated;
        glm_mat4_mulv(model, pos4, rotated);
        glm_vec3_copy(rotated, positions_buff[i]);
    }
    // —– Elige el buffer correcto —–
    if (config->PARTICLE_TYPE == MESH_TYPE) {
        // instanced rendering: actualiza instanceVBO
        glBindBuffer(GL_ARRAY_BUFFER, *instanceVBO);
    }
    else { // POINT_TYPE
        // puntos: actualiza pointVBO
        glBindBuffer(GL_ARRAY_BUFFER, *pointVBO);
    }

    // —– Sube los datos al buffer activo —–
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(vec3), positions_buff);

    // —– Limpieza opcional —–
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void render(GLFWwindow* window, Config *config, GLuint shaderPoint, GLuint shaderMesh,
            GLuint vaoPoint, GLuint vaoMesh, Camera *cam, unsigned int activeCount) {
    // 1) Limpieza de pantalla
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // 2) Elegir shader y VAO
    const bool isPoint = (config->PARTICLE_TYPE == POINT_TYPE);
    GLuint shader = isPoint ? shaderPoint : shaderMesh;
    GLuint vao    = isPoint ? vaoPoint   : vaoMesh;

    glUseProgram(shader);

    // 3) Cálculos de cámara/común
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    float aspect = (float)fbW / (float)fbH;

    mat4 proj, view;
    glm_perspective(glm_rad(cam->Zoom), aspect, 0.1f, 100.0f, proj);
    camera_get_view_matrix(cam, view);
    // Uniforms comunes
    GLint locProj = glGetUniformLocation(shader, "projection");
    GLint locView = glGetUniformLocation(shader, "view");
    GLint locRad  = glGetUniformLocation(shader, "particleRadius");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, (float*)proj);
    glUniformMatrix4fv(locView, 1, GL_FALSE, (float*)view);
    glUniform1f(locRad, config->PARTICLE_RADIUS);

    // 4) Uniforms y estado específico de POINT
    if (isPoint) {
        float fovRad   = glm_rad(cam->Zoom);
        float pointScl = fbH / (2.0f * tanf(fovRad * 0.5f));
        GLint locPS    = glGetUniformLocation(shader, "pointScale");
        glUniform1f(locPS, pointScl);

        glEnable(GL_PROGRAM_POINT_SIZE);
    }
    else{
        glUniform3f(glGetUniformLocation(shaderMesh, "lightDir"),     0.5f, -1.0f, 0.3f);
        glUniform3f(glGetUniformLocation(shaderMesh, "lightColor"),   1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderMesh, "objectColor"),  0.2f, 0.6f, 1.0f);
    }    // 5) Dibujar
    glBindVertexArray(vao);
    if (isPoint) {
        glDrawArrays(GL_POINTS, 0, activeCount);
    } else {
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, activeCount);
    }
    glBindVertexArray(0);

    // 6) Restaurar estado
    if (isPoint) {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}


void reset_buffer_pos(bool resetAll){
    vec4 pos4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    if(resetAll){
        for (size_t i = 0; i < activeCount; i++) {
            glm_vec3_copy(pos4, positions_buff[i]);
        }
    }
    else{
        for (size_t i = 0; i < activeCount; i++) {
            glm_vec3_copy(pos4, positions_buff[i]);
        }
    }
}

void reinit_simulation(Config *config, bool resetAll){
    spawnTimer = 0.0f;
    reset_buffer_pos(resetAll);
    if(resetAll){
        activeCount = config->INIT_PARTICLES;
        init_particles(particles, config);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        isPause = !isPause;
    }
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        load_config(&config, "data/config.txt");
        reinit_simulation(&config, true);
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        change_env(&config);
    }
}

void processInputMovement(GLFWwindow* window, float deltaTime) {
    Camera* camera = glfwGetWindowUserPointer(window);
        float velocity = camera->MovementSpeed * deltaTime;

    bool space      = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool ctrl       = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
                      (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
    if (space) {
        if (ctrl) {
            camera->Position[1] -= velocity;
        } else {
            camera->Position[1] += velocity;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_FORWARD,  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_LEFT,     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_process_keyboard(camera, CAMERA_RIGHT,    deltaTime);

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
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
