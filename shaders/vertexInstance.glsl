layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 iPos;   // instancia
layout (location = 3) in float iScale;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vec3 scaledPos = aPos * iScale;
    vec3 worldPos = scaledPos + iPos;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
