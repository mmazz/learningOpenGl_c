#ifndef CONFIG_H
#define CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    ENV_BOX = 0,
    ENV_SPHERE = 1,
} EnvType;

typedef struct {
    unsigned int RENDER_PARTICLES;
    unsigned int INIT_PARTICLES;
    unsigned int STEP_PARTICLES;
    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;
    float ACCELERATION[3];
    float VISCOSITY[3];
    float PARTICLE_RADIUS;
    float ENV_SIZE;
    unsigned int ENV_DIV;
    EnvType ENV_TYPE;
} Config;

void trim(char* str);
int load_config(Config* cfg, const char* filename);
void print_config(const Config* cfg);

#endif
