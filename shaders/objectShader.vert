#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec3 aOffset;

flat out vec3 flatColor;
out vec3 Color;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;
uniform vec3 u_viewPos;
uniform vec3 u_lightPos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_lightSpaceMatrix;
uniform sampler2D u_shadowMap;

vec3 calculateLighting(vec3 Normal, vec3 FragPos, float shadow) {
    // Ambient lighting
    vec3 ambient = light.ambient;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_lightPos - FragPos);
    float diff = max(dot(lightDir, norm), 0.0);
    vec3 diffuse = light.diffuse * diff;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(u_viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Normal);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = light.specular * spec;
    
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float calculateShadow(vec4 fragPosLightSpace) {
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to NDC
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(u_shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main() {
    vec3 FragPos = vec3(u_model * vec4(aPos + aOffset, 1.0));
//    vec3 Normal = aNormal;
    vec3 Normal = transpose(inverse(mat3(u_model))) * aNormal;
    
    vec4 fragPosLightSpace = u_lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = calculateShadow(fragPosLightSpace);
    
    vec3 lighting = calculateLighting(Normal, FragPos, shadow);
    Color = aColor * lighting;
    flatColor = Color;
    
    gl_Position = u_projection * u_view * u_model * vec4(aPos + aOffset, 1.0);
}
