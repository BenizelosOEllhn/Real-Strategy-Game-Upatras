/// ============================================================
/// Shockwave Refraction Shader
/// ============================================================
/// Used for the expanding sphere wave during explosion.
/// Creates a distortion effect using normal map sampling.
/// ============================================================

#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float vWaveAmplitude;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uExpansionFactor;  // 0 to 1, controls shockwave growth
uniform float uTime;

void main()
{
    // Expand sphere outward over time
    vec3 expandedPos = aPos * (1.0 + uExpansionFactor * 0.5);
    
    FragPos = vec3(model * vec4(expandedPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    // Wave amplitude decreases as expansion grows
    vWaveAmplitude = (1.0 - uExpansionFactor) * 0.3;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
