#version 330 core

out vec4 FragColor;

void main() {
    // gl_PointCoord va de [0,1] en cada eje, lo mapeamos a [-1,1]
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);

    // Discard fuera del círculo
    if (r2 > 1.0) discard;

    // Normal de la “esfera” ficticia
    float z = sqrt(1.0 - r2);
    vec3 normal = normalize(vec3(coord, z));

    // Luz fija
    vec3 lightDir  = normalize(vec3(1.0, 1.0, 2.0));
    vec3 ambient   = vec3(0.3);
    float diff     = max(dot(normal, lightDir), 0.0);
    vec3 diffuse   = diff * vec3(0.7);

    // Color final
    vec3 color = ambient + diffuse;
    FragColor = vec4(color, 1.0);
}
