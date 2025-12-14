#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "../../common/Shader.h"

struct ModelVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct MeshRange {
    unsigned int startOffset; // Where this part starts in the VBO
    unsigned int count;       // How many vertices to draw
    int materialIndex;        // Which color to use
};

class Model {
public:
    Model(const char* path);
    void Draw(Shader& shader);
    void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models);

private:
    unsigned int VAO, VBO, instanceVBO;
    std::vector<ModelVertex> vertices;
    
    // NEW: Store draw ranges and material colors
    std::vector<MeshRange> meshRanges;
    std::vector<glm::vec3> materialColors; // Stores Kd colors from MTL

    void setupMesh();
};