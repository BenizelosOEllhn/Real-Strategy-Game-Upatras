#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

out vec2 UV;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main()
{
    // === CALM WAVE GENERATION ===
    // Much smaller multipliers (0.1, 0.05) for height to prevent clipping into land
    float wave1 = sin(inPos.x * 0.05 + time * 0.5) * 0.1;
    float wave2 = cos(inPos.z * 0.04 + time * 0.4) * 0.1;
    
    // Very subtle detail ripples
    float wave3 = sin((inPos.x + inPos.z) * 0.2 + time * 0.8) * 0.05;

    vec3 displaced = inPos;
    
    // Apply gentle height displacement
    displaced.y += wave1 + wave2 + wave3;

    // Remove the X/Z displacement (shoreline breaking) from before
    // because it causes the water to "slide" into the land too aggressively.
    
    UV = inUV + vec2(time * 0.005, time * 0.005); // Slow texture flow
    WorldPos = vec3(model * vec4(displaced, 1.0));

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}