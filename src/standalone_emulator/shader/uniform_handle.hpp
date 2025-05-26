#pragma once

#include <cassert>

#include "opengl_types.hpp"
#include <glad/glad.h>

namespace OpenGL
{
    class Uniform
    {
    public:
        Uniform(glstring name, UniformId id, UniformType type) :
            name_{std::move(name)}, id_{id}, type_{type} {}

        inline glstring getName() const { return name_; }
        constexpr UniformId getId() const { return id_; }
        constexpr UniformType getType() const { return type_; }

    private:
        glstring name_;
        UniformId id_;
        UniformType type_;
    };

    class Texture2DUniform : public Uniform
    {
    public:
        void setValue(TextureId texture) const
        {
            assert(getType() == GL_SAMPLER_2D);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(static_cast<GLint>(getId()), 0); // 0 => GL_TEXTURE0
        }
    };
}
