#include "Scene.h"


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static void makeColorAttachment(GLuint& tex, int w, int h)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void makeDepthTexture(GLuint& tex, int w, int h)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// ------------------------------------------------------------
// Render targets init / destroy
// ------------------------------------------------------------
void Scene::initWaterRenderTargets(int w, int h)
{
    // scale down if you want performance (e.g. /2)
    waterRTWidth  = w;
    waterRTHeight = h;

    // ---- Reflection ----
    glGenFramebuffers(1, &reflectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);

    makeColorAttachment(reflectionColorTex, waterRTWidth, waterRTHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionColorTex, 0);

    glGenRenderbuffers(1, &reflectionDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, waterRTWidth, waterRTHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, reflectionDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "[Water] Reflection FBO incomplete!\n";

    // ---- Refraction ----
    glGenFramebuffers(1, &refractionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);

    makeColorAttachment(refractionColorTex, waterRTWidth, waterRTHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionColorTex, 0);

    makeDepthTexture(refractionDepthTex, waterRTWidth, waterRTHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, refractionDepthTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "[Water] Refraction FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::destroyWaterRenderTargets()
{
    if (reflectionDepthRBO) glDeleteRenderbuffers(1, &reflectionDepthRBO);
    if (reflectionColorTex) glDeleteTextures(1, &reflectionColorTex);
    if (reflectionFBO)      glDeleteFramebuffers(1, &reflectionFBO);

    if (refractionDepthTex) glDeleteTextures(1, &refractionDepthTex);
    if (refractionColorTex) glDeleteTextures(1, &refractionColorTex);
    if (refractionFBO)      glDeleteFramebuffers(1, &refractionFBO);

    reflectionDepthRBO = reflectionColorTex = reflectionFBO = 0;
    refractionDepthTex = refractionColorTex = refractionFBO = 0;
}

void Scene::Resize(int fbW, int fbH)
{
    if (fbW <= 0 || fbH <= 0) return;
    destroyWaterRenderTargets();
    initWaterRenderTargets(fbW, fbH);
}

// ------------------------------------------------------------
// Pass control
// ------------------------------------------------------------
void Scene::beginReflectionPass(int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene::beginRefractionPass(int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene::endWaterPass(int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
}

// ------------------------------------------------------------
// BIG OCEAN WATER PLANE
// ------------------------------------------------------------
void Scene::GenerateWaterGeometry()
{
    const int   resolution = 128;
    const float size       = 600.0f;
    const float y          = oceanY;
    const float uvScale    = 20.0f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float step = (size * 2.0f) / resolution;

    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            float xPos = -size + x * step;
            float zPos = -size + z * step;

            vertices.push_back(xPos);
            vertices.push_back(y);
            vertices.push_back(zPos);

            float u = (float)x / resolution * uvScale;
            float v = (float)z / resolution * uvScale;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            int tl = z * (resolution + 1) + x;
            int tr = tl + 1;
            int bl = (z + 1) * (resolution + 1) + x;
            int br = bl + 1;

            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }

    waterIndexCount = indices.size();

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (waterVBO) glDeleteBuffers(1, &waterVBO);
    if (waterEBO) glDeleteBuffers(1, &waterEBO);

    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);

    glBindVertexArray(waterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Scene::DrawWater(const glm::mat4& view, const glm::mat4& proj)
{
    if (!waterShader || !waterVAO) return;
    if (!reflectionColorTex || !refractionColorTex || !refractionDepthTex) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    waterShader->Use();
    waterShader->SetMat4("model", glm::mat4(1.0f));
    waterShader->SetMat4("view", view);
    waterShader->SetMat4("projection", proj);
    waterShader->SetFloat("time", (float)glfwGetTime());

    // old textures (your existing water shader can still use these if you want)
    if (waterTex)   waterTex->Bind(0);
    if (noiseTex)   noiseTex->Bind(1);
    if (overlayTex) overlayTex->Bind(2);

    // new RT textures
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, reflectionColorTex);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, refractionColorTex);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, refractionDepthTex);

    // foam noise (optional)
    if (foamTex) { foamTex->Bind(6); }
    else { glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, 0); }

    waterShader->SetInt("textureSampler", 0);
    waterShader->SetInt("noiseSampler",   1);
    waterShader->SetInt("overlaySampler", 2);

    waterShader->SetInt("uReflection",      3);
    waterShader->SetInt("uRefraction",      4);
    waterShader->SetInt("uRefractionDepth", 5);
    waterShader->SetInt("uFoamNoise",       6);

    // depth linearization params (match your projection near/far)
    waterShader->SetFloat("uNear", 0.1f);
    waterShader->SetFloat("uFar",  3000.0f);
    waterShader->SetFloat("uWaterY", oceanY);
    waterShader->SetVec2("uScreenSize", glm::vec2((float)waterRTWidth, (float)waterRTHeight));

    glBindVertexArray(waterVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)waterIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// ------------------------------------------------------------
// LAKE WATER MESH
// ------------------------------------------------------------
void Scene::generateLakeWater()
{
    const float lakeX = 0.0f;
    const float lakeZ = kLakeCenterZ;

    const float lakeOuterR = 90.0f;
    const float lakeInnerR = 55.0f;

    const float waterY = lakeY;
    const int segments = 96;

    lakeWaterVerts.clear();
    lakeWaterIndices.clear();

    lakeWaterVerts.push_back({ glm::vec3(lakeX, waterY, lakeZ), glm::vec2(0.5f, 0.5f) });

    for (int i = 0; i <= segments; ++i)
    {
        float a = (float)i / segments * 2.0f * glm::pi<float>();

        float irregular =
            1.0f
            + 0.18f * std::cos(3.0f * a)
            + 0.08f * std::cos(5.0f * a);

        float innerR = lakeInnerR * irregular;
        float outerR = lakeOuterR * irregular;
        float waterR = innerR + (outerR - innerR) * 0.55f;

        float x = lakeX + std::cos(a) * waterR;
        float z = lakeZ + std::sin(a) * waterR;

        lakeWaterVerts.push_back({
            glm::vec3(x, waterY, z),
            glm::vec2((std::cos(a) + 1.0f) * 0.5f,
                      (std::sin(a) + 1.0f) * 0.5f)
        });
    }

    for (int i = 1; i <= segments; ++i)
    {
        lakeWaterIndices.push_back(0);
        lakeWaterIndices.push_back(i);
        lakeWaterIndices.push_back(i + 1);
    }

    uploadLakeWaterMesh();
}

void Scene::uploadLakeWaterMesh()
{
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (lakeVBO) glDeleteBuffers(1, &lakeVBO);
    if (lakeEBO) glDeleteBuffers(1, &lakeEBO);

    glGenVertexArrays(1, &lakeVAO);
    glGenBuffers(1, &lakeVBO);
    glGenBuffers(1, &lakeEBO);

    glBindVertexArray(lakeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, lakeVBO);
    glBufferData(GL_ARRAY_BUFFER, lakeWaterVerts.size() * sizeof(WaterVertex), lakeWaterVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lakeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lakeWaterIndices.size() * sizeof(unsigned int), lakeWaterIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, uv));

    glBindVertexArray(0);
}

void Scene::DrawLakeWater(const glm::mat4& view, const glm::mat4& proj)
{
    // For now: reuse DrawWater pipeline (same shader + same RTs)
    // If you want separate distortion per lake, we can add it later.
    if (!waterShader || !lakeVAO) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    waterShader->Use();
    waterShader->SetMat4("model", glm::mat4(1.0f));
    waterShader->SetMat4("view", view);
    waterShader->SetMat4("projection", proj);
    waterShader->SetFloat("time", (float)glfwGetTime());

    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, reflectionColorTex);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, refractionColorTex);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, refractionDepthTex);

    waterShader->SetInt("uReflection",      3);
    waterShader->SetInt("uRefraction",      4);
    waterShader->SetInt("uRefractionDepth", 5);

    waterShader->SetFloat("uNear", 0.1f);
    waterShader->SetFloat("uFar",  3000.0f);
    waterShader->SetFloat("uWaterY", lakeY);

    glBindVertexArray(lakeVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)lakeWaterIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// ------------------------------------------------------------
