#pragma once

#include <string>
#include <vector>

namespace OpenGL
{
    class Shader;
    
    const std::string default_shader_name{"default"};

    void dropShaderCache();
    const Shader* getScreenShaderFromName(const std::string& shader_name);
    const Shader* defaultShader();
}
