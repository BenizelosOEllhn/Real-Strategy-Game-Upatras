#include "Model.h"
#include <iostream>

// This defines the library implementation. 
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" 

Model::Model(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path);

    if (!err.empty()) {
        std::cerr << "OBJ Error: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load OBJ: " << path << std::endl;
        return;
    }

// Loop over shapes
    for (const auto& shape : shapes) {
        // Loop over faces (polygons)
        size_t index_offset = 0;
        
        // Loop through every face in the mesh
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // Get number of vertices for this face (usually 3 if triangulated)
            int fv = shape.mesh.num_face_vertices[f];

            // Loop over vertices in the face
            for (size_t v = 0; v < fv; v++) {
                // Access to vertex via index
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                
                ModelVertex vertex;

                // 1. Position
                // tinyobj stores flat arrays (x,y,z,x,y,z...). We jump by 3s.
                vertex.Position.x = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.Position.y = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.Position.z = attrib.vertices[3 * idx.vertex_index + 2];

                // 2. Normal
                // Check if normals exist in the file (normal_index >= 0)
                if (idx.normal_index >= 0) {
                    vertex.Normal.x = attrib.normals[3 * idx.normal_index + 0];
                    vertex.Normal.y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.Normal.z = attrib.normals[3 * idx.normal_index + 2];
                } else {
                    // Fallback if OBJ has no normals (rare if exported correctly)
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); 
                }

                // 3. TexCoords
                // Check if UVs exist
                if (idx.texcoord_index >= 0) {
                    vertex.TexCoords.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    // IMPORTANT: OBJ (0,0) is bottom-left. 
                    // Sometimes you need '1.0 - y' here if textures look upside down.
                    // For now, we leave it raw.
                    vertex.TexCoords.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f);
                }

                vertices.push_back(vertex);
            }
            index_offset += fv;
        }
    }
    // After parsing, upload to GPU
    setupMesh();
}

void Model::setupMesh() {
    // SAFETY CHECK: If no vertices were loaded, don't crash.
    if (vertices.empty()) {
        std::cerr << "CRITICAL WARNING: Model has 0 vertices! Skipping GPU upload." << std::endl;
        return;
    }

    // Create buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    // Load data into VBO
    // &vertices[0] is only safe because we checked !empty() above
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW);

    // 1. Position (Location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)0);

    // 2. Normal (Location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, Normal));

    // 3. TexCoords (Location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, TexCoords));

    glBindVertexArray(0);
}
void Model::Draw(Shader& shader) {
    glBindVertexArray(VAO);
    // Draw triangles using the count of vertices we loaded
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}