// RIVER WATER MESH
// ------------------------------------------------------------
void Scene::generateRiverWater()
{
    const float lakeZ       = kLakeCenterZ;
    const float riverStartZ = lakeZ + 12.0f;
    const float riverEndZ   = 260.0f;
    const float visualEndZ  = 180.0f;

    const float waterY = riverY;
    const float step   = 3.0f;

    riverWaterVerts.clear();
    riverWaterIndices.clear();

    auto evalSlice = [&](float z, float startX, float dir, float& outLeft, float& outRight) -> bool
    {
        if (z < riverStartZ || z > riverEndZ) return false;

        float t = (z - riverStartZ) / (riverEndZ - riverStartZ);

        float endX  = dir * 180.0f;
        float pathX = glm::mix(startX, endX, t);

        pathX += dir * (30.0f * std::sin(z * 0.035f));
        pathX +=        (12.0f * std::cos(z * 0.02f));

        float halfWidth = glm::mix(28.0f, 18.0f, t);
        float fade = glm::clamp((z - 150.0f) / 30.0f, 0.0f, 1.0f);
        halfWidth *= (1.0f - fade);

        if (halfWidth <= 0.1f) return false;

        outLeft  = pathX - halfWidth;
        outRight = pathX + halfWidth;
        return true;
    };

    const float meshStartZ = riverStartZ + 18.0f;

    for (float z = meshStartZ; z <= visualEndZ; z += step)
    {
        float lLeft, lRight, rLeft, rRight;
        bool okL = evalSlice(z, -15.0f, -1.0f, lLeft, lRight);
        bool okR = evalSlice(z, +15.0f, +1.0f, rLeft, rRight);

        if (!okL && !okR) continue;

        WaterVertex v0 { glm::vec3(lLeft,  waterY, z), glm::vec2(0.0f, 0.0f) };
        WaterVertex v1 { glm::vec3(lRight, waterY, z), glm::vec2(1.0f, 0.0f) };
        WaterVertex v2 { glm::vec3(rLeft,  waterY, z), glm::vec2(0.0f, 1.0f) };
        WaterVertex v3 { glm::vec3(rRight, waterY, z), glm::vec2(1.0f, 1.0f) };

        riverWaterVerts.push_back(v0);
        riverWaterVerts.push_back(v1);
        riverWaterVerts.push_back(v2);
        riverWaterVerts.push_back(v3);
    }

    int count = (int)riverWaterVerts.size();
    for (int i = 0; i < count - 4; i += 4)
    {
        riverWaterIndices.push_back(i);     riverWaterIndices.push_back(i + 1); riverWaterIndices.push_back(i + 4);
        riverWaterIndices.push_back(i + 1); riverWaterIndices.push_back(i + 5); riverWaterIndices.push_back(i + 4);

        riverWaterIndices.push_back(i + 2); riverWaterIndices.push_back(i + 3); riverWaterIndices.push_back(i + 6);
        riverWaterIndices.push_back(i + 3); riverWaterIndices.push_back(i + 7); riverWaterIndices.push_back(i + 6);
    }

    uploadRiverWaterMesh();
}

