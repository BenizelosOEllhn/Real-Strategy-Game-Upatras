#include "Raycaster.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Camera.h"  // adjust include path
#include "Terrain.h" // adjust include path

Ray Raycaster::screenPointToRay(
    float screenX, float screenY,
    int windowWidth, int windowHeight,
    const Camera& camera)
{
    // NDC calculation 
    float x = (2.0f * screenX) / windowWidth - 1.0f;
    float y = (2.0f * screenY) / windowHeight - 1.0f;
    glm::vec4 rayNDC(x, y, -1.0f, 1.0f);

    // FIX: Calculate Projection Matrix here using window dims and camera Zoom
    float aspect = (float)windowWidth / (float)windowHeight;
    // Ensure near/far planes match your main render loop (e.g., 0.1f, 1000.0f)
    glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 3000.0f);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 rayWorldH = invVP * rayNDC;
    rayWorldH /= rayWorldH.w;

    // FIX: Use the new GetPosition() accessor
    glm::vec3 camPos = camera.GetPosition(); 
    glm::vec3 dir = glm::normalize(glm::vec3(rayWorldH) - camPos);

    Ray ray;
    ray.origin = camPos;
    ray.dir    = dir;
    return ray;
}

bool Raycaster::raycastTerrain(
    const Ray& ray,
    const Terrain& terrain,
    glm::vec3& hitPoint,
    float maxDistance,
    float step)
{
    // VERY SIMPLE: march along the ray and compare y with terrain height
    float dist = 0.0f;
    glm::vec3 pos;

    while (dist < maxDistance) {
        pos = ray.origin + ray.dir * dist;
        float terrainY = terrain.getHeight(pos.x, pos.z); // <-- adapt name if needed

        if (pos.y <= terrainY) {
            pos.y = terrainY;
            hitPoint = pos;
            return true;
        }

        dist += step;
    }
    return false;
}
