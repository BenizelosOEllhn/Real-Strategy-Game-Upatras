#include "Texture.h"
#include <iostream>

// This defines the library implementation logic
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h" 

Texture::Texture(const char* path) {
    glGenTextures(1, &ID);
    
    int width, height, nrChannels;
    
    // Flip textures vertically because OpenGL expects Y=0 at the bottom
    stbi_set_flip_vertically_on_load(true); 
    
    // Load image
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;  // Standard JPG
        else if (nrChannels == 4)
            format = GL_RGBA; // PNG with Alpha (Transparency)

        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Wrapping: Repeat texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        // Filtering: Linear for smooth scaling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }

    stbi_image_free(data);
}

void Texture::Bind(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}