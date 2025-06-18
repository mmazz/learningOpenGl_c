#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// Parámetros de iluminación
uniform vec3 lightDir;     // dirección de la luz (por ejemplo vec3(0.5,1,0.3))
uniform vec3 lightColor;   // color / intensidad de la luz, p.e. vec3(1.0)
uniform vec3 objectColor;  // color de tu partícula, p.e. vec3(0.2,0.6,1.0)

void main() {
    // Iluminación difusa simple (Lambert)
    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 ambient  = 0.1 * lightColor;
    vec3 diffuse  = diff * lightColor;

    vec3 color = (ambient + diffuse) * objectColor;
    FragColor = vec4(color, 1.0);
}
