#include "Model.h"
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path);

    if (!err.empty())
        std::cerr << "OBJ Error: " << err << std::endl;

    if (!ret) {
        std::cerr << "Failed to load OBJ: " << path << std::endl;
        return;
    }

    // Parse OBJ
    for (const auto& shape : shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                ModelVertex vertex;

                vertex.Position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                if (idx.normal_index >= 0) {
                    vertex.Normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                } else {
                    vertex.Normal = glm::vec3(0, 1, 0);
                }

                if (idx.texcoord_index >= 0) {
                    vertex.TexCoords = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                } else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }

                vertices.push_back(vertex);
            }

            index_offset += fv;
        }
    }

    setupMesh();
}

void Model::setupMesh() {
    if (vertices.empty()) {
        std::cerr << "Model has no vertices!\n";
        return;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);  // <--- NEW

    glBindVertexArray(VAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(ModelVertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Positions (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ModelVertex),
                          (void*)offsetof(ModelVertex, Position));

    // Normals (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ModelVertex),
                          (void*)offsetof(ModelVertex, Normal));

    // TexCoords (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ModelVertex),
                          (void*)offsetof(ModelVertex, TexCoords));

    glBindVertexArray(0);
}

void Model::Draw(Shader& shader) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

// -------------------------------------------------------------
//          INSTANCED DRAW CALL
// -------------------------------------------------------------
void Model::DrawInstanced(Shader& shader,
                          const std::vector<glm::mat4>& transforms)
{
    if (transforms.empty())
        return;

    glBindVertexArray(VAO);

    // Upload instance transform matrices
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 transforms.size() * sizeof(glm::mat4),
                 transforms.data(),
                 GL_STATIC_DRAW);

    // A mat4 occupies 4 attribute locations
    std::size_t vec4Size = sizeof(glm::vec4);

    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i); // locations 3,4,5,6
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4),
                              (void*)(i * vec4Size));

        glVertexAttribDivisor(3 + i, 1); 
    }

    // Draw N instances
    glDrawArraysInstanced(GL_TRIANGLES,
                          0,
                          vertices.size(),
                          transforms.size());

    glBindVertexArray(0);
}
