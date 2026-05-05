#pragma once

#include <glad/gl.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include <stb_image_write.h>
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
};

struct GLMaterialData
{
    Vec3 albedo = Vec3(0.8f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    Vec3 emissive = Vec3(0.0f);
    float emissiveStrength = 1.0f;
    GLuint albedoTexture = 0;
    GLuint normalTexture = 0;
    bool hasAlbedoTexture = false;
    bool hasNormalTexture = false;
};

struct UniformLocations
{
    GLint viewProjection = -1;
    GLint model = -1;
    GLint cameraPos = -1;
    GLint lightDirection = -1;
    GLint lightColor = -1;
    GLint lightIntensity = -1;
    GLint ambientColor = -1;
    GLint albedo = -1;
    GLint metallic = -1;
    GLint roughness = -1;
    GLint ao = -1;
    GLint emissive = -1;
    GLint emissiveStrength = -1;
    GLint useAlbedoTexture = -1;
    GLint useNormalTexture = -1;
    GLint albedoTexture = -1;
    GLint normalTexture = -1;
    GLint boneMatrices = -1;
    GLint pointLightCount = -1;
    GLint pointLightPositions = -1;
    GLint pointLightColors = -1;
    GLint pointLightRanges = -1;
    GLint pointLightIntensities = -1;
    GLint lightSpaceMatrix = -1;
    GLint shadowMap = -1;
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
        CreateShadowShader();
        CreatePostProcessShader();
        CacheUniformLocations();
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
            if (mat.normalTexture) glDeleteTextures(1, &mat.normalTexture);
        }
        m_materialCache.clear();

        if (m_pbrShader.program) { glDeleteProgram(m_pbrShader.program); m_pbrShader.program = 0; }
        if (m_skinnedShader.program) { glDeleteProgram(m_skinnedShader.program); m_skinnedShader.program = 0; }
        if (m_shadowShader.program) { glDeleteProgram(m_shadowShader.program); m_shadowShader.program = 0; }
        if (m_postProcessShader.program) { glDeleteProgram(m_postProcessShader.program); m_postProcessShader.program = 0; }
        if (m_defaultAlbedoTexture) { glDeleteTextures(1, &m_defaultAlbedoTexture); m_defaultAlbedoTexture = 0; }
        if (m_defaultNormalTexture) { glDeleteTextures(1, &m_defaultNormalTexture); m_defaultNormalTexture = 0; }

        DestroyFramebuffer();
        DestroyShadowMap();

        KE_LOG_INFO("OpenGL Renderer shutdown");
    }

    void OnResize(int width, int height) override
    {
        m_windowWidth = width;
        m_windowHeight = height;
        glViewport(0, 0, width, height);
        CreateFramebuffer(width, height);
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
        m_drawCallCount = 0;

        RenderShadowMap(scene);

        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glViewport(0, 0, m_windowWidth, m_windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderStaticMeshes(scene);
        RenderSkinnedMeshes(scene);
        RenderTerrain(scene);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        RenderPostProcess();

        RenderDebugOverlay();
    }

    uint32_t GetDrawCallCount() const { return m_drawCallCount; }

    AssetHandle UploadMesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
        int vertexStride = 8) override
    {
        AssetHandle handle = GenerateHandle();

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
        const std::vector<uint32_t>& indices) override
    {
        AssetHandle handle = GenerateHandle();

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

    AssetHandle UploadTexture(int width, int height, int channels, const uint8_t* data) override
    {
        AssetHandle handle = GenerateHandle();

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

    AssetHandle RegisterMaterial(const Vec3& albedo, float metallic, float roughness,
        AssetHandle albedoTexHandle = INVALID_HANDLE, bool hasTex = false,
        AssetHandle normalTexHandle = INVALID_HANDLE, bool hasNormalTex = false,
        const Vec3& emissiveColor = Vec3(0.0f), float emissiveStr = 1.0f) override
    {
        AssetHandle handle = GenerateHandle();
        GLMaterialData mat;
        mat.albedo = albedo;
        mat.metallic = metallic;
        mat.roughness = roughness;
        mat.hasAlbedoTexture = hasTex;
        mat.hasNormalTexture = hasNormalTex;
        mat.emissive = emissiveColor;
        mat.emissiveStrength = emissiveStr;
        if (hasTex && albedoTexHandle != INVALID_HANDLE)
        {
            auto it = m_textureCache.find(albedoTexHandle);
            if (it != m_textureCache.end())
                mat.albedoTexture = it->second.texture;
        }
        if (hasNormalTex && normalTexHandle != INVALID_HANDLE)
        {
            auto it = m_textureCache.find(normalTexHandle);
            if (it != m_textureCache.end())
                mat.normalTexture = it->second.texture;
        }
        m_materialCache[handle] = mat;
        return handle;
    }

    void UploadBoneMatrices(AssetHandle handle, const std::vector<Mat4>& matrices) override
    {
        m_boneMatricesCache[handle] = matrices;
    }

    void RemoveMesh(AssetHandle handle) override
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

    void RemoveTexture(AssetHandle handle) override
    {
        auto it = m_textureCache.find(handle);
        if (it != m_textureCache.end())
        {
            glDeleteTextures(1, &it->second.texture);
            m_textureCache.erase(it);
        }
    }

    bool SaveScreenshot(const std::string& path, int width, int height) override
    {
        if (width <= 0 || height <= 0) return false;

        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 3);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        stbi_flip_vertically_on_write(1);
        int res = stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3);
        return res != 0;
    }

    std::string GetName() const override { return "OpenGL 4.5"; }

    AssetHandle GetDefaultCubeHandle() const override { return m_defaultCubeHandle; }
    AssetHandle GetDefaultPlaneHandle() const override { return m_defaultPlaneHandle; }
    AssetHandle GetDefaultAlbedoTextureHandle() const override { return m_defaultAlbedoTextureHandle; }

