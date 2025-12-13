#include "Terrain.h"
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <algorithm>

// --- Helper Functions ---
float getHill(float x, float z, float hx, float hz, float height, float radius) {
float dx = x - hx;
float dz = z - hz;
float distSq = dx*dx + dz*dz;
float radiusSq = radius * radius;
if (distSq > radiusSq) return 0.0f;
float factor = 1.0f - (distSq / radiusSq);
return height * (factor * factor); 
}

float getRidge(float x, float z, float x1, float z1, float h1, float x2, float z2, float h2, float radius) {
float dx = x2 - x1;
float dz = z2 - z1;
float lenSq = dx*dx + dz*dz;
float px = x - x1;
float pz = z - z1;
float t = (px * dx + pz * dz) / lenSq;
if (t < 0.0f) t = 0.0f;
if (t > 1.0f) t = 1.0f;
float closestX = x1 + dx * t;
float closestZ = z1 + dz * t;
float distX = x - closestX;
float distZ = z - closestZ;
float distSq = distX*distX + distZ*distZ;
float radiusSq = radius * radius;
if (distSq > radiusSq) return 0.0f;
float currentPeakH = h1 + (h2 - h1) * t;
float factor = 1.0f - (distSq / radiusSq);
return currentPeakH * (factor * factor);
}

