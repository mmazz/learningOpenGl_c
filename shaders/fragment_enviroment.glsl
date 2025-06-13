#version 330 core
uniform vec3 overrideColor;
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) / 0.5; // Normalizamos distancia al rango 0-1


    if (dist > 0.5) discard;
    float alpha = max(0.5, 1.0 - smoothstep(0.1, 0.5, dist));
    FragColor = vec4(overrideColor, alpha);
}
