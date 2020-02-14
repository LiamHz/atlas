#version 330 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 u_objectColor;
uniform vec3 u_lightColor;
uniform vec3 u_lightPos;
uniform vec3 u_viewPos;

void main() {
    // Ambient lighting
    float ambientStrength = 0.7;
    vec3 ambient = ambientStrength * u_lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(u_viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * u_lightColor;
    
    vec3 result = (ambient + diffuse + specular) * u_objectColor;
    FragColor = vec4(result, 1.0);
}
