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
    // Constructor loads the file immediately
    Model(const char* path);
    
    // Draw the model
    void Draw(Shader& shader);

private:
    // OpenGL buffers
    unsigned int VAO, VBO;
    
    // Data container
    std::vector<ModelVertex> vertices;
    
    // Helpers
    void setupMesh();
};