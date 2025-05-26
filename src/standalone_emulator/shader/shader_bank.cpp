#include "shader_bank.hpp"

#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <utility>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <glad/glad.h>

#include "logging.hpp"
#include "shader_part.hpp"
#include "shader.hpp"
#include "constants.hpp"


namespace OpenGL
{
    namespace {

    constexpr const GLchar* vertex_source_code =
        "attribute vec2 v_coord;"
        "varying vec2 f_texcoord;"
        "void main(void) {"
          "gl_Position = vec4(v_coord, 0.0, 1.0);"
          "f_texcoord = (v_coord + 1.0) / 2.0;"
        "}";

    constexpr const GLchar* identity_fragment_source_code =
        "uniform sampler2D fbo_texture;"
        "varying vec2 f_texcoord;"
        "void main(void) {"
          "gl_FragColor = texture2D(fbo_texture, f_texcoord);"
        "}";

    std::unique_ptr<const ShaderPart> compiled_vertex_shader;
    std::unique_ptr<const ShaderPart> compiled_identity_fragment_shader;
    std::unique_ptr<Shader> default_shader;
    std::unordered_map<std::string, std::unique_ptr<Shader>> shader_cache;


    const ShaderPart* getCompiledVertexShader()
    {
        if (compiled_vertex_shader)
            return compiled_vertex_shader.get();
        
        compiled_vertex_shader = ShaderPart::compileFromString(vertex_source_code, GL_VERTEX_SHADER);
        if (compiled_vertex_shader)
            return compiled_vertex_shader.get();

        LOG_FATAL << "Failed to compile blit vertex shader; catching fire...";
        throw std::runtime_error("failed to compile blit vertex shader");
    }
    
    const ShaderPart* getCompiledIdentityFragmentShader()
    {
        if (compiled_identity_fragment_shader)
            return compiled_identity_fragment_shader.get();
        
        compiled_identity_fragment_shader = ShaderPart::compileFromString(identity_fragment_source_code, GL_FRAGMENT_SHADER);
        if (compiled_identity_fragment_shader)
            return compiled_identity_fragment_shader.get();

        LOG_FATAL << "Failed to compile indentity fragment shader; catching fire...";
        throw std::runtime_error("failed to compile indentity fragment shader");
    }

    std::string readWholeFile(const std::filesystem::path& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR << "Failed to open '" << filepath << "'";
            return {};
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    }
    
    void dropShaderCache()
    {
        LOG_DEBUG << "Clearing shader cache";
        shader_cache.clear();
        default_shader.release();
        compiled_vertex_shader.release();
        compiled_identity_fragment_shader.release();
    }

    const Shader* getScreenShaderFromName(const std::string& shader_name)
    {
        // Look for shader in cache
        const auto it = shader_cache.find(shader_name);
        if (it != shader_cache.end())
        {
            LOG_TRACE << "Shader '" << shader_name << "' found in cache.";
            return (*it).second.get();
        }

        LOG_DEBUG << "Shader '" << shader_name << "' not found in cache; compiling shader...";

        // Read the fragment shader file
        std::filesystem::path fragment_file_path{Constants::shader_directory};
        fragment_file_path /= shader_name;
        const std::string fragment_source_code = readWholeFile(fragment_file_path);
        if (fragment_source_code.empty())
        {
            LOG_ERROR << "Failed to load file '" << fragment_file_path << "'";
            return defaultShader();
        }

        // Compile the fragment shader and add it to the cache
        const std::unique_ptr<const ShaderPart> fragment_shader = ShaderPart::compileFromFile(fragment_file_path, GL_FRAGMENT_SHADER);
        if (!fragment_shader)
        {
            LOG_ERROR << "Failed to compile shader '" << fragment_file_path << "', using default shader instead.";
            return defaultShader();
        }

        // Link shader and add it to the cache
        std::unique_ptr<Shader> shader = Shader::makeFromParts(shader_name, {
            getCompiledVertexShader(),
            fragment_shader.get()
        });
        if (!shader)
        {
            LOG_ERROR << "Failed to link shader '" << fragment_file_path << "', using default shader instead.";
            return defaultShader();
        }

        shader_cache.emplace(shader_name, std::move(shader));

        return shader_cache[shader_name].get();
    }

    const Shader* defaultShader()
    {
        // Check if default shader is already compiled
        if (default_shader)
            return default_shader.get();

        LOG_DEBUG << "Default shader not compiled yet; compiling shader...";

        // Compile it if it's not
        default_shader = Shader::makeFromParts(default_shader_name, {
            getCompiledVertexShader(),
            getCompiledIdentityFragmentShader()
        });
        if (default_shader)
        {
            LOG_TRACE << "Default shader compiled successfully.";
            return default_shader.get();
        }

        // Fatal error if linking fails
        LOG_FATAL << "Failed to link default shader; catching fire...";
        throw std::runtime_error("failed to link default shader");
    }
}
