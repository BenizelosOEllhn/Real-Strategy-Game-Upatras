#version 410 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform sampler2D textureGrass;
uniform sampler2D textureRock;
uniform sampler2D texturePeak;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec2 peakHeightRange;

// Adjust to your SHADOW_WIDTH if you change it in C++
const float SHADOW_MAP_SIZE = 2048.0;

// PCF soft shadow
float computeShadow(vec4 fragPosLightSpace)
{
    // Transform from clip space to [0,1] texture space
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside light frustum -> no shadow
    if (projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = 0.0015;               // tweak if acne / peter-panning
    float texelSize = 1.0 / SHADOW_MAP_SIZE;

    // 3x3 PCF kernel
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
    vec3 norm = normalize(Normal);
    vec3 up   = vec3(0.0, 1.0, 0.0);

    // --- Terrain texture blending (your style, kept intact) ---
    float slope = dot(norm, up);

    float blendAmount = clamp((slope - 0.7) * 10.0, 0.0, 1.0);
    vec4 grassColor   = texture(textureGrass, TexCoords);
    vec4 rockColor    = texture(textureRock,  TexCoords);
    vec4 peakColor    = texture(texturePeak,  TexCoords * 0.4);

    vec3 baseColor = mix(rockColor, grassColor, blendAmount).rgb;

    float height   = FragPos.y;
    float peakLerp = clamp((height - peakHeightRange.x) /
                           max(0.001, peakHeightRange.y - peakHeightRange.x),
                           0.0, 1.0);
    peakLerp = smoothstep(0.0, 1.0, peakLerp);

    float steepFactor = clamp((0.85 - slope) * 3.0, 0.0, 1.0);
    float peakBlend   = peakLerp * (0.4 + 0.6 * steepFactor);

    vec3 objectColor = mix(baseColor, peakColor.rgb, peakBlend);

    // --- Phong lighting ---
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);

    float ambientStrength = 0.2;
    vec3 ambient  = ambientStrength * lightColor;

    vec3 diffuse  = diff * lightColor;

    float specularStrength = 0.1;
    vec3 viewDir   = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec     = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular  = specularStrength * spec * lightColor;

    // --- Shadows (PCF) ---
    float shadow = computeShadow(FragPosLightSpace);

    // Ambient always visible, diffuse+spec under shadow
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    vec3 result = lighting * objectColor;
    FragColor = vec4(result, 1.0);
}
