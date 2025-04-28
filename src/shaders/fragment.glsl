#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 UV;

uniform vec3 color; // Optional: Add color uniformity

void main() {
    // Calculate distance from center (procedural circle)
    vec2 center = UV - vec2(0.5);
    float dist = length(center);
    if (dist > 0.5) discard; // Discard fragments outside the circle

    // Smooth edges (optional)
    // float smoothness = 0.02;
    // float alpha = 1.0 - smoothstep(0.5 - smoothness, 0.5, dist);

    // Solid color (replace with your logic)
    FragColor = vec4(color, 1.0); // White circles
}