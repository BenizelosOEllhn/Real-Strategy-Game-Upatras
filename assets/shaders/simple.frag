#version 410 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;

void main() {
    // 1. Texture Color
    vec3 objectColor = texture(texture_diffuse1, TexCoords).rgb;

    // 2. Ambient Light (Base brightness)
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
  
    // 3. Diffuse Light (Sunlight shadows)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 4. Specular Light (Shininess - kept low for wood/leaves)
    float specularStrength = 0.1; 
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * lightColor;  
    
    // Combine results
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}