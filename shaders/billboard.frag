#version 330 core
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0; // [-1,1]
    float r2 = dot(coord, coord);
    if (r2 > 1.0) discard; // fuera del círculo
    float depth = sqrt(1.0 - r2); // como una esfera
    vec3 normal = normalize(vec3(coord, depth));
    vec3 light = normalize(vec3(1.0, 1.0, 2.0));
    float diff = max(dot(normal, light), 0.0);
    FragColor = vec4(vec3(0.5) + diff * 0.5, 1.0);
}
