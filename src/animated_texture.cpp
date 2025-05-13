#include <stb_image.h>
#include "animated_texture.hpp"
#include <cstdio>
#include <iostream>

GLuint create_texture_array_from_sequence(
    std::string const& base,
    int                count,
    int                digits,
    image_format               img_type)
{
    if (count <= 0) return 0;

    // --- 1) Load first frame to get width & height ---
    char filename[512];
    snprintf(filename, sizeof(filename), 
             (base + "%0*d.%s").c_str(), 
             digits, 0, (img_type == image_format::png ? "png" : "jpg"));
    int W, H, C;
    stbi_uc* data = stbi_load(filename, &W, &H, &C, 1);
    if (!data) {
        std::cerr << "[animated_texture] Failed to load “" 
                  << filename << "”\n";
        return 0;
    }
    stbi_image_free(data);

    // --- 2) Create & bind the array texture ---
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);

    // Allocate one level, single channel (R8)
    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,          // mip level
                 GL_R8,      // internal format
                 W, H, count,// width, height, layers
                 0,          // border
                 GL_RED,     // format of incoming data
                 GL_UNSIGNED_BYTE,
                 nullptr);   // no data yet

    // --- 3) Fill each layer ---
    for(int i = 0; i < count; ++i) {
        snprintf(filename, sizeof(filename),
                 (base + "%0*d.%s").c_str(),
                 digits, i, (img_type == image_format::png ? "png" : "jpg"));

        int w2, h2, c2;
        stbi_uc* layer = stbi_load(filename, &w2, &h2, &c2, 1);
        if (!layer) {
            std::cerr << "[animated_texture] Missing “" 
                      << filename << "” - skipping\n";
            continue;
        }

        // upload into layer i
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,      // mip level
                        0, 0, i,// xoffset, yoffset, layer
                        w2, h2, 1,
                        GL_RED, GL_UNSIGNED_BYTE,
                        layer);

        stbi_image_free(layer);
    }

    // --- 4) Set filtering/wrap ---
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,     GL_REPEAT);

    return tex;
}