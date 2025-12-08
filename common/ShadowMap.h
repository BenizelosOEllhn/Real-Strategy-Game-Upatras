#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

class ShadowMap {
public:
    bool init(int width, int height);
    void bindForWriting() const;                 // use in depth pass
    void bindForReading(GLenum textureUnit) const; // bind as texture in main pass

    GLuint getDepthTexture() const { return depthTexture; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    glm::mat4 lightViewProj; // light-space matrix we’ll fill in Scene

private:
    GLuint fbo = 0;
    GLuint depthTexture = 0;
    int width = 0;
    int height = 0;
};
