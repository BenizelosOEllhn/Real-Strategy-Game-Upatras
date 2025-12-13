#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
// INSTANCING: Receive model matrix as attribute (slots 3,4,5,6)
layout (location = 3) in mat4 aInstanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform vec4 uClipPlane;
out float vClipDist;

void main() {
    // Use instance matrix instead of uniform 'model'
    vec4 worldPos = aInstanceMatrix * vec4(aPos, 1.0);
    
    vec4 worldPos = model * vec4(aPos, 1.0);
    vClipDist = dot(worldPos, uClipPlane);

    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    TexCoords = aTexCoords;
    
    // Calculate shadow coordinate
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    
    gl_Position = projection * view * worldPos;
}