#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 UV;
in float Radius;
flat in int InstanceIndex;

uniform vec3 color = vec3(1.0); // Optional: Add color uniformity

void main() {
    // Calculate distance from center (procedural circle)
    vec2 center = UV - vec2(0.5);
    float dist = length(center);
    if (dist > 0.5) discard; // Discard fragments outside the circle

    float indexFactor = 0.0;
    if (float(gl_VertexID) < 6) {
        indexFactor = 1.0;
    }
    vec3 dimmedColor = color * indexFactor;

    FragColor = vec4(dimmedColor, 1.0);
}