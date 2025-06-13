#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform float pointSize;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize =2* pointSize;
}
