#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform float time;

// legacy water textures
uniform sampler2D textureSampler;
uniform sampler2D noiseSampler;
uniform sampler2D overlaySampler;

// reflection/refraction pipeline (your FBOs)
uniform sampler2D uReflection;
uniform sampler2D uRefraction;
uniform sampler2D uRefractionDepth;
uniform sampler2D uFoamNoise;

uniform float uNear;
uniform float uFar;
uniform float uWaterY;
uniform vec2  uScreenSize;

float LinearizeDepth(float d)
{
    float z = d * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

void main()
{
    // distortion from noise
    float n = texture(noiseSampler, vUV * 1.5 + vec2(time * 0.005, time * 0.008)).r;
    vec2 distortion = (n - 0.5) * 0.05;

    // base “lake-ish” tint mixed with your water texture
    vec3 texRGB = texture(textureSampler, vUV + distortion).rgb;
    vec3 deepWater    = vec3(0.05, 0.15, 0.35);
    vec3 shallowWater = vec3(0.10, 0.30, 0.40);
    vec3 waterTint = mix(deepWater, shallowWater, n);
    vec3 colorRGB  = mix(texRGB, waterTint, 0.5);

    // sparkle
    float sparkle = texture(overlaySampler, vUV * 4.0 + distortion + vec2(-time * 0.008, 0.0)).r;
    float highlight = pow(sparkle, 8.0) * 0.4;
    colorRGB += highlight;

    // foam from refraction depth
    float depth01 = texture(uRefractionDepth, vUV).r;
    float sceneDepth = LinearizeDepth(depth01);
    float foamMask = clamp(1.0 - (sceneDepth - 5.0) / 12.0, 0.0, 1.0);

    float fn = texture(uFoamNoise, vUV * 6.0 + vec2(time * 0.03, time * 0.02)).r;
    foamMask *= smoothstep(0.35, 0.75, fn);

    // reflection/refraction blend
    vec3 refl = texture(uReflection, vUV).rgb;
    vec3 refr = texture(uRefraction, vUV).rgb;
    vec3 rr   = mix(refr, refl, 0.35);

    rr = mix(rr, vec3(1.0), foamMask);   // foam to white
    rr = mix(rr, colorRGB, 0.6);         // mix in tinted base

    FragColor = vec4(rr, 0.82);
}
