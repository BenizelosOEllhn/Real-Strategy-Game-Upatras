#include "Model.h"
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" 

Model::Model(const char* path) : instanceVBO(0) { // Initialize instanceVBO
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path)) {
        std::cerr << "Failed to load OBJ: " << path << " Error: " << err << std::endl;
        return;
    }

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                ModelVertex vertex;

                vertex.Position.x = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.Position.y = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.Position.z = attrib.vertices[3 * idx.vertex_index + 2];

                if (idx.normal_index >= 0) {
                    vertex.Normal.x = attrib.normals[3 * idx.normal_index + 0];
                    vertex.Normal.y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.Normal.z = attrib.normals[3 * idx.normal_index + 2];
                } else { vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); }

                if (idx.texcoord_index >= 0) {
                    vertex.TexCoords.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertex.TexCoords.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else { vertex.TexCoords = glm::vec2(0.0f); }

                vertices.push_back(vertex);
            }
            index_offset += fv;
        }
    }
    setupMesh();
}

void Model::setupMesh() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // Generate Instance Buffer once
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW);

    // Vertex Attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, TexCoords));

    glBindVertexArray(0);
}

void Model::Draw(Shader& shader) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

// === NEW FUNCTION ===
void Model::DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models) {
    if (models.empty()) return;

    glBindVertexArray(VAO);

    // 1. Bind the Instance Buffer and upload the matrices
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), &models[0], GL_STATIC_DRAW);

    // 2. Set pointers for the 4 vec4s that make up the mat4
    //    We use locations 3, 4, 5, 6
    std::size_t vec4Size = sizeof(glm::vec4);

    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
        // Tell OpenGL this is an instance variable (1 means update every 1 instance)
        glVertexAttribDivisor(3 + i, 1);
    }

    // 3. Draw
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), models.size());

    // Cleanup attributes
    for (int i = 0; i < 4; i++) {
        glVertexAttribDivisor(3 + i, 0); // Reset divisor
        glDisableVertexAttribArray(3 + i);
    }
    
    glBindVertexArray(0);
}