#version 330 core
in vec2 vUV;
in vec3 vWorldPos;
out vec4 FragColor;
in float vFade;

uniform float time;

// legacy textures
uniform sampler2D textureSampler;
uniform sampler2D noiseSampler;
uniform sampler2D overlaySampler;

// FBO pipeline
uniform sampler2D uReflection;
uniform sampler2D uRefraction;
uniform sampler2D uRefractionDepth;
uniform sampler2D uFoamNoise;

uniform float uNear;
uniform float uFar;

// water plane info
uniform float uWaterY;
uniform vec3  uViewPos;

// tuning
uniform float uWaveStrength;   // ocean:0.04 lake:0.02 river:0.015
uniform float uFoamStrength;   // ocean:1.0  lake:0.7  river:0.9
uniform float uBaseAlpha;      // ocean:0.85 lake:0.80 river:0.75

// world-space noise scaling (IMPORTANT for river banding)
uniform float uNoiseWorldScale; // ocean:0.015 lake:0.020 river:0.030
uniform float uNoiseSpeed;      // ocean:0.020 lake:0.015 river:0.010

// helpers
float LinearizeDepth(float d)
{
    float z = d * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

vec2 SafeUV(vec2 uv)
{
    return clamp(uv, vec2(0.001), vec2(0.999));
}

void main()
{
    // ------------------------------------------------------------
    // 1) World-space noise for distortion (removes river striping)
    // ------------------------------------------------------------
    vec2 worldUV = vWorldPos.xz * uNoiseWorldScale;
    float n = texture(noiseSampler, worldUV + vec2(time * uNoiseSpeed, time * (uNoiseSpeed * 0.7))).r;

    vec2 distortion = vec2(n - 0.5) * uWaveStrength;

    // apply distortion to everything (legacy + refl/refr)
    vec2 uvBase = SafeUV(vUV + distortion);
    vec2 uvRefl = SafeUV(vUV + distortion);
    vec2 uvRefr = SafeUV(vUV + distortion * 0.8);

    // ------------------------------------------------------------
    // 2) Base color (legacy water texture + tint)
    // ------------------------------------------------------------
    vec3 texRGB = texture(textureSampler, uvBase).rgb;

    vec3 deepWater    = vec3(0.05, 0.15, 0.35);
    vec3 shallowWater = vec3(0.10, 0.30, 0.40);
    vec3 waterTint    = mix(deepWater, shallowWater, n);

    vec3 colorRGB = mix(texRGB, waterTint, 0.55);

    // sparkle (optional)
    float sparkle   = texture(overlaySampler, SafeUV(vUV * 4.0 + distortion + vec2(-time * 0.01, 0.0))).r;
    float highlight = pow(sparkle, 8.0) * 0.25;
    colorRGB += highlight;

    // ------------------------------------------------------------
    // 3) Reflection / refraction sampling (distorted)
    // ------------------------------------------------------------
    vec3 refl = texture(uReflection, uvRefl).rgb;
    vec3 refr = texture(uRefraction, uvRefr).rgb;

    // ------------------------------------------------------------
    // 4) Foam from depth (correct sign)
    // ------------------------------------------------------------
    float sceneDepth01 = texture(uRefractionDepth, SafeUV(vUV)).r;

    float foamMask = 0.0;
    if (sceneDepth01 < 0.9999)
    {
        float sceneDepthLin = LinearizeDepth(sceneDepth01);
        float waterSurfLin  = LinearizeDepth(gl_FragCoord.z);

        // how much "stuff behind water" depth we have at this pixel
        float waterDepth = max(sceneDepthLin - waterSurfLin, 0.0);

        // foam strongest when shallow (small waterDepth)
        foamMask = 1.0 - smoothstep(0.15, 2.5, waterDepth);
        foamMask *= uFoamStrength;

        // breakup using world-space foam noise
        float fn = texture(uFoamNoise, SafeUV(worldUV * 6.0 + vec2(time * 0.10, time * 0.08))).r;
        foamMask *= smoothstep(0.35, 0.80, fn);
    }

    // ------------------------------------------------------------
    // 5) REAL Fresnel (needs vWorldPos + uViewPos)
    // ------------------------------------------------------------
    vec3 N = vec3(0.0, 1.0, 0.0);
    vec3 V = normalize(uViewPos - vWorldPos);

    float F0 = 0.02; // water base reflectance
    float cosTheta = clamp(dot(N, V), 0.0, 1.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // blend refr/refl by fresnel
    vec3 rr = mix(refr, refl, fresnel);

    // foam → white
    rr = mix(rr, vec3(1.0), foamMask * vFade);

    // tint overall
    rr = mix(rr, colorRGB, 0.60);

    // alpha: more opaque at grazing angles + foam
    float alpha = uBaseAlpha;
    alpha = mix(alpha * 0.85, alpha, fresnel); // slightly stronger at glancing angles
    alpha = clamp(alpha + foamMask * 0.25, 0.0, 1.0);
    alpha *= smoothstep(0.0, 1.0, vFade);

    FragColor = vec4(rr, alpha);
}
