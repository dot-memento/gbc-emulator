#include "shader_part.hpp"

#include <cstddef>
#include <array>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>

#include <glad/glad.h>

#include "logging.hpp"
#include "opengl_types.hpp"

namespace OpenGL
{
    namespace {

    glstring getShaderInfoLog(ShaderId shader)
    {
        GLsizei max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);
        
        glstring info_log(static_cast<size_t>(max_length), 0);
        glGetShaderInfoLog(shader, max_length, &max_length, info_log.data());

        return info_log;
    }

    }

    std::unique_ptr<ShaderPart> ShaderPart::compileFromFile(const std::filesystem::path& file_path, ShaderType type)
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            LOG_ERROR << "Failed to open '" << file_path << "'.";
            return {};
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return compileFromString(buffer.str(), type);
    }

    std::unique_ptr<ShaderPart> ShaderPart::compileFromString(const glstring& source_code, ShaderType type)
    {
        const ShaderId id = glCreateShader(type);
    
        std::array sources{ source_code.c_str() };
        glShaderSource(id, sources.size(), sources.data(), nullptr);

        glCompileShader(id);

        GLint is_compiled = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &is_compiled);
        if (is_compiled == GL_FALSE)
        {
            LOG_ERROR << "Shader compilation error: " << getShaderInfoLog(id);
            glDeleteShader(id);
            return {};
        }

        LOG_TRACE << "Shader part compiled (" << id << ") successfully.";

        return std::make_unique<ShaderPart>(id, type);
    }

    ShaderPart::~ShaderPart()
    {
        if (id_)
        {
            LOG_TRACE << "Deleting shader part (" << id_ << ')';
            glDeleteShader(id_);
        }
    }
}