void Scene::uploadRiverWaterMesh()
{
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
    if (riverVBO) glDeleteBuffers(1, &riverVBO);
    if (riverEBO) glDeleteBuffers(1, &riverEBO);

    glGenVertexArrays(1, &riverVAO);
    glGenBuffers(1, &riverVBO);
    glGenBuffers(1, &riverEBO);

    glBindVertexArray(riverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, riverVBO);
    glBufferData(GL_ARRAY_BUFFER, riverWaterVerts.size() * sizeof(WaterVertex), riverWaterVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, riverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, riverWaterIndices.size() * sizeof(unsigned int), riverWaterIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void*)offsetof(WaterVertex, uv));

    glBindVertexArray(0);
}

void Scene::DrawRiverWater(const glm::mat4& view, const glm::mat4& proj)
{
    if (!waterShader || !riverVAO) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    waterShader->Use();
    waterShader->SetMat4("model", glm::mat4(1.0f));
    waterShader->SetMat4("view", view);
    waterShader->SetMat4("projection", proj);
    waterShader->SetFloat("time", (float)glfwGetTime());

    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, reflectionColorTex);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, refractionColorTex);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, refractionDepthTex);

    waterShader->SetInt("uReflection",      3);
    waterShader->SetInt("uRefraction",      4);
    waterShader->SetInt("uRefractionDepth", 5);

    waterShader->SetFloat("uNear", 0.1f);
    waterShader->SetFloat("uFar",  3000.0f);
    waterShader->SetFloat("uWaterY", riverY);

    glBindVertexArray(riverVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)riverWaterIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}
