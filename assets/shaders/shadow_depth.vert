#version 410 core

// Vertex attributes coming from your Model VAO
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;  
layout(location = 2) in vec2 aTexCoord; 

// Instance matrix uses 4 attribute locations: 3,4,5,6
layout(location = 3) in mat4 instanceMatrix;

// Uniforms (match your C++ names!)
uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    // model: per-mesh transform (usually identity for trees/rocks)
    // instanceMatrix: per-instance transform (tree/rock placement)
    vec4 worldPos = model * instanceMatrix * vec4(aPos, 1.0);

    gl_Position = lightSpaceMatrix * worldPos;
}
