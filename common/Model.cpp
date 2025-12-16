#include "Model.h"
#include <iostream>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cmath>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/common.hpp>
#include <assimp/anim.h>

Model::Model(const char* path) : instanceVBO(0)
{
    if (path)
        sourcePath_ = path;
    std::string extension;
    try {
        baseDirectory_ = std::filesystem::path(path).parent_path().string();
    } catch (...) {
        baseDirectory_.clear();
    }
    try {
        extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    } catch (...) {
        extension = "";
    }

    bool loaded = false;
    if (extension == ".obj") {
        loaded = loadWithTinyObj(path);
    } else {
        loaded = loadWithAssimp(path);
        if (!loaded && extension.empty()) {
            loaded = loadWithTinyObj(path);
        }
    }

    if (!loaded) {
        std::cerr << "Failed load: " << path << std::endl;
        return;
    }

    setupMesh();
}

Model::~Model()
{
    if (VBO) glDeleteBuffers(1, &VBO);
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}

bool Model::loadWithTinyObj(const char* path)
{
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
        baseDir.clear();
    }

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
        return false;
    }

    materialColors.clear();
    for (const auto& mat : materials) {
        materialColors.push_back(glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
    }
    materialColors.push_back(glm::vec3(1.0f));
    materialTextureIDs_.assign(materialColors.size(), 0);
    hasAnyTextures_ = false;
    boneMapping_.clear();
    bones_.clear();
    animations_.clear();
    rootNode_ = NodeData();
    globalInverseTransform_ = glm::mat4(1.0f);

    std::map<int, std::vector<ModelVertex>> sortedVertices;

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int matId = shape.mesh.material_ids[f];
            if (matId < 0) matId = static_cast<int>(materialColors.size()) - 1;

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
                } else {
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

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

    vertices.clear();
    meshRanges.clear();
    for (auto& pair : sortedVertices) {
        int matId = pair.first;
        std::vector<ModelVertex>& verts = pair.second;

        MeshRange range;
        range.startOffset = static_cast<unsigned int>(vertices.size());
        range.count = static_cast<unsigned int>(verts.size());
        range.materialIndex = matId;

        meshRanges.push_back(range);
        vertices.insert(vertices.end(), verts.begin(), verts.end());
    }

    return true;
}

void Model::processAssimpMesh(const aiMesh* mesh)
{
    if (!mesh) return;

    MeshRange range;
    range.startOffset = static_cast<unsigned int>(vertices.size());
    range.materialIndex = mesh->mMaterialIndex;

    std::vector<ModelVertex> tempVertices(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        ModelVertex& vertex = tempVertices[i];
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals())
        {
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (mesh->HasTextureCoords(0))
        {
            vertex.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f);
        }
    }

    extractBoneWeights(mesh, tempVertices);

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int i = 0; i < face.mNumIndices; ++i)
        {
            unsigned int index = face.mIndices[i];
            if (index < tempVertices.size())
                vertices.push_back(tempVertices[index]);
        }
    }

    range.count = static_cast<unsigned int>(vertices.size()) - range.startOffset;
    meshRanges.push_back(range);
}

void Model::processAssimpNode(const aiNode* node, const aiScene* scene)
{
    if (!node || !scene) return;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processAssimpMesh(mesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processAssimpNode(node->mChildren[i], scene);
    }
}

bool Model::loadWithAssimp(const char* path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode) {
        std::cerr << "Assimp failed: " << importer.GetErrorString() << std::endl;
        return false;
    }

    materialColors.clear();
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* mat = scene->mMaterials[i];
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            materialColors.emplace_back(color.r, color.g, color.b);
        } else {
            materialColors.emplace_back(1.0f, 1.0f, 1.0f);
        }
    }
    materialColors.push_back(glm::vec3(1.0f));

    vertices.clear();
    meshRanges.clear();
    boneMapping_.clear();
    bones_.clear();
    animations_.clear();
    rootNode_ = NodeData();

    processAssimpNode(scene->mRootNode, scene);
    normalizeBoneWeights();
    loadMaterialTextures(scene);
    readHierarchyData(rootNode_, scene->mRootNode);
    globalInverseTransform_ = glm::inverse(convertMatrix(scene->mRootNode->mTransformation));
    loadAnimations(scene);
    std::cout << "[Model] " << sourcePath_ << " detected " << bones_.size() << " bones.\n";

    return !vertices.empty();
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
    glEnableVertexAttribArray(7);
    glVertexAttribIPointer(7, 4, GL_INT, sizeof(ModelVertex), (void*)offsetof(ModelVertex, BoneIDs));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, BoneWeights));

    glBindVertexArray(0);
}

