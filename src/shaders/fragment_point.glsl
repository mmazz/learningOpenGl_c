#version 330 core
in float radius;
out vec4 FragColor;

void main()
{
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) / 0.5; // Normalizamos distancia al rango 0-1

    // Circulo difuso: alfa decrece desde centro a borde
    float alpha = max(0.5, 1.0 - smoothstep(0.1, 0.5, dist));
    if (dist > 0.5) discard;

    // Color azul con alfa para densidad
    FragColor = vec4(0.0, 0.3, 0.8, alpha);
}
