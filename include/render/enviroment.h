#ifndef ENV_H
#define ENV_H
#include <cglm/cglm.h>
#include "glad/gl.h"
#include "core/config.h"
#include "render/camera.h"
#include <GLFW/glfw3.h>

void render_env(GLFWwindow* window, GLuint *shaderProgramEnviroment, Camera* camera, Config* config);
void init_box_environment(const Config* cfg);
void init_sphere_enviroment(const Config* cfg);
void render_sphere(GLuint shaderEnviroment, Config *config);
void render_box(GLuint shader, Config *config);
#endif
