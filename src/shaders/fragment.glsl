#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float mixer;
uniform float sinWave;

void main() {
    FragColor = mix(mix(texture(texture1, TexCoord), texture(texture2, TexCoord), mixer), vec4((ourColor.y+ourColor.z)*sinWave,ourColor.x+ourColor.z,ourColor.y+ourColor.x, 1.0), 0.2);
}