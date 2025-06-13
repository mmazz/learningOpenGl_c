#version 330 core
uniform vec3 overrideColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(overrideColor, 1.0);
}
