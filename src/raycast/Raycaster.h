#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera;
class Terrain; // your terrain class

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

class Raycaster {
public:
    // screenX, screenY: pixels, origin bottom-left
    static Ray screenPointToRay(
        float screenX, float screenY,
        int windowWidth, int windowHeight,
        const Camera& camera
    );

    // Very basic terrain hit: assumes Terrain::getHeight(x,z)
    static bool raycastTerrain(
        const Ray& ray,
        const Terrain& terrain,
        glm::vec3& hitPoint,
        float maxDistance = 1000.0f,
        float step = 1.0f
    );
};
