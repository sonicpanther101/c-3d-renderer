#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
}; 

struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
    // ambient light
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));

    // diffuse light
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));   

    // specular light
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // combined result
    vec3 result = ambient + diffuse + specular;
    // FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2) * vec4(result, 1.0);
    FragColor = vec4(result, 1.0);
}