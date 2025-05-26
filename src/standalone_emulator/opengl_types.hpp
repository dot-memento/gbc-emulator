#pragma once

#include <string>

using GLchar  = char;
using GLuint  = unsigned int;
using GLint   = int;
using GLsizei = int;
using GLenum  = unsigned int;

namespace OpenGL
{
    // Custom aliases, mostly to keep track of what is what
    using ShaderType = GLenum;
    using ShaderId = GLuint;
    
    using ProgramId = GLuint;

    using UniformId = GLuint;
    using UniformType = GLenum;

    using TextureId = GLuint;

    using glstring = std::basic_string<GLchar>;
}
