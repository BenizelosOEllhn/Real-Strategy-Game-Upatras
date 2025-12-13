#version 410 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in float vClipDist;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

const float SHADOW_MAP_SIZE = 2048.0;

float computeShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = 0.0015;
    float texelSize = 1.0 / SHADOW_MAP_SIZE;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(shadowMap, projCoords.xy + offset).r;
            float currentDepth = projCoords.z - bias;
            shadow += currentDepth > closestDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

void main()
{
    if (vClipDist < 0.0) discard;
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;

    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff   = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    float specularStrength = 0.1;
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular   = specularStrength * spec * lightColor;

    float shadow = computeShadow(FragPosLightSpace);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    vec3 result = lighting * albedo;
    FragColor = vec4(result, 1.0);
}
