#pragma once

#include <glad/gl.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <array>

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
};

struct GLMaterialData
{
    Vec3 albedo = Vec3(0.8f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    GLuint albedoTexture = 0;
    bool hasAlbedoTexture = false;
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
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        CreatePBRShader();
        CreateSkinnedShader();
        CreateDefaultMesh();
        CreateDefaultTextures();
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

        for (auto& [handle, mat] : m_materialCache)
        {
            if (mat.albedoTexture) glDeleteTextures(1, &mat.albedoTexture);
        }
        m_materialCache.clear();

        if (m_pbrShader.program) { glDeleteProgram(m_pbrShader.program); m_pbrShader.program = 0; }
        if (m_skinnedShader.program) { glDeleteProgram(m_skinnedShader.program); m_skinnedShader.program = 0; }
        if (m_defaultAlbedoTexture) { glDeleteTextures(1, &m_defaultAlbedoTexture); m_defaultAlbedoTexture = 0; }
        if (m_defaultNormalTexture) { glDeleteTextures(1, &m_defaultNormalTexture); m_defaultNormalTexture = 0; }

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
        RenderStaticMeshes(scene);
        RenderSkinnedMeshes(scene);
        RenderTerrain(scene);
    }

    AssetHandle UploadMesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
        int vertexStride = 8)
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

        int strideBytes = vertexStride * static_cast<int>(sizeof(float));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideBytes, nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, strideBytes,
            reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, strideBytes,
            reinterpret_cast<void*>(6 * sizeof(float)));

        glBindVertexArray(0);

        m_meshCache[handle] = mesh;
        return handle;
    }

    AssetHandle UploadSkinnedMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices)
    {
        static AssetHandle nextHandle = 5000;
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

        int stride = 14 * static_cast<int>(sizeof(float));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<void*>(12 * sizeof(float)));

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

    AssetHandle RegisterGLMaterial(const Vec3& albedo, float metallic, float roughness,
        GLuint albedoTex = 0, bool hasTex = false)
    {
        static AssetHandle nextHandle = 8000;
        AssetHandle handle = nextHandle++;
        GLMaterialData mat;
        mat.albedo = albedo;
        mat.metallic = metallic;
        mat.roughness = roughness;
        mat.albedoTexture = albedoTex;
        mat.hasAlbedoTexture = hasTex;
        m_materialCache[handle] = mat;
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

    const GLMaterialData* GetMaterial(AssetHandle handle) const
    {
        auto it = m_materialCache.find(handle);
        return (it != m_materialCache.end()) ? &it->second : nullptr;
    }

    AssetHandle GetDefaultCubeHandle() const { return m_defaultCubeHandle; }
    AssetHandle GetDefaultPlaneHandle() const { return m_defaultPlaneHandle; }
    GLuint GetDefaultAlbedoTexture() const { return m_defaultAlbedoTexture; }

private:
    void RenderStaticMeshes(const RenderScene& scene)
    {
        if (!m_pbrShader.program) return;
        glUseProgram(m_pbrShader.program);

        SetPBRUniforms(scene);

        for (const auto& cmd : scene.staticDrawCommands)
        {
            GLint modelLoc = glGetUniformLocation(m_pbrShader.program, "uModel");
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            SetMaterialUniforms(cmd.materialHandle);

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

    void RenderSkinnedMeshes(const RenderScene& scene)
    {
        if (!m_skinnedShader.program || scene.skinnedDrawCommands.empty()) return;
        glUseProgram(m_skinnedShader.program);

        SetSkinnedUniforms(scene);

        for (const auto& cmd : scene.skinnedDrawCommands)
        {
            GLint modelLoc = glGetUniformLocation(m_skinnedShader.program, "uModel");
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            GLint boneLoc = glGetUniformLocation(m_skinnedShader.program, "uBoneMatrices[0]");
            auto it = m_boneMatricesCache.find(cmd.boneMatricesHandle);
            if (boneLoc >= 0 && it != m_boneMatricesCache.end())
            {
                glUniformMatrix4fv(boneLoc, static_cast<GLsizei>(it->second.size()),
                    GL_FALSE, &it->second[0][0][0]);
            }

            GLMeshData mesh;
            auto meshIt = m_meshCache.find(cmd.skeletalMeshHandle);
            if (meshIt != m_meshCache.end()) mesh = meshIt->second;

            if (mesh.vao)
            {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        glUseProgram(0);
    }

    void RenderTerrain(const RenderScene& scene)
    {
        if (!m_pbrShader.program || scene.terrainDrawCommands.empty()) return;
        glUseProgram(m_pbrShader.program);

        SetPBRUniforms(scene);

        for (const auto& cmd : scene.terrainDrawCommands)
        {
            GLint modelLoc = glGetUniformLocation(m_pbrShader.program, "uModel");
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            SetMaterialUniforms(cmd.materialHandle);

            auto it = m_meshCache.find(cmd.terrainHandle);
            if (it != m_meshCache.end() && it->second.vao)
            {
                glBindVertexArray(it->second.vao);
                glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        glUseProgram(0);
    }

    void SetPBRUniforms(const RenderScene& scene)
    {
        GLint vpLoc = glGetUniformLocation(m_pbrShader.program, "uViewProjection");
        GLint camPosLoc = glGetUniformLocation(m_pbrShader.program, "uCameraPos");
        GLint lightDirLoc = glGetUniformLocation(m_pbrShader.program, "uLightDirection");
        GLint lightColorLoc = glGetUniformLocation(m_pbrShader.program, "uLightColor");
        GLint lightIntLoc = glGetUniformLocation(m_pbrShader.program, "uLightIntensity");
        GLint ambientLoc = glGetUniformLocation(m_pbrShader.program, "uAmbientColor");

        if (vpLoc >= 0)
            glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (camPosLoc >= 0)
            glUniform3fv(camPosLoc, 1, &scene.camera.position[0]);
        if (lightDirLoc >= 0)
            glUniform3fv(lightDirLoc, 1, &scene.light.direction[0]);
        if (lightColorLoc >= 0)
            glUniform3fv(lightColorLoc, 1, &scene.light.color[0]);
        if (lightIntLoc >= 0)
            glUniform1f(lightIntLoc, scene.light.intensity);
        if (ambientLoc >= 0)
            glUniform3fv(ambientLoc, 1, &scene.light.ambientColor[0]);
    }

    void SetSkinnedUniforms(const RenderScene& scene)
    {
        GLint vpLoc = glGetUniformLocation(m_skinnedShader.program, "uViewProjection");
        GLint camPosLoc = glGetUniformLocation(m_skinnedShader.program, "uCameraPos");
        GLint lightDirLoc = glGetUniformLocation(m_skinnedShader.program, "uLightDirection");
        GLint lightColorLoc = glGetUniformLocation(m_skinnedShader.program, "uLightColor");
        GLint lightIntLoc = glGetUniformLocation(m_skinnedShader.program, "uLightIntensity");

        if (vpLoc >= 0)
            glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (camPosLoc >= 0)
            glUniform3fv(camPosLoc, 1, &scene.camera.position[0]);
        if (lightDirLoc >= 0)
            glUniform3fv(lightDirLoc, 1, &scene.light.direction[0]);
        if (lightColorLoc >= 0)
            glUniform3fv(lightColorLoc, 1, &scene.light.color[0]);
        if (lightIntLoc >= 0)
            glUniform1f(lightIntLoc, scene.light.intensity);
    }

    void SetMaterialUniforms(AssetHandle materialHandle)
    {
        GLint albedoLoc = glGetUniformLocation(m_pbrShader.program, "uAlbedo");
        GLint metallicLoc = glGetUniformLocation(m_pbrShader.program, "uMetallic");
        GLint roughnessLoc = glGetUniformLocation(m_pbrShader.program, "uRoughness");
        GLint aoLoc = glGetUniformLocation(m_pbrShader.program, "uAO");
        GLint useTexLoc = glGetUniformLocation(m_pbrShader.program, "uUseAlbedoTexture");

        if (materialHandle != INVALID_HANDLE)
        {
            auto* mat = GetMaterial(materialHandle);
            if (mat)
            {
                if (albedoLoc >= 0) glUniform3fv(albedoLoc, 1, &mat->albedo[0]);
                if (metallicLoc >= 0) glUniform1f(metallicLoc, mat->metallic);
                if (roughnessLoc >= 0) glUniform1f(roughnessLoc, mat->roughness);
                if (aoLoc >= 0) glUniform1f(aoLoc, mat->ao);
                if (useTexLoc >= 0) glUniform1i(useTexLoc, mat->hasAlbedoTexture ? 1 : 0);

                if (mat->hasAlbedoTexture && mat->albedoTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat->albedoTexture);
                    GLint texLoc = glGetUniformLocation(m_pbrShader.program, "uAlbedoTexture");
                    if (texLoc >= 0) glUniform1i(texLoc, 0);
                }
                return;
            }
        }

        if (albedoLoc >= 0) glUniform3f(albedoLoc, 0.8f, 0.8f, 0.8f);
        if (metallicLoc >= 0) glUniform1f(metallicLoc, 0.0f);
        if (roughnessLoc >= 0) glUniform1f(roughnessLoc, 0.5f);
        if (aoLoc >= 0) glUniform1f(aoLoc, 1.0f);
        if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);
    }

    void CreatePBRShader()
    {
        const char* vertexSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;

            uniform mat4 uViewProjection;
            uniform mat4 uModel;

            out vec3 vWorldPos;
            out vec3 vNormal;
            out vec2 vTexCoord;

            void main()
            {
                vec4 worldPos = uModel * vec4(aPosition, 1.0);
                vWorldPos = worldPos.xyz;
                vNormal = mat3(transpose(inverse(uModel))) * aNormal;
                vTexCoord = aTexCoord;
                gl_Position = uViewProjection * worldPos;
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            in vec3 vWorldPos;
            in vec3 vNormal;
            in vec2 vTexCoord;

            uniform vec3 uCameraPos;
            uniform vec3 uLightDirection;
            uniform vec3 uLightColor;
            uniform float uLightIntensity;
            uniform vec3 uAmbientColor;

            uniform vec3 uAlbedo;
            uniform float uMetallic;
            uniform float uRoughness;
            uniform float uAO;
            uniform bool uUseAlbedoTexture;
            uniform sampler2D uAlbedoTexture;

            out vec4 FragColor;

            const float PI = 3.14159265359;

            float DistributionGGX(vec3 N, vec3 H, float roughness)
            {
                float a = roughness * roughness;
                float a2 = a * a;
                float NdotH = max(dot(N, H), 0.0);
                float NdotH2 = NdotH * NdotH;
                float denom = NdotH2 * (a2 - 1.0) + 1.0;
                return a2 / (PI * denom * denom + 0.0001);
            }

            float GeometrySchlickGGX(float NdotV, float roughness)
            {
                float r = roughness + 1.0;
                float k = (r * r) / 8.0;
                return NdotV / (NdotV * (1.0 - k) + k);
            }

            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
            {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
            }

            vec3 fresnelSchlick(float cosTheta, vec3 F0)
            {
                return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
            }

            void main()
            {
                vec3 albedo = uUseAlbedoTexture ? texture(uAlbedoTexture, vTexCoord).rgb : uAlbedo;

                vec3 N = normalize(vNormal);
                vec3 V = normalize(uCameraPos - vWorldPos);
                vec3 L = normalize(-uLightDirection);
                vec3 H = normalize(V + L);

                vec3 F0 = mix(vec3(0.04), albedo, uMetallic);

                float D = DistributionGGX(N, H, uRoughness);
                float G = GeometrySmith(N, V, L, uRoughness);
                vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

                vec3 numerator = D * G * F;
                float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
                vec3 specular = numerator / denominator;

                vec3 kS = F;
                vec3 kD = (vec3(1.0) - kS) * (1.0 - uMetallic);

                float NdotL = max(dot(N, L), 0.0);
                vec3 Lo = (kD * albedo / PI + specular) * uLightColor * uLightIntensity * NdotL;

                vec3 ambient = uAmbientColor * albedo * uAO;
                vec3 color = ambient + Lo;

                color = color / (color + vec3(1.0));
                color = pow(color, vec3(1.0 / 2.2));

                FragColor = vec4(color, 1.0);
            }
        )";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_pbrShader.program = LinkProgram(vs, fs);
    }

    void CreateSkinnedShader()
    {
        const char* vertexSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            layout(location = 3) in vec4 aBoneWeights;
            layout(location = 4) in vec4 aBoneIndices;

            uniform mat4 uViewProjection;
            uniform mat4 uModel;
            uniform mat4 uBoneMatrices[128];

            out vec3 vWorldPos;
            out vec3 vNormal;
            out vec2 vTexCoord;

            void main()
            {
                ivec4 boneIdx = ivec4(aBoneIndices);
                vec4 weights = aBoneWeights;

                mat4 skinMatrix =
                    weights.x * uBoneMatrices[boneIdx.x] +
                    weights.y * uBoneMatrices[boneIdx.y] +
                    weights.z * uBoneMatrices[boneIdx.z] +
                    weights.w * uBoneMatrices[boneIdx.w];

                vec4 skinnedPos = skinMatrix * vec4(aPosition, 1.0);
                vec4 skinnedNormal = skinMatrix * vec4(aNormal, 0.0);

                vec4 worldPos = uModel * skinnedPos;
                vWorldPos = worldPos.xyz;
                vNormal = mat3(transpose(inverse(uModel))) * skinnedNormal.xyz;
                vTexCoord = aTexCoord;
                gl_Position = uViewProjection * worldPos;
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            in vec3 vWorldPos;
            in vec3 vNormal;
            in vec2 vTexCoord;

            uniform vec3 uCameraPos;
            uniform vec3 uLightDirection;
            uniform vec3 uLightColor;
            uniform float uLightIntensity;

            out vec4 FragColor;

            void main()
            {
                vec3 N = normalize(vNormal);
                vec3 L = normalize(-uLightDirection);
                float NdotL = max(dot(N, L), 0.0);

                vec3 baseColor = vec3(0.8, 0.6, 0.4);
                vec3 ambient = vec3(0.15);
                vec3 diffuse = uLightColor * NdotL * uLightIntensity;

                vec3 color = baseColor * (ambient + diffuse);
                color = color / (color + vec3(1.0));
                color = pow(color, vec3(1.0 / 2.2));
                FragColor = vec4(color, 1.0);
            }
        )";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_skinnedShader.program = LinkProgram(vs, fs);
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

    void CreateDefaultMesh()
    {
        std::vector<float> cubeVerts = {
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,

            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,

             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        };

        std::vector<uint32_t> cubeIdx = {
            0,  1,  2,   2,  3,  0,
            4,  5,  6,   6,  7,  4,
            8,  9,  10,  10, 11, 8,
            12, 13, 14,  14, 15, 12,
            16, 17, 18,  18, 19, 16,
            20, 21, 22,  22, 23, 20,
        };

        m_defaultCubeHandle = UploadMesh(cubeVerts, cubeIdx);
        auto it = m_meshCache.find(m_defaultCubeHandle);
        if (it != m_meshCache.end())
        {
            m_defaultMesh = it->second;
        }

        std::vector<float> planeVerts = {
            -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f,
            -50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 50.0f,
             50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f, 50.0f, 50.0f,
             50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f, 50.0f,  0.0f,
        };
        std::vector<uint32_t> planeIdx = { 0, 1, 2, 2, 3, 0 };
        m_defaultPlaneHandle = UploadMesh(planeVerts, planeIdx);
    }

    void CreateDefaultTextures()
    {
        uint8_t white4x4[] = { 255,255,255,255, 255,255,255,255,
                               255,255,255,255, 255,255,255,255 };
        glGenTextures(1, &m_defaultAlbedoTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultAlbedoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, white4x4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        uint8_t normalDefault[] = { 128,128,255,255, 128,128,255,255,
                                    128,128,255,255, 128,128,255,255 };
        glGenTextures(1, &m_defaultNormalTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultNormalTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, normalDefault);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

public:
    std::unordered_map<AssetHandle, std::vector<Mat4>> m_boneMatricesCache;

private:
    GLShaderProgram m_pbrShader{};
    GLShaderProgram m_skinnedShader{};
    GLMeshData m_defaultMesh{};
    AssetHandle m_defaultCubeHandle = INVALID_HANDLE;
    AssetHandle m_defaultPlaneHandle = INVALID_HANDLE;
    GLuint m_defaultAlbedoTexture = 0;
    GLuint m_defaultNormalTexture = 0;
    std::unordered_map<AssetHandle, GLMeshData> m_meshCache;
    std::unordered_map<AssetHandle, GLTextureData> m_textureCache;
    std::unordered_map<AssetHandle, GLMaterialData> m_materialCache;
};
}
