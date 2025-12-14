#version 410 core

layout (location = 0) in vec3 aPos;

// Instance attributes
layout (location = 3) in vec4 iRow0;
layout (location = 4) in vec4 iRow1;
layout (location = 5) in vec4 iRow2;
layout (location = 6) in vec4 iRow3;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform bool isInstanced;

void main()
{
    mat4 M;

    if (isInstanced)
        M = mat4(iRow0, iRow1, iRow2, iRow3);
    else
        M = model;

    gl_Position = lightSpaceMatrix * M * vec4(aPos, 1.0);
}
