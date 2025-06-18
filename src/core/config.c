#include "core/config.h"

void trim(char* str) {
    // Trim leading spaces
    while (isspace((unsigned char)*str)) memmove(str, str + 1, strlen(str));

    // Trim trailing spaces
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}
int load_config(Config* cfg, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Error opening config file");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 2) continue;

        char key[64], value[128];
        if (sscanf(line, "%63[^=]=%127[^\n]", key, value) != 2) continue;

        trim(key);
        trim(value);

        if (strcmp(key, "RENDER_PARTICLES") == 0) {
            cfg->RENDER_PARTICLES = (unsigned int)atoi(value);
        } else if (strcmp(key, "INIT_PARTICLES") == 0) {
            cfg->INIT_PARTICLES = (unsigned int)atoi(value);
        } else if (strcmp(key, "STEP_PARTICLES") == 0) {
            cfg->STEP_PARTICLES = (unsigned int)atoi(value);
        } else if (strcmp(key, "SCR_WIDTH") == 0) {
            cfg->SCR_WIDTH = (unsigned int)atoi(value);
        } else if (strcmp(key, "SCR_HEIGHT") == 0) {
            cfg->SCR_HEIGHT = (unsigned int)atoi(value);
        } else if (strcmp(key, "ACCELERATION") == 0) {
            sscanf(value, "%f %f %f", &cfg->ACCELERATION[0], &cfg->ACCELERATION[1], &cfg->ACCELERATION[2]);
        } else if (strcmp(key, "VISCOSITY") == 0) {
            sscanf(value, "%f %f %f", &cfg->VISCOSITY[0], &cfg->VISCOSITY[1], &cfg->VISCOSITY[2]);
        } else if (strcmp(key, "PARTICLE_RADIUS") == 0) {
            cfg->PARTICLE_RADIUS = strtof(value, NULL);
        }else if (strcmp(key, "ENV_SIZE") == 0) {
            cfg->ENV_SIZE = strtof(value, NULL);
        }else if (strcmp(key, "ENV_DIV") == 0) {
            cfg->ENV_DIV = (unsigned int)atoi(value);
        } else if (strcmp(key, "ENV_TYPE") == 0) {
            if (strcmp(value, "BOX") == 0)
                cfg->ENV_TYPE = ENV_BOX;
            else if (strcmp(value, "SPHERE") == 0)
                cfg->ENV_TYPE = ENV_SPHERE;
            else {
                fprintf(stderr, "Unknown ENV_TYPE: %s\n", value);
                cfg->ENV_TYPE = ENV_BOX; // default
            }
        } else if (strcmp(key, "PARTICLE_TYPE") == 0) {
            if (strcmp(value, "POINT") == 0)
                cfg->PARTICLE_TYPE= POINT_TYPE;
            else if (strcmp(value, "MESH") == 0)
                cfg->PARTICLE_TYPE= MESH_TYPE;
         //   else if (strcmp(value, "OBJ") == 0)
         //       cfg->PARTICLE_TYPE= OBJ_TYPE;
            else {
                fprintf(stderr, "Unknown ENV_TYPE: %s\n", value);
                cfg->PARTICLE_TYPE = POINT_TYPE; // default
            }
        }

    }

    fclose(f);
    return 1;
}

void print_config(const Config* cfg) {
    printf("RENDER_PARTICLES: %u\n", cfg->RENDER_PARTICLES);
    printf("INIT_PARTICLES: %u\n", cfg->INIT_PARTICLES);
    printf("STEP_PARTICLES: %u\n", cfg->STEP_PARTICLES);
    printf("SCR_WIDTH: %u\n", cfg->SCR_WIDTH);
    printf("SCR_HEIGHT: %u\n", cfg->SCR_HEIGHT);
    printf("ACCELERATION: %f %f %f\n", cfg->ACCELERATION[0], cfg->ACCELERATION[1], cfg->ACCELERATION[2]);
    printf("VISCOSITY: %f %f %f\n", cfg->VISCOSITY[0], cfg->VISCOSITY[1], cfg->VISCOSITY[2]);
    printf("PARTICLE_RADIUS: %f\n", cfg->PARTICLE_RADIUS);
    printf("ENV_RADIUS: %f\n", cfg->ENV_SIZE);
    printf("ENV_DIV: %u\n", cfg->ENV_DIV);
    printf("ENV_TYPE: %u\n", cfg->ENV_TYPE);
    printf("PARTICLE_TYPE: %u\n", cfg->PARTICLE_TYPE);
}

