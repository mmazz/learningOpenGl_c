#include "glad/gl.h"
#include "shader.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <cglm/affine.h> // para funciones como glm_rotate, glm_scale
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdio.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float vertices2[] = {
    // positions          // colors           // texture coords
    0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
    -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
};
float vertices[] = {
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
unsigned int indices2[] = {
    // note that we start from 0!
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};
unsigned int indices[] = {
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

int main() {
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
    printf("Failed to create GLFW window");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD");
    return -1;
  }
  unsigned int VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // Indice del vertex, cantidad de elementos por vertice, tipo de datos,
  //

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //  Deje de usar coordenada de color
    //  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  //glEnableVertexAttribArray(0);
//  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
//                        (void *)(3 * sizeof(float)));
//  glEnableVertexAttribArray(1);
//
//  // For texture
//  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
//                        (void *)(6 * sizeof(float)));
//  glEnableVertexAttribArray(2);

  // Desbindea el VAO 0, que es el unico que cree. Es un ID que creo
  // glGenVertexArrays y es creciente.
  glBindVertexArray(0);

  // TODO: Entender por que si directamente hago un uint* y se lo poas al
  // compile_shader, luego al querer usar  en el link shader mediante
  // *vertexShaderID no me funca.
  const char *vertexFilePath = "src/shaders/vertex.glsl";
  unsigned int vertexShaderID;
  compile_shader(&vertexShaderID, GL_VERTEX_SHADER, vertexFilePath);

  const char *fragmentFilePath = "src/shaders/fragment.glsl";
  unsigned int fragmentShaderID;
  compile_shader(&fragmentShaderID, GL_FRAGMENT_SHADER, fragmentFilePath);

  GLuint shaderProgram = link_shader(vertexShaderID, fragmentShaderID);

  unsigned int texture1;
  const char *texture_file = "assets/wall.jpg";
  load_texture(&texture1, texture_file);
  unsigned int texture2;
  texture_file = "assets/awesomeface.png";
  load_texture(&texture2, texture_file);

  glUseProgram(shaderProgram);
  glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
  glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);

    //habilito el z-depth
  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {
    // input
    // -----
    processInput(window);

    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // borro no solo el buffer de color si no el depth buffer antes de cada dibujo
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glUseProgram(shaderProgram);
    mat4 view;
    glm_mat4_identity(view);
    // note that we're translating the scene in the reverse direction of where
    // we want to move
    glm_translate(view, (vec3){0.0f, 0.0f, -3.0f});
    mat4 projection;
    glm_perspective(glm_rad(45.0f), 800.0f / 600.0f, 0.1f, 100.0f, projection);

    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (float *)view);
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, (float *)projection);

    glBindVertexArray(VAO);

    for(unsigned int i = 0; i < 10; i++)
    {
        mat4 model;
        glm_mat4_identity(model);

        glm_translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        glm_rotate(model, (float)glfwGetTime() *glm_rad(angle), (vec3){1.0f, 0.3f, 0.5f});
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float *)model);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);


    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shaderProgram);
  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}
