#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aRadius;

uniform mat4 projection;
uniform mat4 view;

out float radius;

void main()
{
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position = projection * viewPos;

    // Ajustar tamaño de punto para que mantenga tamaño "real" en pantalla
    float dist = -viewPos.z;
    gl_PointSize = aRadius / dist * 50.0; // Factor para escalar, ajustar a tu cámara
    radius = aRadius;
}
