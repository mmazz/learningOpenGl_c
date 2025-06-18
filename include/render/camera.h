#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>
#include <stdbool.h>

// Opciones de movimiento de cámara
typedef enum {
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT
} Camera_Movement;

// Valores por defecto
#define YAW         -90.0f
#define PITCH        0.0f
#define SPEED        2.5f
#define SENSITIVITY  0.1f
#define ZOOM         45.0f
static vec3 cameraPos   = {0.0f, 0.0f, 10.0f};
static vec3 cameraFront = {0.0f, 0.0f, -1.0f};
static vec3 cameraUp    = {0.0f, 1.0f,  0.0f};


typedef struct {
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
} Camera;

// Funciones públicas
Camera camera_init(vec3 position, vec3 up, float yaw, float pitch);
void camera_get_view_matrix(Camera* camera, mat4 dest);
void camera_process_keyboard(Camera* camera, Camera_Movement direction, float deltaTime);
void camera_process_mouse(Camera* camera, float xoffset, float yoffset, bool constrainPitch);
void camera_process_scroll(Camera* camera, float yoffset);

#endif
