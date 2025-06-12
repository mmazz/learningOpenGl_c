CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude
SRC = src/main.c src/gl.c src/shader.c src/texture.c src/camera.c src/mesh.c src/physics.c
OBJ := $(patsubst src/%.c, build/obj/%.o, $(SRC))
LIBS = -lglfw -ldl -lGL -lm
TARGET = main
EXECUTABLE = simulator
DIRS = build build/obj

.PHONY: clean

all: directories $(TARGET)

directories:
	@for dir in $(DIRS); do \
		if [ ! -d $$dir ]; then \
			echo "Creando $$dir"; \
			mkdir -p $$dir; \
		fi; \
	done

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o build/$(EXECUTABLE) $(OBJ) $(LIBS)


build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) build/$(EXECUTABLE)
