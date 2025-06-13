#include "render/shader.h"

const char* get_shader_content(const char* fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL) {
        return NULL;
    }

    // Ir al final para obtener tamaño
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp); // volver al principio

    if (size <= 0) {
        fclose(fp);
        return NULL;
    }

    // Reservar memoria (+1 para '\0')
    char* content = malloc(size + 1);
    if (content == NULL) {
        fclose(fp);
        return NULL;
    }

    // Leer contenido
    size_t read_size = fread(content, 1, size, fp);
    fclose(fp);
    content[read_size] = '\0'; // aseguramos null-terminación

    return (const char*)content;
}

void compile_shader(GLuint* shaderId, GLenum shaderType, const char* shaderFilePath)
{
    GLint isCompiled = 0;
    const char* shaderSource = get_shader_content(shaderFilePath);

    if (shaderSource == NULL) {
        fprintf(stderr, "Error: could not read shader file: %s\n", shaderFilePath);
        *shaderId = 0;
        return;
    }

    *shaderId = glCreateShader(shaderType);
    if (*shaderId == 0) {
        fprintf(stderr, "Error: glCreateShader failed for %s\n", shaderFilePath);
        free((void*)shaderSource);
        return;
    }

    glShaderSource(*shaderId, 1, &shaderSource, NULL);
    glCompileShader(*shaderId);

    glGetShaderiv(*shaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(*shaderId, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Shader compilation failed (%s):\n%s\n", shaderFilePath, infoLog);
        glDeleteShader(*shaderId);
        *shaderId = 0;
    }

    free((void*)shaderSource); // liberar memoria reservada por get_shader_content
}

GLuint link_shader(GLuint vertexShaderID, GLuint fragmentShaderID)
{
    GLuint programID = glCreateProgram();
    if (programID == 0) {
        fprintf(stderr, "Error: glCreateProgram() failed\n");
        return 0;
    }

    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    GLint isLinked = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

        char* infoLog = malloc(maxLength);
        if (infoLog) {
            glGetProgramInfoLog(programID, maxLength, NULL, infoLog);
            fprintf(stderr, "Shader Program Link Error:\n%s\n", infoLog);
            free(infoLog);
        }

        glDeleteProgram(programID);
        glDeleteShader(vertexShaderID);
        glDeleteShader(fragmentShaderID);
        return 0;
    }

    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}

