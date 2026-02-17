#version 330 core

in vec2 TexCoord;
flat in float vAlpha;

out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    vec4 texColor = texture(uTexture, TexCoord);
    
    // Discard transparent pixels
    if (texColor.a < 0.1)
        discard;
    
    // Apply alpha fade
    texColor.a *= vAlpha;
    
    FragColor = texColor;
}
