#version 330 core

// Geometría de la esfera (unit sphere)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

// Datos por instancia
layout(location = 2) in vec3 aOffset;

uniform mat4 projection;
uniform mat4 view;
uniform float particleRadius;

out vec3 FragPos;    // posición en mundo para el fragment shader
out vec3 Normal;     // normal en mundo
out vec2 TexCoord;   // si quisieras texturizar

void main() {
    // Escalamos la esfera unitaria y desplazamos a la posición de esta instancia
    vec3 scaledPos = aPos * particleRadius;
    vec3 worldPos  = aOffset + scaledPos;

    // Pasamos la posición y normal
    FragPos  = worldPos;
    Normal   = normalize(aPos);  // en espacio local, unidad
    TexCoord = aUV;

    // Transformación de cámara + proyección
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

