#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform vec4 uTint;
uniform int uHasTexture;  // 0 = solid color, 1 = sampled texture

void main()
{
    if (uHasTexture == 1)
    {
        vec4 color = texture(uTex, vUV) * uTint;
        if (color.a < 0.01)
            discard;
        FragColor = color;
    }
    else
    {
        // solid colored quad (bar background, hover frame, etc.)
        FragColor = uTint;
    }
}
