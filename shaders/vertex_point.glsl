#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aRadius;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform float pointScale;

out float radius;
uniform float pointSize;

void main()
{
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position = projection * viewPos * model;
    // Ajustar tamaño de punto para que mantenga tamaño "real" en pantalla
    float dist = -viewPos.z;
    gl_PointSize = aRadius * pointScale / dist;
    radius = aRadius;
}
