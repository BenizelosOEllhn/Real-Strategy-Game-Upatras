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
    void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& transforms);

private:
    void setupMesh();

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint instanceVBO = 0;  

    std::vector<ModelVertex> vertices;
};
