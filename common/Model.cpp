#include "Model.h"
#include <iostream>
#include <map>
#include <filesystem>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" 

Model::Model(const char* path) : instanceVBO(0) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string baseDir;
    try {
        baseDir = std::filesystem::path(path).parent_path().string();
        if (!baseDir.empty() && baseDir.back() != '/')
            baseDir += "/";
    } catch (...) {
        baseDir = "";
    }

    std::cout << "OBJ path: " << path << std::endl;
    std::cout << "MTL baseDir: " << baseDir << std::endl;


    bool ok = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &err,
        path,
        baseDir.c_str(),
        true
    );

    if (!err.empty())
        std::cout << "[OBJ] " << err << std::endl;

    if (!ok) {
        std::cerr << "Failed load: " << path << std::endl;
        return;
    }

    std::cout << "Materials loaded: " << materials.size() << std::endl;

    // 1. Store Material Colors from MTL
    for (const auto& mat : materials) {
        // TinyOBJ loads diffuse as length 3 array
        materialColors.push_back(glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
    }
    // Default fallback material (white) if index is -1
    materialColors.push_back(glm::vec3(1.0f)); 

    // 2. Sort Vertices by Material ID
    // Map: Material ID -> List of Vertices
    std::map<int, std::vector<ModelVertex>> sortedVertices;

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        
        // Loop over faces
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int matId = shape.mesh.material_ids[f];
            if (matId < 0) matId = materialColors.size() - 1; // Use fallback

            int fv = shape.mesh.num_face_vertices[f];
            
            // Loop over vertices in the face
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
                } else {
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                // TexCoords (unused by this model, but kept for compatibility)
                if (idx.texcoord_index >= 0) {
                    vertex.TexCoords.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertex.TexCoords.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }

                sortedVertices[matId].push_back(vertex);
            }
            index_offset += fv;
        }
    }

    // 3. Flatten into single VBO but record ranges
    vertices.clear();
    for (auto& pair : sortedVertices) {
        int matId = pair.first;
        std::vector<ModelVertex>& verts = pair.second;

        MeshRange range;
        range.startOffset = (unsigned int)vertices.size();
        range.count = (unsigned int)verts.size();
        range.materialIndex = matId;
        
        meshRanges.push_back(range);
        
        // Append to main list
        vertices.insert(vertices.end(), verts.begin(), verts.end());
    }

    setupMesh();
}

void Model::setupMesh() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, Normal));
    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, TexCoords));

    glBindVertexArray(0);
}

void Model::Draw(Shader& shader) {
    glBindVertexArray(VAO);

    // Draw each part with its specific material color
    for (const auto& range : meshRanges) {
        // Send color to shader
        if (range.materialIndex >= 0 && range.materialIndex < materialColors.size()) {
            shader.SetVec3("uMaterialColor", materialColors[range.materialIndex]);
        } else {
            shader.SetVec3("uMaterialColor", glm::vec3(1.0f));
        }

        glDrawArrays(GL_TRIANGLES, range.startOffset, range.count);
    }

    glBindVertexArray(0);
}
void Model::DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models)
{
    if (models.empty()) return;

    glBindVertexArray(VAO);

    // Create or update instance buffer
    if (instanceVBO == 0)
        glGenBuffers(1, &instanceVBO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        models.size() * sizeof(glm::mat4),
        models.data(),
        GL_DYNAMIC_DRAW
    );

    // Ensure instance attributes ALWAYS exist
    std::size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(
            3 + i,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(glm::mat4),
            (void*)(i * vec4Size)
        );
        glVertexAttribDivisor(3 + i, 1);
    }

    // Draw per material range
    for (const auto& range : meshRanges)
    {
        shader.SetVec3("uMaterialColor", materialColors[range.materialIndex]);
        glDrawArraysInstanced(
            GL_TRIANGLES,
            range.startOffset,
            range.count,
            (GLsizei)models.size()
        );
    }

    glBindVertexArray(0);
}

