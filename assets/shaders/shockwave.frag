#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
flat in float vWaveAmplitude;

out vec4 FragColor;

uniform sampler2D normalMap;
uniform samplerCube envMap;  // For refraction

void main()
{
    // Sample normal map for distortion
    vec3 mapNormal = texture(normalMap, TexCoord).rgb;
    mapNormal = normalize(mapNormal * 2.0 - 1.0);
    
    // Blend with surface normal
    vec3 finalNormal = normalize(Normal + mapNormal * vWaveAmplitude);
    
    // Refraction direction (simplified)
    vec3 viewDir = normalize(FragPos);
    vec3 refractDir = refract(viewDir, finalNormal, 0.95);
    
    // Sample environment map with refracted direction
    vec4 refractColor = texture(envMap, refractDir);
    
    // Shockwave is semi-transparent white with blue tint
    vec4 shockwaveColor = vec4(0.3, 0.5, 1.0, 0.4);
    
    // Blend refraction with shockwave color
    FragColor = mix(refractColor, shockwaveColor, 0.6);
    
    // Fade amplitude
    FragColor.a *= vWaveAmplitude * 2.0;
}
