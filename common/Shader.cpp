#include "Shader.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

// ------------------------------------------------------------
// Load file into string
// ------------------------------------------------------------
std::string Shader::LoadFileAsString(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

// ------------------------------------------------------------
// Constructor: compile + link shader
// ------------------------------------------------------------
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexCode   = LoadFileAsString(vertexPath);
    std::string fragmentCode = LoadFileAsString(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;

    // ------------------ Vertex Shader ------------------
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    // ------------------ Fragment Shader ------------------
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    // ------------------ Shader Program ------------------
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    
    // Check for linking errors
    CheckCompileErrors(ID, "PROGRAM");

    // Delete shaders after linking
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// ------------------------------------------------------------
// Activation
// ------------------------------------------------------------
void Shader::Use() const
{
    glUseProgram(ID);
}

// ------------------------------------------------------------
// Uniform Helpers
// ------------------------------------------------------------
void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

// ------------------------------------------------------------
// Error checking
// ------------------------------------------------------------
void Shader::CheckCompileErrors(unsigned int shader, const std::string& type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "\nERROR::SHADER_COMPILATION_ERROR of type: "
                      << type << "\n" << infoLog
                      << "\n --------------------------------------"
                      << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);

        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "\nERROR::PROGRAM_LINKING_ERROR\n"
                      << infoLog
                      << "\n --------------------------------------"
                      << std::endl;
        }
    }
}
