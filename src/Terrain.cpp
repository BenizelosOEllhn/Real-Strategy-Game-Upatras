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

    // --- HEIGHT FUNCTION (270x300 Tuned) ---
    float Terrain::getHeight(float x, float z) {
        float y = 0.0f;
        float symX = std::abs(x); 

        // Tunable flattening controls to keep the battlefield more level.
        const float baseFlatness = 0.55f;
        const float ridgeFlatness = 1.05f;   // keep ridge present for gameplay sightlines
        const float ridgeLift = 1.75f;        // ridge sits taller overall
        const float mountainFlatness = 0.82f; // let the mountain heights breathe
        const float peakSharpness = 1.55f;
        const float mountainRoughness = 6.5f;
        const float slopeSoftCap = 21.0f;
        const float pitSoftCap = -6.0f;

        // Constants for this map size
        float mountainStart = -90.0f; 
        float lakeCenterZ = 80.0f;   
        float forestBasinStart = lakeCenterZ + 8.0f;
        
        // --- River Masking Logic ---
        float riverMask = 1.0f; 
        
        if (z < lakeCenterZ && z > mountainStart - 20.0f) {
            float distFromLake = lakeCenterZ - z;
            float basePathX = 25.0f + (distFromLake * 0.4f); 
            float snakeWiggle = std::sin(z * 0.08f) * 12.0f; 
            float riverIdealX = basePathX + snakeWiggle;
            float distToRiver = std::abs(symX - riverIdealX);
            float maskWidth = 22.0f;
            
            if (distToRiver < maskWidth) {
                float ratio = distToRiver / maskWidth;
                riverMask = ratio; 
            }
        }

        float plainReducer = (z > -30.0f && z < 150.0f) ? 0.55f : 1.0f;
        // 1. Base Ground (Wobbly but softened on plains)
        y += 1.5f;
        y += std::sin(symX * 0.024f + z * 0.018f) * 2.2f * baseFlatness * plainReducer; 
        y += std::cos(z * 0.03f) * 1.3f * baseFlatness * plainReducer;
        y += std::sin(symX * 0.25f) * std::cos(z * 0.25f) * 0.2f * baseFlatness * plainReducer; 

        // 2. Strategic Hills & Ridge (Masked)
        float ridgeWiggle = std::sin(z * 0.12f) * 10.0f;
        float curvedX = symX + ridgeWiggle;
        float ridgeH = getRidge(curvedX, z, 90.0f, -20.0f, 14.0f, 55.0f, 50.0f, 7.0f, 25.0f) * ridgeFlatness;
        if (ridgeH > 0.0f) {
            float ridgeSpan = std::exp(-std::pow((z + 5.0f) / 65.0f, 2.0f));
            y += (ridgeH + ridgeLift) * riverMask * ridgeSpan; 
            if (ridgeH > 1.0f) { 
                y += (std::sin(x * 0.2f) * std::cos(z * 0.2f) * 0.75f) * riverMask * ridgeSpan;
            }
        }

        // 3. Mountains
        if (z < mountainStart) {
            float distIntoMtn = mountainStart - z;
            float plateauSize = 40.0f; 
            if (distIntoMtn < plateauSize) {
                float t = distIntoMtn / plateauSize;
                y += 25.0f * t * mountainFlatness; 
                y += (std::sin(x * 0.12f) + std::cos(symX * 0.18f + z * 0.07f)) * 2.4f * mountainFlatness; 
            } else {
                float peakDist = distIntoMtn - plateauSize;
                float peakProgress = std::min(1.0f, peakDist / 85.0f);
                float jagged = 1.0f + 0.6f * std::sin(peakDist * 0.32f + z * 0.25f);
                float symmetryNoise = 0.85f + 0.45f * std::sin(symX * 0.14f + peakDist * 0.12f);
                float taper = 1.0f + 0.4f * (1.0f - std::min(1.0f, symX / 70.0f));
                
                y += 25.0f * mountainFlatness; 
                y += std::pow(std::max(0.0f, peakDist * 0.12f * jagged), 2.25f) * peakSharpness; 
                y += std::abs(std::sin(symX * 0.14f + peakDist * 0.07f)) * 15.0f * mountainFlatness * symmetryNoise; 
                y += std::cos(symX * 0.04f + z * 0.2f) * 4.0f * peakProgress * taper;   
                y += std::sin(symX * 0.55f + peakDist * 0.35f) * mountainRoughness * peakProgress; 
            }
        }

        // 4. Lake
        float lakeRadiusBase = 45.0f; 
        float angle = std::atan2(z - lakeCenterZ, x);
        float noise = std::sin(angle * 5.0f) * 5.0f + std::cos(angle * 12.0f) * 2.0f;      
        float effectiveRadius = lakeRadiusBase + noise;
        float distToLake = std::sqrt(x*x + std::pow(z - lakeCenterZ, 2));

        if (distToLake < effectiveRadius + 25.0f) {
            float blend = (distToLake - effectiveRadius) / 25.0f;
            if (blend < 0) blend = 0;
            y *= blend; 
        }
        if (distToLake < effectiveRadius) {
            float depth = (effectiveRadius - distToLake) * 0.45f;
            y -= depth;
            if (y < -12.0f) y = -12.0f; 
        }

        // 5. Rivers (Digging)
        if (z < lakeCenterZ && z > mountainStart - 20.0f) {
            float distFromLake = lakeCenterZ - z;
            float basePathX = 25.0f + (distFromLake * 0.4f); 
            float snakeWiggle = std::sin(z * 0.08f) * 12.0f; 
            float riverIdealX = basePathX + snakeWiggle;
            float distToRiver = std::abs(symX - riverIdealX);
            float riverWidth = 10.0f; 

            if (distToRiver < riverWidth) {
                float factor = (riverWidth - distToRiver) / riverWidth;
                float trench = factor * factor * 7.0f; 
                if (y > 18.0f) trench = 0.0f; 
                else if (y > 5.0f) trench *= 0.5f;
                y -= trench;
                if (y < -2.0f) y = -2.0f; 
            }
        }
        
        // 6. Forest Basin just below the lake to encourage dense woodland
        if (z > forestBasinStart) {
            float basinSpan = std::min(1.0f, (z - forestBasinStart) / 60.0f);
            float target = 1.4f + std::cos(symX * 0.08f) * 0.25f;
            float blend = 0.45f * basinSpan;
            y = y * (1.0f - blend) + target * blend;
        }

        // 7. Forest Plain (Deep South)
        if (z > 130.0f && symX < 50.0f) {
            float damp = (z - 130.0f) / 30.0f; 
            if (damp > 1.0f) damp = 1.0f;
            if (y > 2.0f) y = y * (1.0f - damp) + 2.0f * damp;
        }

        // 8. Bottom corner plains kept flatter for base building
        if (z > 140.0f && symX > 95.0f) {
            float zBlend = std::min(1.0f, (z - 140.0f) / 30.0f);
            float xBlend = std::min(1.0f, (symX - 95.0f) / 40.0f);
            float blend = zBlend * xBlend;
            float target = 1.0f;
            y = y * (1.0f - blend) + target * blend;
        }

        // Final global flattening: gently clamp steep peaks and deep pits.
        if (y > slopeSoftCap) {
            float excess = y - slopeSoftCap;
            y = slopeSoftCap + excess * 0.45f;
        }
        if (y < pitSoftCap) {
            float excess = pitSoftCap - y;
            y = pitSoftCap - excess * 0.35f;
        }

        y *= 0.9f; // squeeze all slopes just a bit

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
