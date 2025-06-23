#version 450 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 UV;
in float Radius;

uniform vec3 color = vec3(1.0); // Optional: Add color uniformity

void main() {
    // Calculate distance from center (procedural circle)
    vec2 center = UV - vec2(0.5);
    float dist = length(center);
    if (dist > 0.5) discard; // Discard fragments outside the circle

    FragColor = vec4(color, 1.0);
}