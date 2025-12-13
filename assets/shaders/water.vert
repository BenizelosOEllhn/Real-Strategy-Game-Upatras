#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main()
{
    // gentle vertical waves only
    float wave1 = sin(inPos.x * 0.05 + time * 0.5) * 0.1;
    float wave2 = cos(inPos.z * 0.04 + time * 0.4) * 0.1;
    float wave3 = sin((inPos.x + inPos.z) * 0.2 + time * 0.8) * 0.05;

    vec3 displaced = inPos;
    displaced.y += wave1 + wave2 + wave3;

    vUV = inUV + vec2(time * 0.005);
    gl_Position = projection * view * model * vec4(displaced, 1.0);
}
