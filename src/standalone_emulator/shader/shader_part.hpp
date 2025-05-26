#pragma once

#include <cassert>
#include <string>
#include <filesystem>
#include <memory>
#include <utility>

#include "opengl_types.hpp"

namespace OpenGL
{
    class ShaderPart
    {
    public:
        static std::unique_ptr<ShaderPart> compileFromFile(const std::filesystem::path& path, ShaderType type);
        static std::unique_ptr<ShaderPart> compileFromString(const glstring& source_code, ShaderType type);

        constexpr ShaderPart(ShaderId id, ShaderType type) : id_{id}, type_{type} {}
        ~ShaderPart();
        
        ShaderPart(const ShaderPart&) = delete;
        ShaderPart& operator=(const ShaderPart&) = delete;

        constexpr ShaderPart(ShaderPart&& other) :
            id_{other.id_}, type_{other.type_}
        {
            assert(this != &other);
            other.id_ = 0;
        }

        constexpr ShaderPart& operator=(ShaderPart&& other)
        {
            assert(this != &other);
            std::swap(id_, other.id_);
            type_ = other.type_;
            return *this;
        }

        constexpr ShaderId getId() const { return id_; }
        constexpr ShaderType getType() const { return type_; }

    private:

        ShaderId id_;
        ShaderType type_;
    };
}
