#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform vec4 uTint;
uniform int uHasTexture;  // 0 = solid color, 1 = sampled texture
uniform int uRadialFill;  // 0 = normal, 1 = radial fill mode
uniform float uRadialProgress; // 0.0 to 1.0

#define PI 3.14159265359

void main()
{
    if (uHasTexture == 1)
    {
        vec4 color = texture(uTex, vUV) * uTint;
        
        // Radial fill clipping (for capture progress ring)
        if (uRadialFill == 1)
        {
            float progress = clamp(uRadialProgress, 0.0, 1.0);
            if (progress <= 0.0)
                discard;

            if (progress < 1.0)
            {
                // Calculate angle from center (0.5, 0.5)
                vec2 centered = vUV - vec2(0.5);
                // GLSL atan(y, x): 0 at +X axis, CCW positive
                float theta = atan(centered.y, centered.x);

                // Convert to: 0 at top (12 o'clock), increasing clockwise
                float fromTopClockwise = mod((0.5 * PI - theta) + 2.0 * PI, 2.0 * PI);
                float normalizedAngle = fromTopClockwise / (2.0 * PI);

                // Clip if angle exceeds progress
                if (normalizedAngle > progress)
                    discard;
            }
        }
        
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
