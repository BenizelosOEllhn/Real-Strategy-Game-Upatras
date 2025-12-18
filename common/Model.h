#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>
#include "../../common/Shader.h"
#include "../../common/Texture.h"

#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>
#include <assimp/quaternion.h>

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiAnimation;
struct aiNodeAnim;

struct ModelVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::ivec4 BoneIDs = glm::ivec4(0);
    glm::vec4 BoneWeights = glm::vec4(0.0f);

    void AddBoneData(int boneID, float weight);
};

struct MeshRange {
    unsigned int startOffset; // Where this part starts in the VBO
    unsigned int count;       // How many vertices to draw
    int materialIndex;        // Which color to use
};

class Model {
public:
    Model(const char* path);
    ~Model();
    void Draw(Shader& shader);
    void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models);
    bool HasTextures() const { return hasAnyTextures_; }
    void SetOverrideTexture(const std::string& path);
    bool HasAnimations() const { return !animations_.empty(); }
    size_t GetBoneCount() const { return bones_.size(); }
    size_t GetAnimationCount() const { return animations_.size(); }
    void EvaluateAnimation(size_t animationIndex, double timeInSeconds, std::vector<glm::mat4>& outMatrices) const;
    int FindAnimationIndex(const std::string& keyword) const;

private:
    unsigned int VAO, VBO, instanceVBO;
    std::vector<ModelVertex> vertices;
    
    // NEW: Store draw ranges and material colors
    std::vector<MeshRange> meshRanges;
    std::vector<glm::vec3> materialColors; // Stores Kd colors from MTL
    std::vector<unsigned int> materialTextureIDs_;
    std::vector<std::unique_ptr<Texture>> ownedTextures_;
    std::vector<GLuint> ownedGLTextures_;
    bool hasAnyTextures_ = false;
    std::string baseDirectory_;
    std::string sourcePath_;

    struct BoneInfo
    {
        glm::mat4 offsetMatrix{1.0f};
    };

    struct NodeAnimationChannel
    {
        std::vector<std::pair<double, glm::vec3>> positions;
        std::vector<std::pair<double, glm::quat>> rotations;
        std::vector<std::pair<double, glm::vec3>> scales;
    };

    struct AnimationClip
    {
        std::string name;
        double duration = 0.0;
        double ticksPerSecond = 25.0;
        std::unordered_map<std::string, NodeAnimationChannel> channels;
    };

    struct NodeData
    {
        std::string name;
        glm::mat4 transform{1.0f};
        std::vector<NodeData> children;
    };

    std::unordered_map<std::string, int> boneMapping_;
    std::vector<BoneInfo> bones_;
    std::vector<AnimationClip> animations_;
    NodeData rootNode_;
    glm::mat4 globalInverseTransform_{1.0f};

    void setupMesh();
    bool loadWithTinyObj(const char* path);
    bool loadWithAssimp(const char* path);
    void processAssimpNode(const aiNode* node, const aiScene* scene);
    void processAssimpMesh(const aiMesh* mesh);
    void loadMaterialTextures(const aiScene* scene);
    unsigned int loadTextureForMaterial(const std::string& relPath);
    unsigned int loadEmbeddedTexture(const aiScene* scene, const std::string& token);
    void readHierarchyData(NodeData& dest, const aiNode* src);
    void loadAnimations(const aiScene* scene);
    void extractBoneWeights(const aiMesh* mesh, std::vector<ModelVertex>& tempVertices);
    void normalizeBoneWeights();
    void calculateBoneTransforms(const AnimationClip& clip,
                                 double timeTicks,
                                 const NodeData& node,
                                 const glm::mat4& parent,
                                 std::vector<glm::mat4>& outMatrices) const;
    glm::mat4 convertMatrix(const aiMatrix4x4& mat) const;
    glm::vec3 convertVector(const aiVector3D& vec) const;
    glm::quat convertQuat(const aiQuaternion& quat) const;
    glm::mat4 interpolateChannelTransform(const NodeAnimationChannel& channel,
                                          double timeTicks) const;
    glm::vec3 interpolateVec3(const std::vector<std::pair<double, glm::vec3>>& keys,
                              double timeTicks) const;
    glm::quat interpolateQuat(const std::vector<std::pair<double, glm::quat>>& keys,
                              double timeTicks) const;
    std::string toLower(const std::string& value) const;
};