float Terrain::getHeight(float x, float z)
{
    // Map size (matches Scene: 600x600, world coords -300..300)
    const float mapW   = 600.0f;
    const float mapD   = 600.0f;
    const float halfW  = mapW * 0.5f;
    const float halfD  = mapD * 0.5f;

    // World coords (already centered from constructor usage)
    float X = x;
    float Z = z;

    float y     = 0.0f;
    float symX  = std::fabs(X);

    auto clamp = [](float v, float a, float b) {
        return (v < a) ? a : (v > b ? b : v);
    };

    // Normalized [0..1] along depth
    float Nz = (Z + halfD) / mapD;
    float Nx = (X + halfW) / mapW;

    // --------------------------------------------------------------------
    // 1) TRIANGULAR CAPE WITH SMOOTH, ROUNDED TIP
    //    - Base of cape at north (Z ≈ -220)
    //    - Tip of cape around Z = +270, fading smoothly to ocean
    // --------------------------------------------------------------------
    const float zBase = -220.0f;   // start narrowing
    const float zTip  =  270.0f;   // soft tip position

    float tCape = clamp((Z - zBase) / (zTip - zBase), 0.0f, 1.0f); // 0 north, 1 near tip

    const float maxWidth = 1.0f;   // full width at base
    const float minWidth = 0.3f;  // narrow width at tip
    float landWidth = maxWidth + (minWidth - maxWidth) * tCape;

    // Gentle rounding at tip
    float tipCurve = tCape * tCape * 0.15f;
    landWidth -= tipCurve;

    // Small shoreline noise for irregular beach
    float shoreNoise =
        std::sin(Z * 0.012f + X * 0.03f) * 0.04f +
        std::sin(X * 0.02f - Z * 0.025f) * 0.03f +
        std::cos(Z * 0.015f) * 0.02f;

    landWidth += shoreNoise;
    landWidth = clamp(landWidth, 0.18f, 1.0f);

    float left  = 0.5f - landWidth * 0.5f;
    float right = 0.5f + landWidth * 0.5f;

    float outside = 0.0f;
    if (Nx < left)      outside = left - Nx;
    else if (Nx > right)outside = Nx - right;

    // We'll apply coastal smoothing at the end based on "outside"
    // Also fade everything to ocean after zTip
    float tipFade = 0.0f;
    if (Z > zTip) {
        tipFade = clamp((Z - zTip) / 30.0f, 0.0f, 1.0f); // fade over last 30 units
    }

    // --------------------------------------------------------------------
    // 2) BASE PLAINS UNDULATION
    // --------------------------------------------------------------------
    y += 2.0f;
    y += std::sin(symX * 0.02f + Z * 0.02f) * 2.0f;
    y += std::cos(Z * 0.03f) * 1.5f;
    y += std::sin(symX * 0.25f) * std::cos(Z * 0.25f) * 0.25f;

    // --- MOUNTAIN RANGE (with cliffs) ---
    float northT = clamp((-120.0f - Z) / 180.0f, 0.0f, 1.0f);
    northT = northT * northT;  // smooth

    if (northT > 0.0f) {
        // Base mountain lift
        y += 22.0f * northT;

        // Cliffs: increase vertical height based on |X|
        float cliff = fabs(X) * 0.03f * northT;
        cliff = pow(cliff, 1.6f);  // punchy cliffs
        y += cliff * 8.0f;

        // Some noise variation
        y += sin(X * 0.02f + Z * 0.015f) * (4.0f * northT);
        y += cos(Z * 0.027f) * (3.0f * northT);
}

    

    float yBeforeLake = y; // store to blend lake nicely
    float inwardSlope = (1.0f - (symX / halfW)); // 1 at center, 0 at edges
    inwardSlope = clamp(inwardSlope, 0.0f, 1.0f);
    y += inwardSlope * 3.0f;  // adjust strength as needed

// ------------------------------------------------------------
// 4) IRREGULAR MOUNTAIN LAKE
// ------------------------------------------------------------
const float lakeX      = 0.0f;
const float lakeZ      = -200.0f;
const float lakeOuterR = 90.0f;
const float lakeInnerR = 55.0f;
const float lakeFloorH = -2.0f;

float lx = X - lakeX;
float lz = Z - lakeZ;
float distLake = sqrt(lx*lx + lz*lz);
float angle = atan2(lz, lx);

// Irregular radius
float irregular =
      1.0f
    + 0.18f * cos(3.0f * angle)
    + 0.08f * cos(5.0f * angle);

float innerR = lakeInnerR * irregular;
float outerR = lakeOuterR * irregular;

if (distLake < outerR)
{
    float t = (distLake > innerR)
        ? clamp((distLake - innerR) / (outerR - innerR), 0.0f, 1.0f)
        : 0.0f;

    float rimTarget = yBeforeLake * 0.7f + 0.9f;
    y = y * t + rimTarget * (1.0f - t);
}

if (distLake < innerR)
{
    float k = (innerR - distLake) / innerR;  
    k *= k;
    float bowlDepth = 2.0f + 1.5f * k;
    float target = lakeFloorH - bowlDepth * 0.3f;
    y = y * 0.25f + target * 0.75f;  
}


// ------------------------------------------------------------
// 5) TWO SYMMETRICAL RIVERS (deep + wide + curved)
// ------------------------------------------------------------
const float riverStartZ = lakeZ + 12.0f;
const float riverEndZ   = 260.0f;
const float minRiverH   = lakeFloorH + 1.0f ; // deeper rivers

auto carveRiver = [&](float startX, float dir)
{
    if (Z < riverStartZ || Z > riverEndZ)
        return;

    float t = (Z - riverStartZ) / (riverEndZ - riverStartZ);

    float endX  = dir * 180.0f;
    float pathX = glm::mix(startX, endX, t);

    pathX += dir * (30.0f * sin(Z * 0.035f));  
    pathX +=        (12.0f * cos(Z * 0.02f));   

    float halfWidth = glm::mix(28.0f, 18.0f, t);
    float fade = glm::clamp((Z - 150.0f) / 30.0f, 0.0f, 1.0f);
    halfWidth *= (1.0f - fade);

    float d = fabs(X - pathX);
    if (d > halfWidth)
        return;

    float u = (halfWidth - d) / halfWidth;
    float depth = 3.0f + 5.0f * (u * u);  // deeper

    y -= depth * u;
    if (y < minRiverH)
        y = minRiverH;
};

// Southwest river
carveRiver(-15.0f, -1.0f);

// Southeast river
carveRiver(+15.0f, +1.0f);



    // --------------------------------------------------------------------
    // 6) SOUTHERN PLAINS SMOOTHING (FLATTER FOR GAMEPLAY)
    // --------------------------------------------------------------------
    if (Z > 60.0f) {
        float t = clamp((Z - 60.0f) / 160.0f, 0.0f, 1.0f);
        float target = 1.5f + std::cos(symX * 0.06f) * 0.2f;
        y = y * (1.0f - t) + target * t;
    }

    // --------------------------------------------------------------------
    // 7) APPLY COASTAL SMOOTHING / OCEAN & TIP FADE
    // --------------------------------------------------------------------
    if (outside > 0.0f) {
        float t = clamp(outside * 7.0f, 0.0f, 1.0f);

        // First stage: beach toward height ~0
        float beach = std::min(t * 2.5f, 1.0f);
        y = y * (1.0f - beach) + 0.0f * beach;

        // Second stage: deeper ocean
        float deep = clamp((t - 0.4f) / 0.6f, 0.0f, 1.0f);
        y = y * (1.0f - deep) + (-12.0f) * deep;
    }

    // Fade out tip into ocean smoothly
    if (tipFade > 0.0f) {
        float beachH = -2.0f;
        float oceanH = -12.0f;
        float mid = beachH * (1.0f - tipFade) + oceanH * tipFade;
        y = y * (1.0f - tipFade) + mid * tipFade;
    }

    // --------------------------------------------------------------------
    // 8) GLOBAL CLAMPING TO AVOID CRAZY PEAKS / PITS
    // --------------------------------------------------------------------
    if (y > 45.0f) {
        float excess = y - 45.0f;
        y = 45.0f + excess * 0.35f;
    }
    if (y < -14.0f) {
        float excess = -14.0f - y;
        y = -14.0f - excess * 0.35f;
    }
    y = y * 1.2f + 0.5f;
    return y;
}



