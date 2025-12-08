#version 410 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords; 

uniform sampler2D textureGrass;
uniform sampler2D textureRock;
uniform sampler2D texturePeak;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec2 peakHeightRange;

void main() {
    // 1. Calculate Slope
    vec3 norm = normalize(Normal);
    vec3 up = vec3(0.0, 1.0, 0.0);
    
    // Dot product with UP vector: 
    // 1.0 = Flat ground, 0.0 = Vertical wall
    float slope = dot(norm, up);
    
    // 2. Blend Textures
    // If slope > 0.7 (flat-ish), use mostly Grass.
    // If slope < 0.7 (steep), blend in Rock.
    // We create a blendFactor that creates a smooth transition.
    float blendAmount = clamp((slope - 0.7) * 10.0, 0.0, 1.0); 
    
    vec4 grassColor = texture(textureGrass, TexCoords);
    vec4 rockColor  = texture(textureRock, TexCoords);
    vec4 peakColor  = texture(texturePeak, TexCoords * 0.4);
    
    vec3 baseColor = mix(rockColor, grassColor, blendAmount).rgb;
    float height = FragPos.y;
    float peakLerp = clamp((height - peakHeightRange.x) / max(0.001, peakHeightRange.y - peakHeightRange.x), 0.0, 1.0);
    peakLerp = smoothstep(0.0, 1.0, peakLerp);
    float steepFactor = clamp((0.85 - slope) * 3.0, 0.0, 1.0);
    float peakBlend = peakLerp * (0.4 + 0.6 * steepFactor);
    vec3 objectColor = mix(baseColor, peakColor.rgb, peakBlend);

    // 3. Lighting 
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.1; 
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
