#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

flat out vec3 Color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform vec3 u_objectColor;
uniform vec3 u_lightColor;
uniform vec3 u_lightPos;
uniform vec3 u_viewPos;

vec3 calculateLighting(vec3 Normal, vec3 FragPos) {
    // Ambient lighting
    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * u_lightColor;
    
    // Diffuse lighting
    vec3 lightDir = normalize(u_lightPos - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor;

    // Specular lighting
    float specularStrength = 0.3;
    vec3 viewDir = normalize(u_viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Normal);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * u_lightColor;
    
    return (ambient + diffuse + specular);
}

vec3 get_color(int r, int g, int b) {
    return vec3(r/255.0, g/255.0, b/255.0);
}

void main() {
    vec3 FragPos = vec3(u_model * vec4(aPos, 1.0));
    vec3 Normal = mat3(transpose(inverse(u_model))) * aNormal;
    
    vec3 lighting = calculateLighting(Normal, FragPos);
    
    Color = aColor * lighting;
    
    gl_Position = u_projection * u_view * vec4(FragPos, 1.0);
}
