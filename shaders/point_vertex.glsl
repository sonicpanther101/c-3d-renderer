#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aInstancePos;

out vec2 UV;
out vec3 FragPos;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main() {
    vec3 position = aInstancePos + (cameraRight * aPos.x + cameraUp * aPos.y) * 0.2;
    gl_Position = projection * view * vec4(position, 1.0);
    UV = aPos + vec2(0.5); // Convert from [-0.5, 0.5] to [0, 1]
    FragPos = position;
}