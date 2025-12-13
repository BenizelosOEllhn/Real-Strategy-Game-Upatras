#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

out vec2 UV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    UV = inUV;
    gl_Position = uProj * uView * uModel * vec4(inPos, 1.0);
}
