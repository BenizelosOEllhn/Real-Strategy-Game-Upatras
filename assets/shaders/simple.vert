#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Instance attributes (Used only for Trees/Rocks)
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

// --- NEW UNIFORMS ---
uniform mat4 model;         // For single buildings
uniform bool isInstanced;   // Switch: True=Trees, False=Buildings
uniform vec4 uClipPlane;

out float vClipDist;

void main()
{
    mat4 finalModel;

    if (isInstanced) {
        finalModel = mat4(iRow0, iRow1, iRow2, iRow3);
    } else {
        finalModel = model;
    }

    vec4 worldPos = finalModel * vec4(aPos, 1.0);
    vClipDist = dot(worldPos, uClipPlane);

    FragPos = worldPos.xyz;
    Normal  = mat3(transpose(inverse(finalModel))) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
