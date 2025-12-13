#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Scene.h"
#include "core/Camera.h"
#include "../common/Shader.h"   

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif

// --- Globals ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

Camera camera(glm::vec3(0.0f, 150.0f, 220.0f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -90.0f,
              -60.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Shadow map globals
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
unsigned int depthMapFBO = 0;
unsigned int depthMap    = 0;
Scene* gScene = nullptr;

void initShadowMap()
{
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main()
{
    // 1. Initialize Window & OpenGL
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    int winW = 1200, winH = 900;
    GLFWwindow* window = glfwCreateWindow(winW, winH, "RTS Engine - Shadows", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initShadowMap();

    // 2. Create Game Scene
    Scene gameScene;
    gScene = &gameScene;   
    gameScene.Init();

    // 3. Load Shaders
    Shader terrainShader(std::string(ASSET_PATH) + "shaders/terrain.vert",
                         std::string(ASSET_PATH) + "shaders/terrain.frag");

    Shader objectShader(std::string(ASSET_PATH) + "shaders/simple.vert",
                        std::string(ASSET_PATH) + "shaders/simple.frag");

    Shader depthShader(std::string(ASSET_PATH) + "shaders/shadow_depth.vert",
                       std::string(ASSET_PATH) + "shaders/shadow_depth.frag");

    // 🌞 FIX #2 — Better light for visible shadows
    glm::vec3 lightPos(-200.0f, 300.0f, -200.0f);

    // 4. Game Loop
    while (!glfwWindowShouldClose(window)) {
        // Time Logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);
        gameScene.Update(deltaTime);

        // --- Compute light-space matrix for directional light ---
        float orthoSize = 350.0f;  //// FIX #3 — cover entire terrain
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize,
                                               -orthoSize, orthoSize,
                                               1.0f, 800.0f);
        glm::mat4 lightView = glm::lookAt(lightPos,
                                          glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // ----------------- 1) Depth map pass -----------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.Use();
        depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        gameScene.DrawDepth(depthShader, lightSpaceMatrix);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //// FIX #1 — RESTORE SCREEN VIEWPORT correctly
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        // ----------------- 2) Normal render pass -----------------
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f);  // Sky Blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                float(fbW) / float(fbH),
                                                0.1f, 3000.0f);

        // Draw Scene with shadow map
        gameScene.Draw(terrainShader,
                       objectShader,
                       view,
                       projection,
                       lightPos,
                       camera.Position,
                       lightSpaceMatrix,
                       depthMap);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ------------- Input & Callbacks -------------

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool isMoving = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
        isMoving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
        isMoving = true;
    }

    if (!isMoving)
        camera.ResetSpeed();
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (gScene)
        gScene->onMouseMove(xpos, ypos);
}

// ⭐ NEW: Pass clicks to Scene (for UI clicks & Building Placement)
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (!gScene) return;

    // Only forward valid presses
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // You might need to adjust this depending on Scene.h signature
        // Assuming onMouseButton(button, action, mods) or onMouseClick(button)
        gScene->onMouseButton(button, action, mods); 
    }
}