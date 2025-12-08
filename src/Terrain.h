#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal; 
    glm::vec2 TexCoords;
};

class Terrain {
public:
    Terrain(int width, int depth);
    ~Terrain();
    static float getHeight(float x, float z);
    static glm::vec3 getNormal(float x, float z);
    void Draw(unsigned int shaderProgram);

private:
    int width, depth;
    unsigned int VAO, VBO, EBO;
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void setupMesh();
};