void Model::Draw(Shader& shader) {
    glBindVertexArray(VAO);

    // Draw each part with its specific material color
    for (const auto& range : meshRanges) {
        bool boundTexture = false;
        if (hasAnyTextures_ &&
            range.materialIndex >= 0 &&
            range.materialIndex < static_cast<int>(materialTextureIDs_.size()))
        {
            unsigned int tex = materialTextureIDs_[range.materialIndex];
            if (tex != 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);
                shader.SetBool("useTexture", true);
                boundTexture = true;
            }
        }
        if (!boundTexture)
            shader.SetBool("useTexture", false);

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
        bool boundTexture = false;
        if (hasAnyTextures_ &&
            range.materialIndex >= 0 &&
            range.materialIndex < static_cast<int>(materialTextureIDs_.size()))
        {
            unsigned int tex = materialTextureIDs_[range.materialIndex];
            if (tex != 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);
                shader.SetBool("useTexture", true);
                boundTexture = true;
            }
        }
        if (!boundTexture)
            shader.SetBool("useTexture", false);

        glm::vec3 color = glm::vec3(1.0f);
        if (range.materialIndex >= 0 && range.materialIndex < static_cast<int>(materialColors.size()))
            color = materialColors[range.materialIndex];
        shader.SetVec3("uMaterialColor", color);
        glDrawArraysInstanced(
            GL_TRIANGLES,
            range.startOffset,
            range.count,
            (GLsizei)models.size()
        );
    }

    glBindVertexArray(0);
}

void Model::loadMaterialTextures(const aiScene* scene)
{
    materialTextureIDs_.clear();
    if (!scene)
    {
        hasAnyTextures_ = false;
        return;
    }

    materialTextureIDs_.assign(scene->mNumMaterials + 1, 0);
    hasAnyTextures_ = false;

    std::unordered_map<std::string, unsigned int> cache;

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* material = scene->mMaterials[i];
        if (!material) continue;

        aiString texPath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            std::string relPath = texPath.C_Str();
            unsigned int texID = 0;
            auto it = cache.find(relPath);
            if (it != cache.end())
            {
                texID = it->second;
            }
            else
            {
                texID = loadTextureForMaterial(relPath);
                if (texID != 0)
                    cache[relPath] = texID;
            }

            materialTextureIDs_[i] = texID;
            if (texID != 0)
                hasAnyTextures_ = true;
        }
    }
}

unsigned int Model::loadTextureForMaterial(const std::string& relPath)
{
    if (relPath.empty())
        return 0;

    auto existsSafe = [](const std::filesystem::path& p) -> bool
    {
        try { return std::filesystem::exists(p); }
        catch (...) { return false; }
    };

    std::filesystem::path texturePath(relPath);
    bool windowsAbsolute = relPath.size() > 1 && relPath[1] == ':';
    if (windowsAbsolute)
    {
        size_t pos = relPath.find_last_of("\\/");
        std::string fileName = (pos != std::string::npos)
            ? relPath.substr(pos + 1)
            : relPath;
        texturePath = std::filesystem::path(fileName);
    }

    if (texturePath.is_relative() && !baseDirectory_.empty())
    {
        texturePath = std::filesystem::path(baseDirectory_) / texturePath;
    }

    if (!existsSafe(texturePath))
    {
        std::vector<std::filesystem::path> searchDirs;
        if (!baseDirectory_.empty())
        {
            std::filesystem::path base(baseDirectory_);
            searchDirs.push_back(base);
            searchDirs.push_back(base / "textures");

            std::filesystem::path parent = base.parent_path();
            int depth = 0;
            while (!parent.empty() && depth < 3)
            {
                searchDirs.push_back(parent);
                searchDirs.push_back(parent / "textures");
                searchDirs.push_back(parent / "Textures");
                parent = parent.parent_path();
                ++depth;
            }
        }

        std::filesystem::path filename = texturePath.filename();
        for (const auto& dir : searchDirs)
        {
            if (dir.empty()) continue;
            std::filesystem::path candidate = dir / filename;
            if (existsSafe(candidate))
            {
                texturePath = candidate;
                break;
            }
        }
    }

    if (!existsSafe(texturePath))
    {
        std::cerr << "[Model] Missing texture: " << relPath << " (base: " << baseDirectory_ << ")\n";
        return 0;
    }

    std::string fullPath = texturePath.string();
    std::unique_ptr<Texture> tex = std::make_unique<Texture>(fullPath.c_str());
    unsigned int texID = tex->ID;
    ownedTextures_.push_back(std::move(tex));
    return texID;
}

