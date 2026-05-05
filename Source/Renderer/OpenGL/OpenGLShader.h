#pragma once

#include <glad/gl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "Core/Log.h"

namespace Kojeom
{
struct GLShaderProgram
{
    GLuint program = 0;
};

class OpenGLShader
{
public:
    OpenGLShader() = default;
    ~OpenGLShader() { ClearAll(); }

    GLuint CompileShader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            KE_LOG_ERROR("Shader compile failed: {}", log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint LinkProgram(GLuint vs, GLuint fs)
    {
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetProgramInfoLog(program, 1024, nullptr, log);
            KE_LOG_ERROR("Shader link failed: {}", log);
            glDeleteProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    GLuint CreateProgram(const char* vertexSrc, const char* fragmentSrc)
    {
        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return 0;
        return LinkProgram(vs, fs);
    }

    void AddShader(const std::string& name, GLuint program)
    {
        m_shaders[name] = { program };
    }

    GLuint GetProgram(const std::string& name) const
    {
        auto it = m_shaders.find(name);
        return (it != m_shaders.end()) ? it->second.program : 0;
    }

    GLint GetUniformLocation(GLuint program, const char* name) const
    {
        return glGetUniformLocation(program, name);
    }

    void SetUniform1i(GLint loc, int value) { if (loc >= 0) glUniform1i(loc, value); }
    void SetUniform1f(GLint loc, float value) { if (loc >= 0) glUniform1f(loc, value); }
    void SetUniform2f(GLint loc, float v0, float v1) { if (loc >= 0) glUniform2f(loc, v0, v1); }
    void SetUniform3f(GLint loc, float v0, float v1, float v2) { if (loc >= 0) glUniform3f(loc, v0, v1, v2); }
    void SetUniform3fv(GLint loc, int count, const float* value) { if (loc >= 0) glUniform3fv(loc, count, value); }
    void SetUniformMatrix3fv(GLint loc, int count, const float* value) { if (loc >= 0) glUniformMatrix3fv(loc, count, GL_FALSE, value); }
    void SetUniformMatrix4fv(GLint loc, int count, const float* value) { if (loc >= 0) glUniformMatrix4fv(loc, count, GL_FALSE, value); }

    void DeleteProgram(const std::string& name)
    {
        auto it = m_shaders.find(name);
        if (it != m_shaders.end())
        {
            if (it->second.program) glDeleteProgram(it->second.program);
            m_shaders.erase(it);
        }
    }

    void ClearAll()
    {
        for (auto& [name, shader] : m_shaders)
        {
            if (shader.program) glDeleteProgram(shader.program);
        }
        m_shaders.clear();
    }

private:
    std::unordered_map<std::string, GLShaderProgram> m_shaders;
};
}
