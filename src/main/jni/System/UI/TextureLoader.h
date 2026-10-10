#pragma once
#include <GLES3/gl3.h>
#include "ImGui/stb_image.h"
#include <cstdio>

static GLuint astralTextureID = 0;
static bool astralLoaded = false;

inline GLuint LoadTextureFromMemory(const unsigned char* imageData, int imageSize)
{
    if (!imageData || imageSize <= 0) {
        printf("Invalid image data provided!\n");
        return 0;
    }

    int width, height, channels;
    unsigned char* data = stbi_load_from_memory(
        imageData, imageSize, &width, &height, &channels, 4
    );

    if (!data) {
        printf("Failed to load texture from memory! stbi_failure_reason: %s\n", stbi_failure_reason());
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
    
    printf("Texture loaded successfully! ID: %u, Size: %dx%d\n", texID, width, height);
    return texID;
}

inline GLuint LoadAstralTexture(const unsigned char* logoData, int logoSize)
{
    if (astralLoaded && astralTextureID != 0) {
        return astralTextureID;
    }

    astralTextureID = LoadTextureFromMemory(logoData, logoSize);
    
    if (astralTextureID != 0) {
        astralLoaded = true;
    }
    
    return astralTextureID;
}

inline void FreeTexture(GLuint* textureID)
{
    if (textureID && *textureID != 0) {
        glDeleteTextures(1, textureID);
        *textureID = 0;
    }
}

inline void CleanupAllTextures()
{
    FreeTexture(&astralTextureID);
    astralLoaded = false;
}

/*
inline float GetLogoOpacity() {
    return 1.0f;
}

inline float GetLogoSizeMultiplier() {
    return 1.0f;
} */
