#version 330 core
out vec4 FragColor;

struct SpotLight {
    vec3 position;  
    vec3 direction;  
    float cutOff;
    float outerCutOff;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform SpotLight spotLight;
uniform sampler2D particleTexture;
uniform float time;

vec3 calculateBillboardLighting(SpotLight light, vec3 fragPos, vec3 viewDir) {
    // Simplified lighting calculation for billboards
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Combine components
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * max(dot(viewDir, lightDir), 0.0);
    
    return (ambient + diffuse) * attenuation * intensity;
}

void main() {
    // Sample particle texture with alpha test
    vec4 texColor = texture(particleTexture, TexCoords);
    if(texColor.a < 0.1) discard;

    // Billboard-specific normal calculation (facing camera)
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Simplified lighting calculation
    vec3 lighting = calculateBillboardLighting(spotLight, FragPos, viewDir);
    
    // Final color with alpha
    FragColor = texColor;
    // FragColor = vec4(lighting * texColor.rgb, texColor.a);
    
    // Optional: Additive blending
    // FragColor.rgb *= 2.0; // Uncomment for brighter particles
}