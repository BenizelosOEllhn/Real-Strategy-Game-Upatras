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

    // Vertex access for deformation
    std::vector<Vertex>& GetVertices() { return vertices; }
    const std::vector<Vertex>& GetVertices() const { return vertices; }
    std::vector<unsigned int>& GetIndices() { return indices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }
    
    // Utility methods for deformation
    int GetWidth() const { return width; }
    int GetDepth() const { return depth; }
    
    // Sync vertex data back to GPU after modifications
    void SyncVerticesToGPU();
    
    // Recalculate surface normals in a given radius
    void RecalculateNormalsInRadius(const glm::vec3& center, float radius);

private:
    int width, depth;
    unsigned int VAO, VBO, EBO;
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void setupMesh();
};