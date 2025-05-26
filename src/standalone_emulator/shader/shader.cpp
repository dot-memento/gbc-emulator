#include "shader.hpp"

#include <string>
#include <vector>
#include <memory>
#include <utility>

#include "shader_part.hpp"
#include "logging.hpp"
#include "opengl_types.hpp"

namespace OpenGL
{
    namespace {

    glstring getProgramInfoLog(GLuint program)
    {
        GLsizei max_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

        glstring info_log(static_cast<size_t>(max_length), 0);
        glGetProgramInfoLog(program, max_length, &max_length, info_log.data());
        
        return info_log;
    }

    }

    std::unique_ptr<Shader> Shader::makeFromParts(std::string name, const std::vector<const ShaderPart*>& parts)
    {
        const ProgramId id = glCreateProgram();
        for (const ShaderPart* part : parts)
            glAttachShader(id, part->getId());

        // Why am I doing this again?
        glBindAttribLocation(id, 0, "v_coord");
        
        glLinkProgram(id);

        GLint is_linked = 0;
        glGetProgramiv(id, GL_LINK_STATUS, &is_linked);
        if (is_linked == GL_FALSE)
        {
            LOG_ERROR << "Program (shader) linking error: " << getProgramInfoLog(id);
            glDeleteProgram(id);
            return {};
        }

        for (const ShaderPart* part : parts)
            glDetachShader(id, part->getId());

        LOG_TRACE << "Shader '" << name << "' compiled successfully.";

        auto shader = std::make_unique<Shader>(std::move(name), id);
        shader->findUniforms();

        return shader;
    }

    Shader::~Shader()
    {
        if (id_)
        {
            LOG_TRACE << "Deleting shader '" << name_ << "'";
            glDeleteProgram(id_);
        }
    }
    
    void Shader::findUniforms()
    {
        constexpr static size_t max_uniform_name_length = 256;

        GLint uniform_count = 0;
        glGetProgramiv(id_, GL_ACTIVE_UNIFORMS, &uniform_count);

        for(GLuint i = 0; i < static_cast<GLuint>(uniform_count); ++i)
        {
            glstring uniform_name(max_uniform_name_length, 0);
            GLint array_size = 0;
            UniformType type = 0;
            GLsizei actual_length = 0;
            glGetActiveUniform(id_, i, static_cast<GLsizei>(uniform_name.size()), &actual_length, &array_size, &type, uniform_name.data());
            uniform_name.resize(static_cast<size_t>(actual_length));

            const GLint uniform_id = glGetUniformLocation(id_, uniform_name.c_str());
            if (uniform_id == -1)
            {
                LOG_ERROR << "Could not get uniform '" << uniform_name << "' location.";
                continue;
            }

            LOG_TRACE << "  Found uniform in shader '" << name_ << "': " << uniform_name << " (" << type << ')';

            uniform_list_.emplace_back(std::move(uniform_name), static_cast<UniformId>(uniform_id), type);
        }
    }
}
