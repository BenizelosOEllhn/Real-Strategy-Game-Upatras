#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID; // The GPU Program ID

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    void Use();
    // Helper to set uniforms easily
    void SetMat4(const std::string& name, const glm::mat4& mat); 
};