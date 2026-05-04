#pragma once

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace Kojeom
{
struct GLMeshData
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    uint32_t indexCount = 0;
};

struct GLTextureData
{
    GLuint texture = 0;
    int width = 0;
    int height = 0;
};

struct GLShaderProgram
{
    GLuint program = 0;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
};

class OpenGLRenderer : public IRendererBackend
{
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override { Shutdown(); }

    bool Initialize(void* windowHandle) override
    {
        glfwMakeContextCurrent(static_cast<GLFWwindow*>(windowHandle));
        int version = gladLoadGL(glfwGetProcAddress);
        if (!version)
        {
            KE_LOG_ERROR("Failed to initialize GLAD");
            return false;
        }
        KE_LOG_INFO("OpenGL Version: {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const GLchar* message, const void* userParam)
        {
            if (severity == GL_DEBUG_SEVERITY_HIGH)
                KE_LOG_ERROR("GL: {}", message);
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
                KE_LOG_WARN("GL: {}", message);
        }, nullptr);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

        CreateDefaultShader();
        CreateDefaultMesh();
        KE_LOG_INFO("OpenGL Renderer initialized");
        return true;
    }

    void Shutdown() override
    {
        for (auto& [handle, mesh] : m_meshCache)
        {
            glDeleteVertexArrays(1, &mesh.vao);
            glDeleteBuffers(1, &mesh.vbo);
            glDeleteBuffers(1, &mesh.ibo);
        }
        m_meshCache.clear();

        for (auto& [handle, tex] : m_textureCache)
        {
            glDeleteTextures(1, &tex.texture);
        }
        m_textureCache.clear();

        for (auto& [handle, shader] : m_shaderCache)
        {
            glDeleteProgram(shader.program);
        }
        m_shaderCache.clear();

        KE_LOG_INFO("OpenGL Renderer shutdown");
    }

    void OnResize(int width, int height) override
    {
        glViewport(0, 0, width, height);
    }

    void BeginFrame() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void EndFrame() override
    {
    }

