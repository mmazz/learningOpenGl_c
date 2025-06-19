#version 330 core
uniform vec3 overrideColor;
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0; // mapa de [0,1] a [-1,1]
    float r2 = dot(coord, coord);
    if (r2 > 1.0) discard;
    FragColor = vec4(overrideColor, 0.8);
}
