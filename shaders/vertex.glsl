#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Matriz model por instancia (mat4 = 4 vec4)
layout (location = 2) in vec4 instanceModel0;
layout (location = 3) in vec4 instanceModel1;
layout (location = 4) in vec4 instanceModel2;
layout (location = 5) in vec4 instanceModel3;

out vec2 TexCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 model = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
