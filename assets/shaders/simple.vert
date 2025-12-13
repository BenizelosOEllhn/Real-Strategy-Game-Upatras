#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Instance matrix rows (matches your C++ attrib pointers 3..6)
layout (location = 3) in vec4 iRow0;
layout (location = 4) in vec4 iRow1;
layout (location = 5) in vec4 iRow2;
layout (location = 6) in vec4 iRow3;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform vec4 uClipPlane;
out float vClipDist;

void main()
{
    mat4 instanceModel = mat4(iRow0, iRow1, iRow2, iRow3);

    vec4 worldPos = instanceModel * vec4(aPos, 1.0);

    vClipDist = dot(worldPos, uClipPlane);

    FragPos = worldPos.xyz;
    Normal  = mat3(transpose(inverse(instanceModel))) * aNormal;
    TexCoords = aTexCoords;

    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
