#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in float inFade;

out vec2 vUV;
out vec3 vWorldPos;
out float vFade;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

uniform float uVertexWaveAmp;     // ocean 0.10, lake 0.05, river 0.02
uniform float uVertexWaveFreq;    // ocean 0.05, lake 0.06, river 0.10
uniform float uVertexWaveSpeed;   // ocean 0.50, lake 0.40, river 0.25

void main()
{
    // vertex waves (subtle, you can even set amp=0 to disable)
    float wave1 = sin(inPos.x * uVertexWaveFreq + time * uVertexWaveSpeed) * (uVertexWaveAmp);
    float wave2 = cos(inPos.z * (uVertexWaveFreq * 0.8) + time * (uVertexWaveSpeed * 0.8)) * (uVertexWaveAmp);
    float wave3 = sin((inPos.x + inPos.z) * (uVertexWaveFreq * 2.0) + time * (uVertexWaveSpeed * 1.4)) * (uVertexWaveAmp * 0.5);

    vec3 displaced = inPos;
    displaced.y += wave1 + wave2 + wave3;

    vec4 world = model * vec4(displaced, 1.0);
    vWorldPos = world.xyz;
    vFade = inFade;
    
    // keep UV mostly stable; only tiny drift
    vUV = inUV;

    gl_Position = projection * view * world;
}