void Model::SetOverrideTexture(const std::string& path)
{
    if (path.empty())
        return;

    std::unique_ptr<Texture> tex = std::make_unique<Texture>(path.c_str());
    unsigned int texID = tex->ID;
    ownedTextures_.push_back(std::move(tex));

    size_t materialCount = std::max<size_t>(materialColors.size(), static_cast<size_t>(1));
    materialTextureIDs_.assign(materialCount, texID);
    hasAnyTextures_ = true;
}

glm::mat4 Model::convertMatrix(const aiMatrix4x4& mat) const
{
    glm::mat4 result;
    result[0][0] = mat.a1; result[1][0] = mat.a2; result[2][0] = mat.a3; result[3][0] = mat.a4;
    result[0][1] = mat.b1; result[1][1] = mat.b2; result[2][1] = mat.b3; result[3][1] = mat.b4;
    result[0][2] = mat.c1; result[1][2] = mat.c2; result[2][2] = mat.c3; result[3][2] = mat.c4;
    result[0][3] = mat.d1; result[1][3] = mat.d2; result[2][3] = mat.d3; result[3][3] = mat.d4;
    return result;
}

glm::vec3 Model::convertVector(const aiVector3D& vec) const
{
    return glm::vec3(vec.x, vec.y, vec.z);
}

glm::quat Model::convertQuat(const aiQuaternion& quat) const
{
    return glm::quat(quat.w, quat.x, quat.y, quat.z);
}

void Model::readHierarchyData(NodeData& dest, const aiNode* src)
{
    dest.name = src->mName.C_Str();
    dest.transform = convertMatrix(src->mTransformation);
    dest.children.clear();
    dest.children.reserve(src->mNumChildren);
    for (unsigned int i = 0; i < src->mNumChildren; ++i)
    {
        NodeData child;
        readHierarchyData(child, src->mChildren[i]);
        dest.children.push_back(std::move(child));
    }
}

void Model::extractBoneWeights(const aiMesh* mesh, std::vector<ModelVertex>& tempVertices)
{
    if (!mesh) return;

    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
    {
        aiBone* bone = mesh->mBones[i];
        std::string name = bone->mName.C_Str();
        int boneIndex = 0;

        auto it = boneMapping_.find(name);
        if (it == boneMapping_.end())
        {
            boneIndex = static_cast<int>(bones_.size());
            boneMapping_[name] = boneIndex;
            BoneInfo info;
            info.offsetMatrix = convertMatrix(bone->mOffsetMatrix);
            bones_.push_back(info);
        }
        else
        {
            boneIndex = it->second;
        }

        for (unsigned int w = 0; w < bone->mNumWeights; ++w)
        {
            const aiVertexWeight& weight = bone->mWeights[w];
            size_t vertexID = weight.mVertexId;
            if (vertexID < tempVertices.size())
            {
                tempVertices[vertexID].AddBoneData(boneIndex, weight.mWeight);
            }
        }
    }
}

void Model::normalizeBoneWeights()
{
    for (auto& vertex : vertices)
    {
        float sum = vertex.BoneWeights.x + vertex.BoneWeights.y +
                    vertex.BoneWeights.z + vertex.BoneWeights.w;
        if (sum > 0.0f)
        {
            vertex.BoneWeights /= sum;
        }
    }
}

void Model::loadAnimations(const aiScene* scene)
{
    animations_.clear();
    if (!scene || scene->mNumAnimations == 0)
    {
        std::cout << "[Model] " << sourcePath_ << " contains 0 animations.\n";
        return;
    }

    animations_.reserve(scene->mNumAnimations);
    std::cout << "[Model] " << sourcePath_ << " loading " << scene->mNumAnimations << " animation(s):\n";
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        const aiAnimation* anim = scene->mAnimations[i];
        AnimationClip clip;
        clip.name = anim->mName.length > 0 ? anim->mName.C_Str() : ("Animation" + std::to_string(i));
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0;

        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            const aiNodeAnim* channel = anim->mChannels[c];
            NodeAnimationChannel animChannel;

            for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k)
            {
                animChannel.positions.emplace_back(channel->mPositionKeys[k].mTime,
                                                   convertVector(channel->mPositionKeys[k].mValue));
            }
            for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k)
            {
                animChannel.rotations.emplace_back(channel->mRotationKeys[k].mTime,
                                                   convertQuat(channel->mRotationKeys[k].mValue));
            }
            for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k)
            {
                animChannel.scales.emplace_back(channel->mScalingKeys[k].mTime,
                                                convertVector(channel->mScalingKeys[k].mValue));
            }

            clip.channels[channel->mNodeName.C_Str()] = std::move(animChannel);
        }

        animations_.push_back(std::move(clip));
        std::cout << "  - " << animations_.back().name << " (" << animations_.back().duration << " ticks)\n";
    }
}

