#version 410 core

// Per-vertex
layout(location = 0) in vec3 aPos;

// Per-instance matrix (4 vec4 attributes!)
layout(location = 3) in vec4 iRow0;
layout(location = 4) in vec4 iRow1;
layout(location = 5) in vec4 iRow2;
layout(location = 6) in vec4 iRow3;

uniform mat4 lightSpaceMatrix;

void main()
{
    mat4 instanceModel = mat4(iRow0, iRow1, iRow2, iRow3);
    gl_Position = lightSpaceMatrix * instanceModel * vec4(aPos, 1.0);
}
