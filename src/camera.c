#include "camera.h"
#include <cglm/vec3.h>
#include <cglm/mat4.h>
#include <cglm/cam.h>
#include <math.h>

static void update_camera_vectors(Camera* camera) {
    vec3 front;
    front[0] = cosf(glm_rad(camera->Yaw)) * cosf(glm_rad(camera->Pitch));
    front[1] = sinf(glm_rad(camera->Pitch));
    front[2] = sinf(glm_rad(camera->Yaw)) * cosf(glm_rad(camera->Pitch));
    glm_vec3_normalize_to(front, camera->Front);

    glm_vec3_cross(camera->Front, camera->WorldUp, camera->Right);
    glm_vec3_normalize(camera->Right);

    glm_vec3_cross(camera->Right, camera->Front, camera->Up);
    glm_vec3_normalize(camera->Up);
}

Camera * camera_init(vec3 position, vec3 up, float yaw, float pitch) {
    Camera* camera = malloc(sizeof(Camera));
    if (!camera) {
        fprintf(stderr, "Failed to allocate memory for Camera\n");
        return NULL;
    }
    glm_vec3_copy(position, camera->Position);
    glm_vec3_copy(up, camera->WorldUp);
    camera->Yaw = yaw;
    camera->Pitch = pitch;

    camera->MovementSpeed = SPEED;
    camera->MouseSensitivity = SENSITIVITY;
    camera->Zoom = ZOOM;

    glm_vec3_zero(camera->Front);  // se sobreescribe en update
    update_camera_vectors(camera);
    return camera;
}

void camera_get_view_matrix(Camera* camera, mat4 dest) {
    vec3 center;
    glm_vec3_add(camera->Position, camera->Front, center);
    glm_lookat(camera->Position, center, camera->Up, dest);
}

void camera_process_keyboard(Camera* cam, Camera_Movement direction, float deltaTime) {
    float velocity = cam->MovementSpeed * deltaTime;
    vec3 scaled;

    switch (direction) {
        case CAMERA_FORWARD:
            glm_vec3_scale(cam->Front, velocity, scaled);
            glm_vec3_add(cam->Position, scaled, cam->Position);
            break;
        case CAMERA_BACKWARD:
            glm_vec3_scale(cam->Front, velocity, scaled);
            glm_vec3_sub(cam->Position, scaled, cam->Position);
            break;
        case CAMERA_LEFT:
            glm_vec3_scale(cam->Right, velocity, scaled);
            glm_vec3_sub(cam->Position, scaled, cam->Position);
            break;
        case CAMERA_RIGHT:
            glm_vec3_scale(cam->Right, velocity, scaled);
            glm_vec3_add(cam->Position, scaled, cam->Position);
            break;
    }
}

void camera_process_mouse(Camera* camera, float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= camera->MouseSensitivity;
    yoffset *= camera->MouseSensitivity;

    camera->Yaw += xoffset;
    camera->Pitch += yoffset;

    if (constrainPitch) {
        if (camera->Pitch > 89.0f) camera->Pitch = 89.0f;
        if (camera->Pitch < -89.0f) camera->Pitch = -89.0f;
    }

    update_camera_vectors(camera);
}

void camera_process_scroll(Camera* camera, float yoffset) {
    camera->Zoom -= yoffset;
    if (camera->Zoom < 1.0f) camera->Zoom = 1.0f;
    if (camera->Zoom > 45.0f) camera->Zoom = 45.0f;
}

