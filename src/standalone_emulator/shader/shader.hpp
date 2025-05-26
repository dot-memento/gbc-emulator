#pragma once

#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <concepts>
#include <memory>
#include <utility>

#include <glad/glad.h>

#include "opengl_types.hpp"
#include "uniform_handle.hpp"

namespace OpenGL
{
    class ShaderPart;

    class Shader
    {
    public:
        static std::unique_ptr<Shader> makeFromParts(std::string name, const std::vector<const ShaderPart*>& parts);

        Shader(std::string name, ProgramId program_id) :
            name_{std::move(name)}, id_{program_id} {}
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        
        Shader(Shader&& other) :
            name_{std::move(other.name_)},
            id_{other.id_},
            uniform_list_{std::move(other.uniform_list_)}
        {
            assert(this != &other);
            other.id_ = 0;
        }

        Shader& operator=(Shader&& other)
        {
            assert(this != &other);
            
            name_ = std::move(other.name_);
            std::swap(id_, other.id_);
            uniform_list_ = std::move(other.uniform_list_);

            return *this;
        }

        inline std::string getName() const { return name_; }
        inline void useShader() const { glUseProgram(id_); }
        inline const std::vector<Uniform>& getUniformList() const { return uniform_list_; }
        
        template<class T> 
        const T* getUniform(const glstring& name) const
        {
            const auto it = std::find_if(uniform_list_.cbegin(), uniform_list_.cend(),
            [&name](const Uniform& uniform) {
                return uniform.getName() == name;
            });
            return (it == uniform_list_.cend()) ? nullptr : static_cast<const T*>(&(*it));
        }

    private:
        void findUniforms();

        std::string name_;
        ShaderId id_;
        std::vector<Uniform> uniform_list_;
    };
}
