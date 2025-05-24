#pragma once

#include <string>
#include "cgp/cgp.hpp"

enum class image_format { jpg, png };

/// Load `count` frames named
///    base + "%0*d.{jpg|png}"
/// into a single GL_TEXTURE_2D_ARRAY.
/// 
/// @param base      Path + prefix, e.g. "assets/caustics/frame_"
/// @param count     Number of frames (0..count-1)
/// @param digits    Zero–padding digits (usually 4)
/// @param as_png    true = ".png", false = ".jpg"
/// @returns         GL handle, or 0 on error
GLuint create_texture_array_from_sequence(
    std::string const& base,
    int                count,
    int                digits  = 4,
    image_format               img_type  = image_format::jpg);