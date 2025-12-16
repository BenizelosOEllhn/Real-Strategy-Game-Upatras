#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 7) in uvec4 aBoneIDs;
layout (location = 8) in vec4 aBoneWeights;

layout (location = 3) in vec4 iRow0;
layout (location = 4) in vec4 iRow1;
layout (location = 5) in vec4 iRow2;
layout (location = 6) in vec4 iRow3;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform bool isInstanced;
uniform bool uUseSkinning;
uniform samplerBuffer uBoneTexture;
uniform int uBoneCount;

void main()
{
    mat4 M = isInstanced ? mat4(iRow0, iRow1, iRow2, iRow3) : model;
    vec4 localPos = vec4(aPos, 1.0);
    if (uUseSkinning && uBoneCount > 0)
    {
        float weightSum = aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w;
        if (weightSum > 0.0f)
        {
            mat4 skinMat = mat4(0.0);
            if (aBoneIDs.x < uint(uBoneCount))
            {
                int base = int(aBoneIDs.x) * 4;
                mat4 bone = mat4(
                    texelFetch(uBoneTexture, base + 0),
                    texelFetch(uBoneTexture, base + 1),
                    texelFetch(uBoneTexture, base + 2),
                    texelFetch(uBoneTexture, base + 3)
                );
                skinMat += aBoneWeights.x * bone;
            }
            if (aBoneIDs.y < uint(uBoneCount))
            {
                int base = int(aBoneIDs.y) * 4;
                mat4 bone = mat4(
                    texelFetch(uBoneTexture, base + 0),
                    texelFetch(uBoneTexture, base + 1),
                    texelFetch(uBoneTexture, base + 2),
                    texelFetch(uBoneTexture, base + 3)
                );
                skinMat += aBoneWeights.y * bone;
            }
            if (aBoneIDs.z < uint(uBoneCount))
            {
                int base = int(aBoneIDs.z) * 4;
                mat4 bone = mat4(
                    texelFetch(uBoneTexture, base + 0),
                    texelFetch(uBoneTexture, base + 1),
                    texelFetch(uBoneTexture, base + 2),
                    texelFetch(uBoneTexture, base + 3)
                );
                skinMat += aBoneWeights.z * bone;
            }
            if (aBoneIDs.w < uint(uBoneCount))
            {
                int base = int(aBoneIDs.w) * 4;
                mat4 bone = mat4(
                    texelFetch(uBoneTexture, base + 0),
                    texelFetch(uBoneTexture, base + 1),
                    texelFetch(uBoneTexture, base + 2),
                    texelFetch(uBoneTexture, base + 3)
                );
                skinMat += aBoneWeights.w * bone;
            }
            localPos = skinMat * localPos;
        }
    }
    gl_Position = lightSpaceMatrix * M * localPos;
}
