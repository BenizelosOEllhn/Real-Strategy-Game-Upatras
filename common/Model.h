#pragma once
#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.h" 

struct ModelVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Model {
public:
    Model(const char* path);
    
    void Draw(Shader& shader);
    // NEW: Function to draw many copies at once
    void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models);

private:
    unsigned int VAO, VBO;
    // NEW: We need a buffer for the matrices
    unsigned int instanceVBO; 
    
    std::vector<ModelVertex> vertices;
    void setupMesh();
};