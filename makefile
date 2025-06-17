CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude
LIBS = -lglfw -ldl -lGL -lm

# Archivos fuente organizados por módulo
SRC = \
	src/main.c \
	src/core/gl.c \
	src/core/config.c \
	src/render/shader.c \
	src/render/mesh.c \
	src/render/camera.c \
	src/render/texture.c \
	src/render/enviroment.c \
	src/physics/physics.c

# Reglas para convertir src/... en build/obj/...
OBJ = $(patsubst src/%.c, build/obj/%.o, $(SRC))

TARGET = simulator
DIRS = build build/obj

.PHONY: all clean directories

all: directories build/$(TARGET)

directories:
	@for dir in $(DIRS); do \
		if [ ! -d $$dir ]; then \
			echo "Creando $$dir"; \
			mkdir -p $$dir; \
		fi; \
	done

# Regla principal de linkeo
build/$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compilación de cada .c en su .o
build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build/obj/*.o build/$(TARGET)

