#version 410 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in float vClipDist;


// --- Grass Variations ---
uniform sampler2D grass1;
uniform sampler2D grass2;
uniform sampler2D grass3;
uniform sampler2D noiseDetail;

// --- Terrain Layers ---
uniform sampler2D textureRock;
uniform sampler2D texturePeak;
uniform sampler2D sandTex;
uniform sampler2D scorchedEarthTex;  // Crater scorch marks
uniform sampler2D craterMaskTex;
uniform vec2 uTerrainOrigin;
uniform vec2 uTerrainSize;
const int MAX_CRATERS = 64;
uniform int uCraterCount;
// x=center.x, y=center.z, z=texture radius in world units, w=unused
uniform vec4 uCraters[MAX_CRATERS];

// --- Shadows & Lighting ---
uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

// (no need for peakHeightRange anymore)
// uniform vec2 peakHeightRange;

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
            float pcfDepth = texture(shadowMap,
                                     projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

float CraterMaskFromUniforms(vec2 worldXZ)
{
    float mask = 0.0;
    for (int i = 0; i < uCraterCount; ++i)
    {
        vec4 crater = uCraters[i];
        vec2 d = worldXZ - crater.xy;
        float radius = max(crater.z, 0.001);
        float distSq = dot(d, d);
        float rSq = radius * radius;
        if (distSq > rSq)
            continue;

        float falloff = 1.0 - (distSq / rSq);
        falloff = pow(falloff, 0.62);
        float strength = 0.08 + falloff * 0.92;
        mask = max(mask, strength);
    }
    return mask;
}

// ----------------------------------------------------------
// Main
// ----------------------------------------------------------
void main()
{
    if (vClipDist < 0.0) discard;
    // ======================================================
    // 1. GRASS MIXING
    // ======================================================
    float grassTiling = 0.15;
    vec2 grassUV      = TexCoords * grassTiling;

    float noiseTiling = 0.4;
    float n = texture(noiseDetail, grassUV * noiseTiling).r;

    float w1     = smoothstep(0.0, 0.4, n);
    float w3_raw = smoothstep(0.6, 1.0, n);
    float w2     = clamp(1.0 - w1 - w3_raw, 0.0, 1.0);
    float w3     = clamp(1.0 - w1 - w2,    0.0, 1.0);

    vec3 g1 = texture(grass1, grassUV).rgb;
    vec3 g2 = texture(grass2, grassUV * 1.1).rgb;
    vec3 g3 = texture(grass3, grassUV * 0.9).rgb;

    vec3 blendedGrass = g1 * w1 + g2 * w2 + g3 * w3;

    float detail       = texture(noiseDetail, grassUV * 3.0).r;
    float detailFactor = 0.8 + detail * 0.3;
    vec3 finalGrassColor = blendedGrass * detailFactor;

//------------------------------------------------------
// HARD BIOME LAYERS (no smoothing, no blending)
//------------------------------------------------------
float h = FragPos.y;

// Final color output
vec3 col;

// ---- HARD SAND ----
if (h < 1.2)
{
    col = texture(sandTex, TexCoords * 0.4).rgb;
}
// ---- HARD GRASS ----
else if (h < 14.0)
{
    col = finalGrassColor;
}
// ---- HARD ROCK ----
else if (h < 28.0)
{
    col = texture(textureRock, TexCoords * 0.35).rgb;
}
// ---- HARD PEAK ----
else
{
    col = texture(texturePeak, TexCoords * 0.25).rgb;
}

// Optional rock override for steep slopes:
float slope = dot(normalize(Normal), vec3(0.0, 1.0, 0.0));
if (slope < 0.55)  // lower = steeper
{
    vec3 rockColor = texture(textureRock, TexCoords * 0.35).rgb;
    col = rockColor;
}

vec2 craterUV = (FragPos.xz - uTerrainOrigin) / uTerrainSize;
float craterMask = texture(craterMaskTex, craterUV).r;
craterMask = max(craterMask, CraterMaskFromUniforms(FragPos.xz));
if (craterMask > 0.001) {
    vec2 scorchUV = FragPos.xz * 0.08 + vec2(0.37, 0.19);
    vec3 scorchCol = texture(scorchedEarthTex, scorchUV).rgb;
    float scorchBlend = smoothstep(0.24, 0.84, craterMask);
    col = mix(col, scorchCol, scorchBlend);
}

    vec3 norm = normalize(Normal);

    // ======================================================
    // 3. LIGHTING & SHADOWS
    // ======================================================
    float ambientStrength = 0.22;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;

    float specularStrength = 0.12;
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular   = specularStrength * spec * lightColor;

    float shadow = ShadowCalculation(FragPosLightSpace);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * col;

    FragColor = vec4(lighting, 1.0);
}