glm::vec3 Model::interpolateVec3(const std::vector<std::pair<double, glm::vec3>>& keys,
                                 double timeTicks) const
{
    if (keys.empty())
        return glm::vec3(0.0f);
    if (keys.size() == 1)
        return keys[0].second;

    size_t index = 0;
    for (; index < keys.size() - 1; ++index)
    {
        if (timeTicks < keys[index + 1].first)
            break;
    }
    size_t nextIndex = std::min(index + 1, keys.size() - 1);
    double delta = keys[nextIndex].first - keys[index].first;
    double factor = 0.0;
    if (delta > std::numeric_limits<double>::epsilon())
        factor = (timeTicks - keys[index].first) / delta;
    return glm::mix(keys[index].second, keys[nextIndex].second, static_cast<float>(factor));
}

glm::quat Model::interpolateQuat(const std::vector<std::pair<double, glm::quat>>& keys,
                                 double timeTicks) const
{
    if (keys.empty())
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (keys.size() == 1)
        return keys[0].second;

    size_t index = 0;
    for (; index < keys.size() - 1; ++index)
    {
        if (timeTicks < keys[index + 1].first)
            break;
    }
    size_t nextIndex = std::min(index + 1, keys.size() - 1);
    double delta = keys[nextIndex].first - keys[index].first;
    double factor = 0.0;
    if (delta > std::numeric_limits<double>::epsilon())
        factor = (timeTicks - keys[index].first) / delta;
    return glm::slerp(keys[index].second, keys[nextIndex].second, static_cast<float>(factor));
}

glm::mat4 Model::interpolateChannelTransform(const NodeAnimationChannel& channel,
                                             double timeTicks) const
{
    glm::vec3 translation = interpolateVec3(channel.positions, timeTicks);
    glm::quat rotation = interpolateQuat(channel.rotations, timeTicks);
    glm::vec3 scale = interpolateVec3(channel.scales, timeTicks);

    glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 r = glm::toMat4(rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}

void Model::calculateBoneTransforms(const AnimationClip& clip,
                                    double timeTicks,
                                    const NodeData& node,
                                    const glm::mat4& parent,
                                    std::vector<glm::mat4>& outMatrices) const
{
    glm::mat4 nodeTransform = node.transform;
    auto channelIt = clip.channels.find(node.name);
    if (channelIt != clip.channels.end())
    {
        nodeTransform = interpolateChannelTransform(channelIt->second, timeTicks);
    }

    glm::mat4 globalTransform = parent * nodeTransform;
    auto boneIt = boneMapping_.find(node.name);
    if (boneIt != boneMapping_.end() && boneIt->second < static_cast<int>(outMatrices.size()))
    {
        int index = boneIt->second;
        outMatrices[index] = globalInverseTransform_ * globalTransform * bones_[index].offsetMatrix;
    }

    for (const auto& child : node.children)
    {
        calculateBoneTransforms(clip, timeTicks, child, globalTransform, outMatrices);
    }
}

void Model::EvaluateAnimation(size_t animationIndex, double timeInSeconds, std::vector<glm::mat4>& outMatrices) const
{
    if (animations_.empty() || bones_.empty())
    {
        outMatrices.clear();
        return;
    }

    const AnimationClip& clip = animations_[animationIndex % animations_.size()];
    double ticks = timeInSeconds * clip.ticksPerSecond;
    double cycle = fmod(ticks, clip.duration);

    outMatrices.assign(bones_.size(), glm::mat4(1.0f));
    calculateBoneTransforms(clip, cycle, rootNode_, glm::mat4(1.0f), outMatrices);
}

int Model::FindAnimationIndex(const std::string& keyword) const
{
    if (animations_.empty())
        return -1;

    if (keyword.empty())
        return 0;

    std::string loweredKey = toLower(keyword);
    for (size_t i = 0; i < animations_.size(); ++i)
    {
        if (toLower(animations_[i].name).find(loweredKey) != std::string::npos)
            return static_cast<int>(i);
    }
    return animations_.empty() ? -1 : 0;
}

std::string Model::toLower(const std::string& value) const
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}
void ModelVertex::AddBoneData(int boneID, float weight)
{
    for (int i = 0; i < 4; ++i)
    {
        if (BoneWeights[i] == 0.0f)
        {
            BoneIDs[i] = boneID;
            BoneWeights[i] = weight;
            return;
        }
    }

    int minIndex = 0;
    for (int i = 1; i < 4; ++i)
    {
        if (BoneWeights[i] < BoneWeights[minIndex])
            minIndex = i;
    }

    if (weight > BoneWeights[minIndex])
    {
        BoneIDs[minIndex] = boneID;
        BoneWeights[minIndex] = weight;
    }
}
