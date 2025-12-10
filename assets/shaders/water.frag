#version 330 core

in vec2 UV;
in vec3 WorldPos;
out vec4 color;

uniform float time;
uniform sampler2D textureSampler;  // Water texture
uniform sampler2D noiseSampler;    // Distortion
uniform sampler2D overlaySampler;  // Foam/Sparkle

void main()
{
    // 1. Slow, organic distortion
    // Sample noise at two different slow speeds
    vec2 flow1 = vec2(time * 0.005, time * 0.008); 
    vec2 flow2 = vec2(-time * 0.008, 0.0);

    vec2 noiseUV = UV * 1.5 + flow1;
    float noise = texture(noiseSampler, noiseUV).r;
    
    // 2. Base Color & Texture
    // Gentle distortion of the main texture
    vec2 distort = (vec2(noise) - 0.5) * 0.05;
    vec4 texColor = texture(textureSampler, UV + distort);

    // 3. Color Grading (Freshwater / Lake look)
    // Darker, greener blue for lakes/rivers usually looks more natural than tropical cyan
    vec3 deepWater = vec3(0.05, 0.15, 0.35);
    vec3 shallowWater = vec3(0.1, 0.3, 0.4);
    
    vec3 waterTone = mix(deepWater, shallowWater, noise);
    
    // Mix texture with procedural color
    vec3 finalRGB = mix(texColor.rgb, waterTone, 0.5);

    // 4. Specular Highlight (Fake Sun reflection)
    // Makes the water look wet/shiny without complex lighting math
    float sparkle = texture(overlaySampler, UV * 4.0 + distort + flow2).r;
    float highlight = pow(sparkle, 8.0) * 0.4; // Sharp highlights
    finalRGB += vec3(highlight);

    // 5. Transparency
    // Keep it somewhat transparent to see the river bed slightly
    color = vec4(finalRGB, 0.82); 
}