private:
    void RenderStaticMeshes(const RenderScene& scene)
    {
        if (!m_pbrShader.program) return;
        glUseProgram(m_pbrShader.program);

        SetPBRUniforms(scene, m_pbrUniforms);

        if (m_pbrUniforms.shadowMap >= 0) glUniform1i(m_pbrUniforms.shadowMap, 2);
        if (m_pbrUniforms.lightSpaceMatrix >= 0 && m_shadowShader.program)
            glUniformMatrix4fv(m_pbrUniforms.lightSpaceMatrix, 1, GL_FALSE, &m_lightSpaceMatrix[0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);

        for (const auto& cmd : scene.staticDrawCommands)
        {
            if (m_pbrUniforms.model >= 0)
                glUniformMatrix4fv(m_pbrUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            SetMaterialUniforms(cmd.materialHandle, m_pbrUniforms);

            GLMeshData mesh = m_defaultMesh;
            auto it = m_meshCache.find(cmd.meshHandle);
            if (it != m_meshCache.end()) mesh = it->second;

            if (mesh.vao)
            {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        if (m_pbrUniforms.useNormalTexture >= 0)
            glUniform1i(m_pbrUniforms.useNormalTexture, 0);
        if (m_pbrUniforms.useAlbedoTexture >= 0)
            glUniform1i(m_pbrUniforms.useAlbedoTexture, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void RenderSkinnedMeshes(const RenderScene& scene)
    {
        if (!m_skinnedShader.program || scene.skinnedDrawCommands.empty()) return;
        glUseProgram(m_skinnedShader.program);

        SetSkinnedUniforms(scene);

        if (m_skinnedUniforms.shadowMap >= 0) glUniform1i(m_skinnedUniforms.shadowMap, 2);
        if (m_skinnedUniforms.lightSpaceMatrix >= 0 && m_shadowShader.program)
            glUniformMatrix4fv(m_skinnedUniforms.lightSpaceMatrix, 1, GL_FALSE, &m_lightSpaceMatrix[0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);

        for (const auto& cmd : scene.skinnedDrawCommands)
        {
            if (m_skinnedUniforms.model >= 0)
                glUniformMatrix4fv(m_skinnedUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            GLint boneLoc = m_skinnedUniforms.boneMatrices;
            auto it = m_boneMatricesCache.find(cmd.boneMatricesHandle);
            if (boneLoc >= 0 && it != m_boneMatricesCache.end())
            {
                glUniformMatrix4fv(boneLoc, static_cast<GLsizei>(it->second.size()),
                    GL_FALSE, &it->second[0][0][0]);
            }

            SetSkinnedMaterialUniforms(cmd.materialHandle);

            GLMeshData mesh;
            auto meshIt = m_meshCache.find(cmd.skeletalMeshHandle);
            if (meshIt != m_meshCache.end()) mesh = meshIt->second;

            if (mesh.vao)
            {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void RenderTerrain(const RenderScene& scene)
    {
        if (!m_pbrShader.program || scene.terrainDrawCommands.empty()) return;
        glUseProgram(m_pbrShader.program);

        SetPBRUniforms(scene, m_pbrUniforms);

        if (m_pbrUniforms.shadowMap >= 0) glUniform1i(m_pbrUniforms.shadowMap, 2);
        if (m_pbrUniforms.lightSpaceMatrix >= 0 && m_shadowShader.program)
            glUniformMatrix4fv(m_pbrUniforms.lightSpaceMatrix, 1, GL_FALSE, &m_lightSpaceMatrix[0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);

        for (const auto& cmd : scene.terrainDrawCommands)
        {
            if (m_pbrUniforms.model >= 0)
                glUniformMatrix4fv(m_pbrUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            SetMaterialUniforms(cmd.materialHandle, m_pbrUniforms);

            auto it = m_meshCache.find(cmd.terrainHandle);
            if (it != m_meshCache.end() && it->second.vao)
            {
                glBindVertexArray(it->second.vao);
                glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void RenderShadowMap(const RenderScene& scene)
    {
        if (!m_shadowShader.program) return;

        if (m_shadowMapFBO == 0)
            CreateShadowMap(2048, 2048);

        float nearP = 0.1f;
        float farP = 50.0f;
        Mat4 lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearP, farP);
        Mat4 lightView = glm::lookAt(-scene.light.direction * 25.0f,
            Vec3(0.0f), Vec3(0.0f, 1.0f, 0.0f));
        m_lightSpaceMatrix = lightProj * lightView;

        glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_shadowShader.program);
        GLint lightSpaceLoc = glGetUniformLocation(m_shadowShader.program, "uLightSpaceMatrix");
        if (lightSpaceLoc >= 0)
            glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, &m_lightSpaceMatrix[0][0]);

        GLint modelLoc = glGetUniformLocation(m_shadowShader.program, "uModel");

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

        for (const auto& cmd : scene.terrainDrawCommands)
        {
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            auto it = m_meshCache.find(cmd.terrainHandle);
            if (it != m_meshCache.end() && it->second.vao)
            {
                glBindVertexArray(it->second.vao);
                glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        if (m_skinnedShader.program)
        {
            GLint skinnedModelLoc = glGetUniformLocation(m_shadowShader.program, "uModel");
            GLint skinnedBoneLoc = glGetUniformLocation(m_shadowShader.program, "uBoneMatrices[0]");
            for (const auto& cmd : scene.skinnedDrawCommands)
            {
                if (skinnedModelLoc >= 0)
                    glUniformMatrix4fv(skinnedModelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

                auto boneIt = m_boneMatricesCache.find(cmd.boneMatricesHandle);
                if (skinnedBoneLoc >= 0 && boneIt != m_boneMatricesCache.end())
                {
                    glUniformMatrix4fv(skinnedBoneLoc, static_cast<GLsizei>(boneIt->second.size()),
                        GL_FALSE, &boneIt->second[0][0][0]);
                }

                auto meshIt = m_meshCache.find(cmd.skeletalMeshHandle);
                if (meshIt != m_meshCache.end() && meshIt->second.vao)
                {
                    glBindVertexArray(meshIt->second.vao);
                    glDrawElements(GL_TRIANGLES, meshIt->second.indexCount, GL_UNSIGNED_INT, nullptr);
                    glBindVertexArray(0);
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_windowWidth, m_windowHeight);
        glUseProgram(0);
    }

    void RenderPostProcess()
    {
        if (!m_postProcessShader.program) return;

        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(m_postProcessShader.program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(m_postProcessShader.program, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);

        GLint exposureLoc = glGetUniformLocation(m_postProcessShader.program, "uExposure");
        if (exposureLoc >= 0) glUniform1f(exposureLoc, 1.0f);

        if (m_screenQuadVAO == 0)
        {
            float quad[] = {
                -1.0f,  1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f,
                 1.0f, -1.0f, 1.0f, 0.0f,
                -1.0f,  1.0f, 0.0f, 1.0f,
                 1.0f, -1.0f, 1.0f, 0.0f,
                 1.0f,  1.0f, 1.0f, 1.0f,
            };
            glGenVertexArrays(1, &m_screenQuadVAO);
            glGenBuffers(1, &m_screenQuadVBO);
            glBindVertexArray(m_screenQuadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                reinterpret_cast<void*>(2 * sizeof(float)));
            glBindVertexArray(0);
        }

        glBindVertexArray(m_screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glUseProgram(0);
    }

    void RenderDebugOverlay()
    {
    }

    void SetPBRUniforms(const RenderScene& scene, const UniformLocations& locs)
    {
        if (locs.viewProjection >= 0)
            glUniformMatrix4fv(locs.viewProjection, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (locs.cameraPos >= 0)
            glUniform3fv(locs.cameraPos, 1, &scene.camera.position[0]);
        if (locs.lightDirection >= 0)
            glUniform3fv(locs.lightDirection, 1, &scene.light.direction[0]);
        if (locs.lightColor >= 0)
            glUniform3fv(locs.lightColor, 1, &scene.light.color[0]);
        if (locs.lightIntensity >= 0)
            glUniform1f(locs.lightIntensity, scene.light.intensity);
        if (locs.ambientColor >= 0)
            glUniform3fv(locs.ambientColor, 1, &scene.light.ambientColor[0]);

        int plCount = std::min(scene.light.pointLightCount,
            static_cast<int>(LightData::MAX_POINT_LIGHTS));
        if (locs.pointLightCount >= 0)
            glUniform1i(locs.pointLightCount, plCount);
        if (plCount > 0)
        {
            float positions[12] = {};
            float colors[12] = {};
            float ranges[4] = {};
            float intensities[4] = {};
            for (int i = 0; i < plCount; ++i)
            {
                positions[i * 3]     = scene.light.pointLights[i].position.x;
                positions[i * 3 + 1] = scene.light.pointLights[i].position.y;
                positions[i * 3 + 2] = scene.light.pointLights[i].position.z;
                colors[i * 3]        = scene.light.pointLights[i].color.x;
                colors[i * 3 + 1]    = scene.light.pointLights[i].color.y;
                colors[i * 3 + 2]    = scene.light.pointLights[i].color.z;
                ranges[i]            = scene.light.pointLights[i].range;
                intensities[i]       = scene.light.pointLights[i].intensity;
            }
            if (locs.pointLightPositions >= 0)
                glUniform3fv(locs.pointLightPositions, plCount, positions);
            if (locs.pointLightColors >= 0)
                glUniform3fv(locs.pointLightColors, plCount, colors);
            if (locs.pointLightRanges >= 0)
                glUniform1fv(locs.pointLightRanges, plCount, ranges);
            if (locs.pointLightIntensities >= 0)
                glUniform1fv(locs.pointLightIntensities, plCount, intensities);
        }
    }

    void SetSkinnedUniforms(const RenderScene& scene)
    {
        if (m_skinnedUniforms.viewProjection >= 0)
            glUniformMatrix4fv(m_skinnedUniforms.viewProjection, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (m_skinnedUniforms.cameraPos >= 0)
            glUniform3fv(m_skinnedUniforms.cameraPos, 1, &scene.camera.position[0]);
        if (m_skinnedUniforms.lightDirection >= 0)
            glUniform3fv(m_skinnedUniforms.lightDirection, 1, &scene.light.direction[0]);
        if (m_skinnedUniforms.lightColor >= 0)
            glUniform3fv(m_skinnedUniforms.lightColor, 1, &scene.light.color[0]);
        if (m_skinnedUniforms.lightIntensity >= 0)
            glUniform1f(m_skinnedUniforms.lightIntensity, scene.light.intensity);
        if (m_skinnedUniforms.ambientColor >= 0)
            glUniform3fv(m_skinnedUniforms.ambientColor, 1, &scene.light.ambientColor[0]);
    }

    void SetSkinnedMaterialUniforms(AssetHandle materialHandle)
    {
        if (materialHandle != INVALID_HANDLE)
        {
            auto matIt = m_materialCache.find(materialHandle);
            if (matIt != m_materialCache.end())
            {
                const auto& mat = matIt->second;
                if (m_skinnedUniforms.albedo >= 0) glUniform3fv(m_skinnedUniforms.albedo, 1, &mat.albedo[0]);
                if (m_skinnedUniforms.metallic >= 0) glUniform1f(m_skinnedUniforms.metallic, mat.metallic);
                if (m_skinnedUniforms.roughness >= 0) glUniform1f(m_skinnedUniforms.roughness, mat.roughness);
                if (m_skinnedUniforms.ao >= 0) glUniform1f(m_skinnedUniforms.ao, mat.ao);
                if (m_skinnedUniforms.emissive >= 0) glUniform3fv(m_skinnedUniforms.emissive, 1, &mat.emissive[0]);
                if (m_skinnedUniforms.emissiveStrength >= 0) glUniform1f(m_skinnedUniforms.emissiveStrength, mat.emissiveStrength);
                if (m_skinnedUniforms.useAlbedoTexture >= 0) glUniform1i(m_skinnedUniforms.useAlbedoTexture, mat.hasAlbedoTexture ? 1 : 0);
                if (mat.hasAlbedoTexture && mat.albedoTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
                    if (m_skinnedUniforms.albedoTexture >= 0) glUniform1i(m_skinnedUniforms.albedoTexture, 0);
                }
                return;
            }
        }

        if (m_skinnedUniforms.albedo >= 0) glUniform3f(m_skinnedUniforms.albedo, 0.8f, 0.6f, 0.4f);
        if (m_skinnedUniforms.metallic >= 0) glUniform1f(m_skinnedUniforms.metallic, 0.0f);
        if (m_skinnedUniforms.roughness >= 0) glUniform1f(m_skinnedUniforms.roughness, 0.5f);
        if (m_skinnedUniforms.ao >= 0) glUniform1f(m_skinnedUniforms.ao, 1.0f);
        if (m_skinnedUniforms.emissive >= 0) glUniform3f(m_skinnedUniforms.emissive, 0.0f, 0.0f, 0.0f);
        if (m_skinnedUniforms.emissiveStrength >= 0) glUniform1f(m_skinnedUniforms.emissiveStrength, 0.0f);
        if (m_skinnedUniforms.useAlbedoTexture >= 0) glUniform1i(m_skinnedUniforms.useAlbedoTexture, 0);
    }

    void SetMaterialUniforms(AssetHandle materialHandle, const UniformLocations& locs)
    {
        if (materialHandle != INVALID_HANDLE)
        {
            auto matIt = m_materialCache.find(materialHandle);
            if (matIt != m_materialCache.end())
            {
                const auto& mat = matIt->second;
                if (locs.albedo >= 0) glUniform3fv(locs.albedo, 1, &mat.albedo[0]);
                if (locs.metallic >= 0) glUniform1f(locs.metallic, mat.metallic);
                if (locs.roughness >= 0) glUniform1f(locs.roughness, mat.roughness);
                if (locs.ao >= 0) glUniform1f(locs.ao, mat.ao);
                if (locs.emissive >= 0) glUniform3fv(locs.emissive, 1, &mat.emissive[0]);
                if (locs.emissiveStrength >= 0) glUniform1f(locs.emissiveStrength, mat.emissiveStrength);
                if (locs.useAlbedoTexture >= 0) glUniform1i(locs.useAlbedoTexture, mat.hasAlbedoTexture ? 1 : 0);
                if (locs.useNormalTexture >= 0) glUniform1i(locs.useNormalTexture, mat.hasNormalTexture ? 1 : 0);

                if (mat.hasAlbedoTexture && mat.albedoTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
                    if (locs.albedoTexture >= 0) glUniform1i(locs.albedoTexture, 0);
                }
                if (mat.hasNormalTexture && mat.normalTexture)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mat.normalTexture);
                    if (locs.normalTexture >= 0) glUniform1i(locs.normalTexture, 1);
                }
                return;
            }
        }

        if (locs.albedo >= 0) glUniform3f(locs.albedo, 0.8f, 0.8f, 0.8f);
        if (locs.metallic >= 0) glUniform1f(locs.metallic, 0.0f);
        if (locs.roughness >= 0) glUniform1f(locs.roughness, 0.5f);
        if (locs.ao >= 0) glUniform1f(locs.ao, 1.0f);
        if (locs.emissive >= 0) glUniform3f(locs.emissive, 0.0f, 0.0f, 0.0f);
        if (locs.emissiveStrength >= 0) glUniform1f(locs.emissiveStrength, 0.0f);
        if (locs.useAlbedoTexture >= 0) glUniform1i(locs.useAlbedoTexture, 0);
        if (locs.useNormalTexture >= 0) glUniform1i(locs.useNormalTexture, 0);
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
            uniform vec3 uEmissive;
            uniform float uEmissiveStrength;
            uniform bool uUseAlbedoTexture;
            uniform bool uUseNormalTexture;
            uniform sampler2D uAlbedoTexture;
            uniform sampler2D uNormalTexture;

            uniform mat4 uLightSpaceMatrix;
            uniform sampler2D uShadowMap;

            uniform int uPointLightCount;
            uniform vec3 uPointLightPositions[4];
            uniform vec3 uPointLightColors[4];
            uniform float uPointLightRanges[4];
            uniform float uPointLightIntensities[4];

            out vec4 FragColor;

            const float PI = 3.14159265359;

            float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
            {
                vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
                projCoords = projCoords * 0.5 + 0.5;
                if (projCoords.z > 1.0) return 0.0;
                float closestDepth = texture(uShadowMap, projCoords.xy).r;
                float currentDepth = projCoords.z;
                float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
                float shadow = 0.0;
                vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
                for (int x = -1; x <= 1; ++x)
                {
                    for (int y = -1; y <= 1; ++y)
                    {
                        float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                    }
                }
                shadow /= 9.0;
                return shadow;
            }

            vec3 GetNormal()
            {
                vec3 N = normalize(vNormal);
                if (uUseNormalTexture)
                {
                    vec3 tangentNormal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
                    vec3 Q1 = dFdx(vWorldPos);
                    vec3 Q2 = dFdy(vWorldPos);
                    vec2 st1 = dFdx(vTexCoord);
                    vec2 st2 = dFdy(vTexCoord);
                    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
                    vec3 B = normalize(cross(N, T));
                    if (dot(cross(N, T), B) < 0.0) T = -T;
                    mat3 TBN = mat3(T, B, N);
                    N = normalize(TBN * tangentNormal);
                }
                return N;
            }

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

            vec3 CalcLighting(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 albedo, float shadow)
            {
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
                return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
            }

            void main()
            {
                vec3 albedo = uUseAlbedoTexture ? texture(uAlbedoTexture, vTexCoord).rgb : uAlbedo;

                vec3 N = GetNormal();
                vec3 V = normalize(uCameraPos - vWorldPos);

                vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(vWorldPos, 1.0);
                float shadow = ShadowCalculation(fragPosLightSpace, N, normalize(-uLightDirection));

                vec3 L = normalize(-uLightDirection);
                vec3 radiance = uLightColor * uLightIntensity;
                vec3 Lo = CalcLighting(L, radiance, N, V, albedo, shadow);

                for (int i = 0; i < uPointLightCount && i < 4; ++i)
                {
                    vec3 lightVec = uPointLightPositions[i] - vWorldPos;
                    float dist = length(lightVec);
                    if (dist > uPointLightRanges[i]) continue;
                    L = lightVec / dist;
                    float attenuation = 1.0 - (dist / uPointLightRanges[i]);
                    attenuation = attenuation * attenuation;
                    radiance = uPointLightColors[i] * uPointLightIntensities[i] * attenuation;
                    Lo += CalcLighting(L, radiance, N, V, albedo, 0.0);
                }

                vec3 ambient = uAmbientColor * albedo * uAO;
                vec3 emissiveContrib = uEmissive * uEmissiveStrength;
                vec3 color = ambient + Lo + emissiveContrib;

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
            uniform vec3 uAmbientColor;

            uniform vec3 uAlbedo;
            uniform float uMetallic;
            uniform float uRoughness;
            uniform float uAO;
            uniform vec3 uEmissive;
            uniform float uEmissiveStrength;
            uniform bool uUseAlbedoTexture;
            uniform sampler2D uAlbedoTexture;

            uniform mat4 uLightSpaceMatrix;
            uniform sampler2D uShadowMap;

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

            float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
            {
                vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
                projCoords = projCoords * 0.5 + 0.5;
                if (projCoords.z > 1.0) return 0.0;
                float closestDepth = texture(uShadowMap, projCoords.xy).r;
                float currentDepth = projCoords.z;
                float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
                float shadow = 0.0;
                vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
                for (int x = -1; x <= 1; ++x)
                {
                    for (int y = -1; y <= 1; ++y)
                    {
                        float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                    }
                }
                shadow /= 9.0;
                return shadow;
            }

            void main()
            {
                vec3 albedo = uUseAlbedoTexture ? texture(uAlbedoTexture, vTexCoord).rgb : uAlbedo;

                vec3 N = normalize(vNormal);
                vec3 V = normalize(uCameraPos - vWorldPos);

                vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(vWorldPos, 1.0);
                float shadow = ShadowCalculation(fragPosLightSpace, N, normalize(-uLightDirection));

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
                vec3 Lo = (1.0 - shadow) * (kD * albedo / PI + specular) * uLightColor * uLightIntensity * NdotL;

                vec3 ambient = uAmbientColor * albedo * uAO;
                vec3 emissiveContrib = uEmissive * uEmissiveStrength;
                vec3 color = ambient + Lo + emissiveContrib;

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

        m_defaultAlbedoTextureHandle = UploadTexture(2, 2, 4, white4x4);

        uint8_t normalDefault[] = { 128,128,255,255, 128,128,255,255,
                                    128,128,255,255, 128,128,255,255 };
        glGenTextures(1, &m_defaultNormalTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultNormalTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, normalDefault);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void CacheUniformLocations()
    {
        auto cache = [](GLuint program, const char* name) -> GLint
        {
            GLint loc = glGetUniformLocation(program, name);
            return loc;
        };

        if (m_pbrShader.program)
        {
            m_pbrUniforms.viewProjection = cache(m_pbrShader.program, "uViewProjection");
            m_pbrUniforms.model = cache(m_pbrShader.program, "uModel");
            m_pbrUniforms.cameraPos = cache(m_pbrShader.program, "uCameraPos");
            m_pbrUniforms.lightDirection = cache(m_pbrShader.program, "uLightDirection");
            m_pbrUniforms.lightColor = cache(m_pbrShader.program, "uLightColor");
            m_pbrUniforms.lightIntensity = cache(m_pbrShader.program, "uLightIntensity");
            m_pbrUniforms.ambientColor = cache(m_pbrShader.program, "uAmbientColor");
            m_pbrUniforms.albedo = cache(m_pbrShader.program, "uAlbedo");
            m_pbrUniforms.metallic = cache(m_pbrShader.program, "uMetallic");
            m_pbrUniforms.roughness = cache(m_pbrShader.program, "uRoughness");
            m_pbrUniforms.ao = cache(m_pbrShader.program, "uAO");
            m_pbrUniforms.emissive = cache(m_pbrShader.program, "uEmissive");
            m_pbrUniforms.emissiveStrength = cache(m_pbrShader.program, "uEmissiveStrength");
            m_pbrUniforms.useAlbedoTexture = cache(m_pbrShader.program, "uUseAlbedoTexture");
            m_pbrUniforms.useNormalTexture = cache(m_pbrShader.program, "uUseNormalTexture");
            m_pbrUniforms.albedoTexture = cache(m_pbrShader.program, "uAlbedoTexture");
            m_pbrUniforms.normalTexture = cache(m_pbrShader.program, "uNormalTexture");
            m_pbrUniforms.pointLightCount = cache(m_pbrShader.program, "uPointLightCount");
            m_pbrUniforms.pointLightPositions = cache(m_pbrShader.program, "uPointLightPositions[0]");
            m_pbrUniforms.pointLightColors = cache(m_pbrShader.program, "uPointLightColors[0]");
            m_pbrUniforms.pointLightRanges = cache(m_pbrShader.program, "uPointLightRanges[0]");
            m_pbrUniforms.pointLightIntensities = cache(m_pbrShader.program, "uPointLightIntensities[0]");
            m_pbrUniforms.lightSpaceMatrix = cache(m_pbrShader.program, "uLightSpaceMatrix");
            m_pbrUniforms.shadowMap = cache(m_pbrShader.program, "uShadowMap");
        }

        if (m_skinnedShader.program)
        {
            m_skinnedUniforms.viewProjection = cache(m_skinnedShader.program, "uViewProjection");
            m_skinnedUniforms.model = cache(m_skinnedShader.program, "uModel");
            m_skinnedUniforms.cameraPos = cache(m_skinnedShader.program, "uCameraPos");
            m_skinnedUniforms.lightDirection = cache(m_skinnedShader.program, "uLightDirection");
            m_skinnedUniforms.lightColor = cache(m_skinnedShader.program, "uLightColor");
            m_skinnedUniforms.lightIntensity = cache(m_skinnedShader.program, "uLightIntensity");
            m_skinnedUniforms.ambientColor = cache(m_skinnedShader.program, "uAmbientColor");
            m_skinnedUniforms.albedo = cache(m_skinnedShader.program, "uAlbedo");
            m_skinnedUniforms.metallic = cache(m_skinnedShader.program, "uMetallic");
            m_skinnedUniforms.roughness = cache(m_skinnedShader.program, "uRoughness");
            m_skinnedUniforms.ao = cache(m_skinnedShader.program, "uAO");
            m_skinnedUniforms.emissive = cache(m_skinnedShader.program, "uEmissive");
            m_skinnedUniforms.emissiveStrength = cache(m_skinnedShader.program, "uEmissiveStrength");
            m_skinnedUniforms.useAlbedoTexture = cache(m_skinnedShader.program, "uUseAlbedoTexture");
            m_skinnedUniforms.albedoTexture = cache(m_skinnedShader.program, "uAlbedoTexture");
            m_skinnedUniforms.boneMatrices = cache(m_skinnedShader.program, "uBoneMatrices[0]");
            m_skinnedUniforms.lightSpaceMatrix = cache(m_skinnedShader.program, "uLightSpaceMatrix");
            m_skinnedUniforms.shadowMap = cache(m_skinnedShader.program, "uShadowMap");
        }
    }

    void CreateShadowShader()
    {
        const char* vertexSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 3) in vec4 aBoneWeights;
            layout(location = 4) in vec4 aBoneIndices;

            uniform mat4 uLightSpaceMatrix;
            uniform mat4 uModel;
            uniform mat4 uBoneMatrices[128];

            void main()
            {
                vec4 skinnedPos = vec4(aPosition, 1.0);
                if (aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w > 0.0)
                {
                    ivec4 boneIdx = ivec4(aBoneIndices);
                    mat4 skinMatrix =
                        aBoneWeights.x * uBoneMatrices[boneIdx.x] +
                        aBoneWeights.y * uBoneMatrices[boneIdx.y] +
                        aBoneWeights.z * uBoneMatrices[boneIdx.z] +
                        aBoneWeights.w * uBoneMatrices[boneIdx.w];
                    skinnedPos = skinMatrix * vec4(aPosition, 1.0);
                }
                gl_Position = uLightSpaceMatrix * uModel * skinnedPos;
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            void main()
            {
            }
        )";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_shadowShader.program = LinkProgram(vs, fs);
    }

    void CreatePostProcessShader()
    {
        const char* vertexSrc = R"(
            #version 450 core
            layout(location = 0) in vec2 aPosition;
            layout(location = 1) in vec2 aTexCoord;
            out vec2 vTexCoord;
            void main()
            {
                vTexCoord = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec4 FragColor;

            uniform sampler2D uSceneTexture;
            uniform float uExposure;

            void main()
            {
                vec3 color = texture(uSceneTexture, vTexCoord).rgb;

                color = vec3(1.0) - exp(-color * uExposure);

                color = pow(color, vec3(1.0 / 2.2));

                FragColor = vec4(color, 1.0);
            }
        )";

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_postProcessShader.program = LinkProgram(vs, fs);
    }

    void CreateFramebuffer(int width, int height)
    {
        DestroyFramebuffer();

        if (width <= 0 || height <= 0) return;

        glGenFramebuffers(1, &m_sceneFBO);
        glGenTextures(1, &m_sceneColorTexture);
        glGenRenderbuffers(1, &m_sceneDepthRBO);

        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindRenderbuffer(GL_RENDERBUFFER, m_sceneDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneColorTexture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sceneDepthRBO);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            KE_LOG_ERROR("Framebuffer incomplete: {}", static_cast<int>(status));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        KE_LOG_INFO("Created scene framebuffer {}x{}", width, height);
    }

    void DestroyFramebuffer()
    {
        if (m_sceneFBO) { glDeleteFramebuffers(1, &m_sceneFBO); m_sceneFBO = 0; }
        if (m_sceneColorTexture) { glDeleteTextures(1, &m_sceneColorTexture); m_sceneColorTexture = 0; }
        if (m_sceneDepthRBO) { glDeleteRenderbuffers(1, &m_sceneDepthRBO); m_sceneDepthRBO = 0; }
    }

    void CreateShadowMap(int width, int height)
    {
        DestroyShadowMap();

        m_shadowMapSize = width;

        glGenFramebuffers(1, &m_shadowMapFBO);
        glGenTextures(1, &m_shadowMapTexture);

        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DestroyShadowMap()
    {
        if (m_shadowMapFBO) { glDeleteFramebuffers(1, &m_shadowMapFBO); m_shadowMapFBO = 0; }
        if (m_shadowMapTexture) { glDeleteTextures(1, &m_shadowMapTexture); m_shadowMapTexture = 0; }
    }

public:
    std::unordered_map<AssetHandle, std::vector<Mat4>> m_boneMatricesCache;

private:
    AssetHandle GenerateHandle()
    {
        return ++m_nextHandle;
    }

    GLShaderProgram m_pbrShader{};
    GLShaderProgram m_skinnedShader{};
    GLShaderProgram m_shadowShader{};
    GLShaderProgram m_postProcessShader{};
    UniformLocations m_pbrUniforms{};
    UniformLocations m_skinnedUniforms{};
    GLMeshData m_defaultMesh{};
    AssetHandle m_defaultCubeHandle = INVALID_HANDLE;
    AssetHandle m_defaultPlaneHandle = INVALID_HANDLE;
    GLuint m_defaultAlbedoTexture = 0;
    GLuint m_defaultNormalTexture = 0;
    AssetHandle m_defaultAlbedoTextureHandle = INVALID_HANDLE;
    std::unordered_map<AssetHandle, GLMeshData> m_meshCache;
    std::unordered_map<AssetHandle, GLTextureData> m_textureCache;
    std::unordered_map<AssetHandle, GLMaterialData> m_materialCache;
    AssetHandle m_nextHandle = 0;
    uint32_t m_drawCallCount = 0;

    GLuint m_sceneFBO = 0;
    GLuint m_sceneColorTexture = 0;
    GLuint m_sceneDepthRBO = 0;
    GLuint m_shadowMapFBO = 0;
    GLuint m_shadowMapTexture = 0;
    int m_shadowMapSize = 2048;
    GLuint m_screenQuadVAO = 0;
    GLuint m_screenQuadVBO = 0;
    Mat4 m_lightSpaceMatrix = Mat4(1.0f);
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
};
}