// --- Normal Calculation ---
glm::vec3 Terrain::getNormal(float x, float z) {
float epsilon = 0.1f;
float hL = getHeight(x - epsilon, z);
float hR = getHeight(x + epsilon, z);
float hD = getHeight(x, z - epsilon);
float hU = getHeight(x, z + epsilon);

glm::vec3 tanX(2.0f * epsilon, hR - hL, 0.0f);
glm::vec3 tanZ(0.0f, hU - hD, 2.0f * epsilon);

return glm::normalize(glm::cross(tanZ, tanX));
}

// --- Constructor & Mesh Setup ---
Terrain::Terrain(int w, int d) : width(w), depth(d) {
float halfW = width / 2.0f;
float halfD = depth / 2.0f;

// 1. Generate Vertices
for (int z = 0; z <= depth; ++z) {
for (int x = 0; x <= width; ++x) {
    Vertex vertex;
    float worldX = x - halfW;
    float worldZ = z - halfD;
    
    float worldY = getHeight(worldX, worldZ);
    vertex.Position = glm::vec3(worldX, worldY, worldZ);
    vertex.Normal = getNormal(worldX, worldZ);
    
    vertex.TexCoords = glm::vec2(worldX / 15.0f, worldZ / 15.0f);
    
    vertices.push_back(vertex);
}
}

// 2. Generate Indices
for (int z = 0; z < depth; ++z) {
for (int x = 0; x < width; ++x) {
    int topLeft = (z * (width + 1)) + x;
    int topRight = topLeft + 1;
    int bottomLeft = ((z + 1) * (width + 1)) + x;
    int bottomRight = bottomLeft + 1;

    indices.push_back(topLeft);
    indices.push_back(bottomLeft);
    indices.push_back(topRight);

    indices.push_back(topRight);
    indices.push_back(bottomLeft);
    indices.push_back(bottomRight);
}
}

setupMesh();
}

void Terrain::setupMesh() {
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glGenBuffers(1, &EBO);

glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
glEnableVertexAttribArray(0);

glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
glEnableVertexAttribArray(1);

glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
glEnableVertexAttribArray(2);

glBindVertexArray(0);
}

void Terrain::Draw(unsigned int shaderProgram) {
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
glBindVertexArray(0);
}

Terrain::~Terrain() {
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);
glDeleteBuffers(1, &EBO);
}
