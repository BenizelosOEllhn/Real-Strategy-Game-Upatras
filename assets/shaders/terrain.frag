#version 410 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

// --- Grass Variations ---
uniform sampler2D grass1;
uniform sampler2D grass2;
uniform sampler2D grass3;
uniform sampler2D noiseDetail;

// --- Terrain Layers (Renamed to match Scene.cpp) ---
uniform sampler2D textureRock; // Was rockTex
uniform sampler2D texturePeak; // Was peakTex

// --- Shadows & Lighting ---
uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

// --- Terrain Config ---
uniform vec2 peakHeightRange;

// ----------------------------------------------------------
// Shadow Calculation (PCF)
// ----------------------------------------------------------
float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = 0.0015;
    float currentDepth = projCoords.z - bias;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    // 3x3 PCF
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// ----------------------------------------------------------
// Main
// ----------------------------------------------------------
void main()
{
    // ======================================================
    // 1. GRASS MIXING
    // ======================================================

    float grassTiling = 0.6;
    vec2 grassUV      = TexCoords * grassTiling;

    float noiseTiling = 0.4;
    float n = texture(noiseDetail, grassUV * noiseTiling).r;

    float w1 = smoothstep(0.0, 0.4, n);
    float w3_raw = smoothstep(0.6, 1.0, n);
    float w2 = clamp(1.0 - w1 - w3_raw, 0.0, 1.0);
    float w3 = clamp(1.0 - w1 - w2, 0.0, 1.0);

    vec3 g1 = texture(grass1, grassUV).rgb;
    vec3 g2 = texture(grass2, grassUV * 1.1).rgb;
    vec3 g3 = texture(grass3, grassUV * 0.9).rgb;

    vec3 blendedGrass = g1 * w1 + g2 * w2 + g3 * w3;

    float detail = texture(noiseDetail, grassUV * 3.0).r; 
    float detailFactor = 0.8 + detail * 0.3;              
    vec3 finalGrassColor = blendedGrass * detailFactor;


    // ======================================================
    // 2. TERRAIN LAYERING
    // ======================================================
    vec3 norm = normalize(Normal);
    vec3 up   = vec3(0.0, 1.0, 0.0);
    float slope = dot(norm, up); 

    // Steep => more rock, flat => more grass
    float blendAmount = smoothstep(0.3, 0.8, slope);

    // Use corrected uniform names here
    vec4 rockColor = texture(textureRock, TexCoords * 0.4);
    vec4 peakColor = texture(texturePeak, TexCoords * 0.25);

    vec3 baseColor = mix(rockColor.rgb, finalGrassColor, blendAmount);

    // Height-based snow
    float height = FragPos.y;
    float peakLerp = clamp((height - peakHeightRange.x) / max(0.001, peakHeightRange.y - peakHeightRange.x), 0.0, 1.0);
    peakLerp = smoothstep(0.0, 1.0, peakLerp);

    // Prefer snow on flatter tops
    float flatFactor = smoothstep(0.6, 1.0, slope); 
    float peakBlend  = peakLerp * flatFactor;

    vec3 objectColor = mix(baseColor, peakColor.rgb, peakBlend);


    // ======================================================
    // 3. LIGHTING & SHADOWS
    // ======================================================
    float ambientStrength = 0.22;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.12;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;

    float shadow = ShadowCalculation(FragPosLightSpace);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;

    FragColor = vec4(lighting, 1.0);
}