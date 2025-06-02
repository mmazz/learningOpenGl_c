#ifndef SHADER_H
#define SHADER_H

#include <stddef.h>
#include "glad/gl.h"
#include <stdlib.h>
#include <stdio.h>

const char* get_shader_content(const char* fileName);
void compile_shader(GLuint* shaderId, GLenum shaderType, const char* shaderFilePath);
GLuint link_shader(GLuint vertexShaderID, GLuint fragmentShaderID);

#endif
