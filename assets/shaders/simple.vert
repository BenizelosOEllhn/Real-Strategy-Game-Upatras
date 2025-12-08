#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// Instance matrix (4 attribute slots: 3,4,5,6)
layout(location = 3) in mat4 instanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    // World position = model * instanceMatrix * local vertex
    vec4 worldPos = model * instanceMatrix * vec4(aPos, 1.0);

    FragPos           = worldPos.xyz;
    Normal            = mat3(transpose(inverse(model * mat4(mat3(instanceMatrix))))) * aNormal;
    TexCoords         = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
