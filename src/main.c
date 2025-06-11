#include "glad/gl.h"
#include "shader.h"
#include "texture.h"
#include "camera.h"
#include "mesh.h"
#include <GLFW/glfw3.h>
#include <cglm/affine.h> // para funciones como glm_rotate, glm_scale
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdio.h>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;



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


void processInput(GLFWwindow* window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
bool init_glad();
void init_buffers(unsigned int *VAO, unsigned int *VBO, unsigned int *EBO, float *vertices, size_t vertices_size, unsigned int *indices, size_t indices_size);
void init_texture(GLuint shaderProgram, GLuint *tex, const char *path, const char *uniformName, int textureUnit);

void render_scene(GLuint shaderProgram, Camera *camera, Mesh mesh);
GLuint init_shader_program(const char *vertexPath, const char *fragmentPath);
GLFWwindow *setup_window(int width, int height, const char* title, Camera* camera);

void del_buffers(Mesh* mesh);
int main() {
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
  //  GLuint VAO, VBO, EBO;
  //  Mesh cube = mesh_generate_cube();
    Mesh sphere = mesh_generate_sphere(20, 20);

   // init_buffers(&cube.VAO, &cube.VBO, &cube.EBO, cube.vertices, cube.vertexSize, cube.indices, cube.indexSize);
    init_buffers(&sphere.VAO, &sphere.VBO, &sphere.EBO, sphere.vertices, sphere.vertexSize, sphere.indices, sphere.indexSize);

    GLuint shaderProgram = init_shader_program("src/shaders/vertex.glsl", "src/shaders/fragment.glsl");
    GLuint texture1, texture2;
    init_texture(shaderProgram, &texture1, "assets/wall.jpg", "texture1", 0);
    init_texture(shaderProgram, &texture2, "assets/awesomeface.png", "texture2", 1);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
     //   render_scene(shaderProgram, &camera, cube.VAO, cube.EBO);
        render_scene(shaderProgram, &camera, sphere);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    //del_buffers(&cube);
    del_buffers(&sphere);


    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void del_buffers(Mesh* mesh){
    glDeleteVertexArrays(1, &mesh->VAO);
    glDeleteBuffers(1, &mesh->VBO);
    glDeleteBuffers(1, &mesh->EBO);
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

void render_scene(GLuint shaderProgram, Camera *camera, Mesh mesh) {
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

    // Modelado y dibujado
    glBindVertexArray(mesh.VAO);
    for (unsigned int i = 0; i < 10; i++) {
        mat4 model;
        glm_mat4_identity(model);
        glm_translate(model, cubePositions[i]);

        float angle = 20.0f * i;
        glm_rotate(model, glfwGetTime() * glm_rad(angle), (vec3){1.0f, 0.3f, 0.5f});
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, (float *)model);

        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