    void Render(const RenderScene& scene) override
    {
        if (!m_defaultShader.program) return;

        glUseProgram(m_defaultShader.program);

        GLint vpLoc = glGetUniformLocation(m_defaultShader.program, "uViewProjection");
        GLint modelLoc = glGetUniformLocation(m_defaultShader.program, "uModel");
        GLint lightDirLoc = glGetUniformLocation(m_defaultShader.program, "uLightDir");
        GLint lightColorLoc = glGetUniformLocation(m_defaultShader.program, "uLightColor");

        if (vpLoc >= 0)
            glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (lightDirLoc >= 0)
            glUniform3fv(lightDirLoc, 1, &scene.light.direction[0]);
        if (lightColorLoc >= 0)
            glUniform3fv(lightColorLoc, 1, &scene.light.color[0]);

        for (const auto& cmd : scene.staticDrawCommands)
        {
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            GLMeshData mesh = m_defaultMesh;
            auto it = m_meshCache.find(cmd.meshHandle);
            if (it != m_meshCache.end()) mesh = it->second;

            if (mesh.vao)
            {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        glUseProgram(0);
    }

    AssetHandle UploadMesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
    {
        static AssetHandle nextHandle = 1;
        AssetHandle handle = nextHandle++;

        GLMeshData mesh{};
        mesh.indexCount = static_cast<uint32_t>(indices.size());

        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glGenBuffers(1, &mesh.ibo);

        glBindVertexArray(mesh.vao);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
            vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
            indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
            reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
            reinterpret_cast<void*>(6 * sizeof(float)));

        glBindVertexArray(0);

        m_meshCache[handle] = mesh;
        return handle;
    }

    AssetHandle UploadTexture(int width, int height, int channels, const uint8_t* data)
    {
        static AssetHandle nextHandle = 1000;
        AssetHandle handle = nextHandle++;

        GLTextureData tex{};
        tex.width = width;
        tex.height = height;

        glGenTextures(1, &tex.texture);
        glBindTexture(GL_TEXTURE_2D, tex.texture);

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
            format, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        m_textureCache[handle] = tex;
        return handle;
    }

    void RemoveMesh(AssetHandle handle)
    {
        auto it = m_meshCache.find(handle);
        if (it != m_meshCache.end())
        {
            glDeleteVertexArrays(1, &it->second.vao);
            glDeleteBuffers(1, &it->second.vbo);
            glDeleteBuffers(1, &it->second.ibo);
            m_meshCache.erase(it);
        }
    }

    void RemoveTexture(AssetHandle handle)
    {
        auto it = m_textureCache.find(handle);
        if (it != m_textureCache.end())
        {
            glDeleteTextures(1, &it->second.texture);
            m_textureCache.erase(it);
        }
    }

    std::string GetName() const override { return "OpenGL 4.5"; }

    const GLMeshData* GetMesh(AssetHandle handle) const
    {
        auto it = m_meshCache.find(handle);
        return (it != m_meshCache.end()) ? &it->second : nullptr;
    }

    const GLTextureData* GetTexture(AssetHandle handle) const
    {
        auto it = m_textureCache.find(handle);
        return (it != m_textureCache.end()) ? &it->second : nullptr;
    }

private:
    void CreateDefaultShader()
    {
        const char* vertexSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;

            uniform mat4 uViewProjection;
            uniform mat4 uModel;

            out vec3 vNormal;
            out vec2 vTexCoord;

            void main()
            {
                gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
                vNormal = mat3(transpose(inverse(uModel))) * aNormal;
                vTexCoord = aTexCoord;
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            in vec3 vNormal;
            in vec2 vTexCoord;

            uniform vec3 uLightDir;
            uniform vec3 uLightColor;

            out vec4 FragColor;

            void main()
            {
                vec3 N = normalize(vNormal);
                vec3 L = normalize(-uLightDir);
                float NdotL = max(dot(N, L), 0.0);
                vec3 ambient = vec3(0.15);
                vec3 diffuse = uLightColor * NdotL;
                vec3 baseColor = vec3(0.8, 0.8, 0.8);
                FragColor = vec4(baseColor * (ambient + diffuse), 1.0);
            }
        )";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_defaultShader.program = glCreateProgram();
        glAttachShader(m_defaultShader.program, vs);
        glAttachShader(m_defaultShader.program, fs);
        glLinkProgram(m_defaultShader.program);

        GLint success;
        glGetProgramiv(m_defaultShader.program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(m_defaultShader.program, 512, nullptr, log);
            KE_LOG_ERROR("Shader link failed: {}", log);
            glDeleteProgram(m_defaultShader.program);
            m_defaultShader.program = 0;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        m_defaultShader.vertexShader = vs;
        m_defaultShader.fragmentShader = fs;
    }

    GLuint CompileShader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            KE_LOG_ERROR("Shader compile failed: {}", log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void CreateDefaultMesh()
    {
        std::vector<float> vertices = {
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        };
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            0, 3, 7, 7, 4, 0,
            1, 5, 6, 6, 2, 1,
            3, 2, 6, 6, 7, 3,
            0, 4, 5, 5, 1, 0,
        };

        m_defaultCubeHandle = UploadMesh(vertices, indices);
        auto it = m_meshCache.find(m_defaultCubeHandle);
        if (it != m_meshCache.end())
        {
            m_defaultMesh = it->second;
        }
    }

    GLShaderProgram m_defaultShader{};
    GLMeshData m_defaultMesh{};
    AssetHandle m_defaultCubeHandle = INVALID_HANDLE;
    std::unordered_map<AssetHandle, GLMeshData> m_meshCache;
    std::unordered_map<AssetHandle, GLTextureData> m_textureCache;
    std::unordered_map<AssetHandle, GLShaderProgram> m_shaderCache;
};
}
