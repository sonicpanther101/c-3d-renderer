#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aInstancePos;

out vec2 TexCoords;
out vec3 FragPos;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float billboardScale;

void main() {
    vec3 position = aInstancePos + (cameraRight * aPos.x + cameraUp * aPos.y) * billboardScale;
    
    gl_Position = projection * view * vec4(position, 1.0);
    TexCoords = aTexCoords;
    FragPos = position;
}