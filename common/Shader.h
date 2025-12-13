#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;

    // Constructor: loads & builds shader
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    // Activate shader program
    void Use() const;

    // --- Uniform setters ---
    void SetBool (const std::string& name, bool value) const;
    void SetInt  (const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;

    void SetVec2 (const std::string& name, const glm::vec2& value) const;
    void SetVec3 (const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string &name, const glm::vec4 &value) const;

    void SetMat4 (const std::string& name, const glm::mat4& mat) const;

private:
    // Utility: read shader file to string
    std::string LoadFileAsString(const std::string& path);

    // Utility: check compile/link errors
    void CheckCompileErrors(unsigned int shader, const std::string& type);
};
