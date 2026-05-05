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
#include <algorithm>
#include <array>
#include <string>
#include <limits>

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
    GLuint metallicRoughnessTexture = 0;
    bool hasAlbedoTexture = false;
    bool hasNormalTexture = false;
    bool hasMetallicRoughnessTexture = false;
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
    GLint useMetallicRoughnessTexture = -1;
    GLint albedoTexture = -1;
    GLint normalTexture = -1;
    GLint metallicRoughnessTexture = -1;
    GLint boneMatrices = -1;
    GLint pointLightCount = -1;
    GLint pointLightPositions = -1;
    GLint pointLightColors = -1;
    GLint pointLightRanges = -1;
    GLint pointLightIntensities = -1;
    GLint lightSpaceMatrix = -1;
    GLint shadowMap = -1;
    GLint normalMatrix = -1;
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
        CreateBloomShaders();
        CacheUniformLocations();
        CreateDefaultMesh();
        CreateDefaultTextures();

        int fbWidth = 0, fbHeight = 0;
        GLFWwindow* win = static_cast<GLFWwindow*>(windowHandle);
        glfwGetFramebufferSize(win, &fbWidth, &fbHeight);
        if (fbWidth > 0 && fbHeight > 0)
        {
            m_windowWidth = fbWidth;
            m_windowHeight = fbHeight;
            glViewport(0, 0, fbWidth, fbHeight);
            CreateFramebuffer(fbWidth, fbHeight);
            CreateBloomFBOs(fbWidth, fbHeight);
        }

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
            if (mat.metallicRoughnessTexture) glDeleteTextures(1, &mat.metallicRoughnessTexture);
        }
        m_materialCache.clear();

        if (m_pbrShader.program) { glDeleteProgram(m_pbrShader.program); m_pbrShader.program = 0; }
        if (m_skinnedShader.program) { glDeleteProgram(m_skinnedShader.program); m_skinnedShader.program = 0; }
        if (m_shadowShader.program) { glDeleteProgram(m_shadowShader.program); m_shadowShader.program = 0; }
        if (m_shadowSkinnedShader.program) { glDeleteProgram(m_shadowSkinnedShader.program); m_shadowSkinnedShader.program = 0; }
        if (m_postProcessShader.program) { glDeleteProgram(m_postProcessShader.program); m_postProcessShader.program = 0; }
        if (m_bloomExtractShader.program) { glDeleteProgram(m_bloomExtractShader.program); m_bloomExtractShader.program = 0; }
        if (m_bloomBlurShader.program) { glDeleteProgram(m_bloomBlurShader.program); m_bloomBlurShader.program = 0; }
        if (m_defaultAlbedoTexture) { glDeleteTextures(1, &m_defaultAlbedoTexture); m_defaultAlbedoTexture = 0; }
        if (m_defaultNormalTexture) { glDeleteTextures(1, &m_defaultNormalTexture); m_defaultNormalTexture = 0; }
        if (m_screenQuadVAO) { glDeleteVertexArrays(1, &m_screenQuadVAO); m_screenQuadVAO = 0; }
        if (m_screenQuadVBO) { glDeleteBuffers(1, &m_screenQuadVBO); m_screenQuadVBO = 0; }

        DestroyFramebuffer();
        DestroyBloomFBOs();
        DestroyShadowMap();

        KE_LOG_INFO("OpenGL Renderer shutdown");
    }

    void OnResize(int width, int height) override
    {
        m_windowWidth = width;
        m_windowHeight = height;
        glViewport(0, 0, width, height);
        CreateFramebuffer(width, height);
        CreateBloomFBOs(width, height);
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
        RenderBloom();
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
        if (width <= 0 || height <= 0 || !data) return INVALID_HANDLE;

        AssetHandle handle = GenerateHandle();

        GLTextureData tex{};
        tex.width = width;
        tex.height = height;

        glGenTextures(1, &tex.texture);
        glBindTexture(GL_TEXTURE_2D, tex.texture);

        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB8;
        if (channels == 4)
        {
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
        }
        else if (channels == 1)
        {
            format = GL_RED;
            internalFormat = GL_R8;
        }

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
        const Vec3& emissiveColor = Vec3(0.0f), float emissiveStr = 1.0f,
        AssetHandle metallicRoughnessTexHandle = INVALID_HANDLE, bool hasMetallicRoughnessTex = false,
        float ao = 1.0f) override
    {
        AssetHandle handle = GenerateHandle();
        GLMaterialData mat;
        mat.albedo = albedo;
        mat.metallic = metallic;
        mat.roughness = roughness;
        mat.ao = ao;
        mat.hasAlbedoTexture = hasTex;
        mat.hasNormalTexture = hasNormalTex;
        mat.hasMetallicRoughnessTexture = hasMetallicRoughnessTex;
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
        if (hasMetallicRoughnessTex && metallicRoughnessTexHandle != INVALID_HANDLE)
        {
            auto it = m_textureCache.find(metallicRoughnessTexHandle);
            if (it != m_textureCache.end())
                mat.metallicRoughnessTexture = it->second.texture;
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
        if (path.empty()) return false;

        GLint prevFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glFlush();

        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 3);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

        stbi_flip_vertically_on_write(1);
        int res = stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3);
        return res != 0;
    }

    std::string GetName() const override { return "OpenGL 4.5"; }

    AssetHandle GetDefaultCubeHandle() const override { return m_defaultCubeHandle; }
    AssetHandle GetDefaultPlaneHandle() const override { return m_defaultPlaneHandle; }
    AssetHandle GetDefaultAlbedoTextureHandle() const override { return m_defaultAlbedoTextureHandle; }

private:
    struct FrustumPlane
    {
        Vec3 normal;
        float d;
    };

    std::array<FrustumPlane, 6> ExtractFrustumPlanes(const Mat4& vp) const
    {
        std::array<FrustumPlane, 6> planes;

        auto extract = [&](int idx, const Vec4& a, const Vec4& b)
        {
            Vec4 p = a + b;
            float len = glm::length(Vec3(p));
            if (len > 0.0001f)
            {
                planes[idx].normal = Vec3(p) / len;
                planes[idx].d = p.w / len;
            }
        };

        Vec4 col0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
        Vec4 col1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
        Vec4 col2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
        Vec4 col3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

        extract(0, col3,  col0);
        extract(1, col3, -col0);
        extract(2, col3,  col1);
        extract(3, col3, -col1);
        extract(4, col3,  col2);
        extract(5, col3, -col2);

        return planes;
    }

    bool IsSphereInFrustum(const std::array<FrustumPlane, 6>& planes,
        const Vec3& center, float radius) const
    {
        for (int i = 0; i < 6; ++i)
        {
            float dist = glm::dot(planes[i].normal, center) + planes[i].d;
            if (dist < -radius) return false;
        }
        return true;
    }

    Mat3 ComputeNormalMatrix(const Mat4& modelMatrix) const
    {
        return glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    }

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

        auto frustumPlanes = ExtractFrustumPlanes(scene.camera.viewProjectionMatrix);

        for (const auto& cmd : scene.staticDrawCommands)
        {
            if (m_pbrUniforms.model >= 0)
                glUniformMatrix4fv(m_pbrUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            if (m_pbrUniforms.normalMatrix >= 0)
            {
                Mat3 normalMat = ComputeNormalMatrix(cmd.worldMatrix);
                glUniformMatrix3fv(m_pbrUniforms.normalMatrix, 1, GL_FALSE, &normalMat[0][0]);
            }

            Vec3 worldPos = Vec3(cmd.worldMatrix * Vec4(cmd.boundsCenter, 1.0f));
            float cullRadius = (cmd.boundsRadius > 0.0f) ? cmd.boundsRadius : 100.0f;
            if (!IsSphereInFrustum(frustumPlanes, worldPos, cullRadius)) continue;

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

            if (m_skinnedUniforms.normalMatrix >= 0)
            {
                Mat3 normalMat = ComputeNormalMatrix(cmd.worldMatrix);
                glUniformMatrix3fv(m_skinnedUniforms.normalMatrix, 1, GL_FALSE, &normalMat[0][0]);
            }

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
        glActiveTexture(GL_TEXTURE3);
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

        auto terrainFrustum = ExtractFrustumPlanes(scene.camera.viewProjectionMatrix);

        for (const auto& cmd : scene.terrainDrawCommands)
        {
            if (m_pbrUniforms.model >= 0)
                glUniformMatrix4fv(m_pbrUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            if (m_pbrUniforms.normalMatrix >= 0)
            {
                Mat3 normalMat = ComputeNormalMatrix(cmd.worldMatrix);
                glUniformMatrix3fv(m_pbrUniforms.normalMatrix, 1, GL_FALSE, &normalMat[0][0]);
            }

            Vec3 terrainPos = Vec3(cmd.worldMatrix * Vec4(cmd.boundsCenter, 1.0f));
            float terrainRadius = (cmd.boundsRadius > 0.0f) ? cmd.boundsRadius : 200.0f;
            if (!IsSphereInFrustum(terrainFrustum, terrainPos, terrainRadius)) continue;

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

        Vec3 sceneMin(std::numeric_limits<float>::max());
        Vec3 sceneMax(std::numeric_limits<float>::lowest());

        auto expandBounds = [&](const Mat4& worldMatrix, const Vec3& localCenter, float localRadius)
        {
            if (localRadius <= 0.0f) return;
            for (int dx = -1; dx <= 1; dx += 2)
            {
                for (int dy = -1; dy <= 1; dy += 2)
                {
                    for (int dz = -1; dz <= 1; dz += 2)
                    {
                        Vec3 corner = localCenter + localRadius * Vec3(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
                        Vec3 worldCorner = Vec3(worldMatrix * Vec4(corner, 1.0f));
                        sceneMin = glm::min(sceneMin, worldCorner);
                        sceneMax = glm::max(sceneMax, worldCorner);
                    }
                }
            }
        };

        for (const auto& cmd : scene.staticDrawCommands)
            expandBounds(cmd.worldMatrix, cmd.boundsCenter, cmd.boundsRadius);
        for (const auto& cmd : scene.terrainDrawCommands)
            expandBounds(cmd.worldMatrix, cmd.boundsCenter, cmd.boundsRadius);
        for (const auto& cmd : scene.skinnedDrawCommands)
        {
            Vec3 skinnedCenter = Vec3(cmd.worldMatrix * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
            sceneMin = glm::min(sceneMin, skinnedCenter - Vec3(2.0f));
            sceneMax = glm::max(sceneMax, skinnedCenter + Vec3(2.0f));
        }

        if (scene.staticDrawCommands.empty() && scene.terrainDrawCommands.empty() && scene.skinnedDrawCommands.empty())
        {
            sceneMin = Vec3(-10.0f);
            sceneMax = Vec3(10.0f);
        }

        Vec3 center = (sceneMin + sceneMax) * 0.5f;
        Vec3 extents = sceneMax - sceneMin;
        float maxExtent = std::max({ extents.x, extents.y, extents.z }) * 0.5f;
        float shadowRange = std::max(maxExtent, 10.0f) + 10.0f;

        float nearP = 0.1f;
        float farP = shadowRange * 2.0f;
        Mat4 lightProj = glm::ortho(-shadowRange, shadowRange, -shadowRange, shadowRange, nearP, farP);
        Mat4 lightView = glm::lookAt(-scene.light.direction * shadowRange + center,
            center, Vec3(0.0f, 1.0f, 0.0f));
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

        if (m_shadowSkinnedShader.program)
        {
            glUseProgram(m_shadowSkinnedShader.program);
            GLint skinnedLightSpaceLoc = glGetUniformLocation(m_shadowSkinnedShader.program, "uLightSpaceMatrix");
            if (skinnedLightSpaceLoc >= 0)
                glUniformMatrix4fv(skinnedLightSpaceLoc, 1, GL_FALSE, &m_lightSpaceMatrix[0][0]);

            GLint skinnedModelLoc = glGetUniformLocation(m_shadowSkinnedShader.program, "uModel");
            GLint skinnedBoneLoc = glGetUniformLocation(m_shadowSkinnedShader.program, "uBoneMatrices[0]");
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

        EnsureScreenQuad();

        glUseProgram(m_postProcessShader.program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(m_postProcessShader.program, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);

        bool useBloom = (m_bloomPingPongFBO[0] != 0);
        GLint useBloomLoc = glGetUniformLocation(m_postProcessShader.program, "uUseBloom");
        if (useBloomLoc >= 0) glUniform1i(useBloomLoc, useBloom ? 1 : 0);

        if (useBloom)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_bloomPingPongTexture[0]);
            GLint bloomTexLoc = glGetUniformLocation(m_postProcessShader.program, "uBloomTexture");
            if (bloomTexLoc >= 0) glUniform1i(bloomTexLoc, 1);
        }

        GLint exposureLoc = glGetUniformLocation(m_postProcessShader.program, "uExposure");
        if (exposureLoc >= 0) glUniform1f(exposureLoc, 1.0f);

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

        int plCount = std::min(scene.light.pointLightCount,
            static_cast<int>(LightData::MAX_POINT_LIGHTS));
        if (m_skinnedUniforms.pointLightCount >= 0)
            glUniform1i(m_skinnedUniforms.pointLightCount, plCount);
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
            if (m_skinnedUniforms.pointLightPositions >= 0)
                glUniform3fv(m_skinnedUniforms.pointLightPositions, plCount, positions);
            if (m_skinnedUniforms.pointLightColors >= 0)
                glUniform3fv(m_skinnedUniforms.pointLightColors, plCount, colors);
            if (m_skinnedUniforms.pointLightRanges >= 0)
                glUniform1fv(m_skinnedUniforms.pointLightRanges, plCount, ranges);
            if (m_skinnedUniforms.pointLightIntensities >= 0)
                glUniform1fv(m_skinnedUniforms.pointLightIntensities, plCount, intensities);
        }
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
                if (m_skinnedUniforms.useNormalTexture >= 0) glUniform1i(m_skinnedUniforms.useNormalTexture, mat.hasNormalTexture ? 1 : 0);
                if (m_skinnedUniforms.useMetallicRoughnessTexture >= 0) glUniform1i(m_skinnedUniforms.useMetallicRoughnessTexture, mat.hasMetallicRoughnessTexture ? 1 : 0);
                if (mat.hasAlbedoTexture && mat.albedoTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
                    if (m_skinnedUniforms.albedoTexture >= 0) glUniform1i(m_skinnedUniforms.albedoTexture, 0);
                }
                if (mat.hasNormalTexture && mat.normalTexture)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mat.normalTexture);
                    if (m_skinnedUniforms.normalTexture >= 0) glUniform1i(m_skinnedUniforms.normalTexture, 1);
                }
                if (mat.hasMetallicRoughnessTexture && mat.metallicRoughnessTexture)
                {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, mat.metallicRoughnessTexture);
                    if (m_skinnedUniforms.metallicRoughnessTexture >= 0) glUniform1i(m_skinnedUniforms.metallicRoughnessTexture, 3);
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
        if (m_skinnedUniforms.useNormalTexture >= 0) glUniform1i(m_skinnedUniforms.useNormalTexture, 0);
        if (m_skinnedUniforms.useMetallicRoughnessTexture >= 0) glUniform1i(m_skinnedUniforms.useMetallicRoughnessTexture, 0);
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
                if (locs.useMetallicRoughnessTexture >= 0) glUniform1i(locs.useMetallicRoughnessTexture, mat.hasMetallicRoughnessTexture ? 1 : 0);

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
                if (mat.hasMetallicRoughnessTexture && mat.metallicRoughnessTexture)
                {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, mat.metallicRoughnessTexture);
                    if (locs.metallicRoughnessTexture >= 0) glUniform1i(locs.metallicRoughnessTexture, 3);
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
        if (locs.useMetallicRoughnessTexture >= 0) glUniform1i(locs.useMetallicRoughnessTexture, 0);
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
            uniform mat3 uNormalMatrix;

            out vec3 vWorldPos;
            out vec3 vNormal;
            out vec2 vTexCoord;

            void main()
            {
                vec4 worldPos = uModel * vec4(aPosition, 1.0);
                vWorldPos = worldPos.xyz;
                vNormal = uNormalMatrix * aNormal;
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
            uniform bool uUseMetallicRoughnessTexture;
            uniform sampler2D uAlbedoTexture;
            uniform sampler2D uNormalTexture;
            uniform sampler2D uMetallicRoughnessTexture;

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
                float metallic = uMetallic;
                float roughness = uRoughness;
                float ao = uAO;

                if (uUseMetallicRoughnessTexture)
                {
                    vec3 mrSample = texture(uMetallicRoughnessTexture, vTexCoord).rgb;
                    roughness = mrSample.g * roughness;
                    metallic = mrSample.b * metallic;
                }

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

                vec3 ambient = uAmbientColor * albedo * ao;
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
            uniform mat3 uNormalMatrix;

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
                vNormal = uNormalMatrix * skinnedNormal.xyz;
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
            uniform bool uUseMetallicRoughnessTexture;
            uniform sampler2D uAlbedoTexture;
            uniform sampler2D uNormalTexture;
            uniform sampler2D uMetallicRoughnessTexture;

            uniform mat4 uLightSpaceMatrix;
            uniform sampler2D uShadowMap;

            uniform int uPointLightCount;
            uniform vec3 uPointLightPositions[4];
            uniform vec3 uPointLightColors[4];
            uniform float uPointLightRanges[4];
            uniform float uPointLightIntensities[4];

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
                float metallic = uMetallic;
                float roughness = uRoughness;

                if (uUseMetallicRoughnessTexture)
                {
                    vec3 mrSample = texture(uMetallicRoughnessTexture, vTexCoord).rgb;
                    roughness = mrSample.g * roughness;
                    metallic = mrSample.b * metallic;
                }

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
            m_pbrUniforms.useMetallicRoughnessTexture = cache(m_pbrShader.program, "uUseMetallicRoughnessTexture");
            m_pbrUniforms.albedoTexture = cache(m_pbrShader.program, "uAlbedoTexture");
            m_pbrUniforms.normalTexture = cache(m_pbrShader.program, "uNormalTexture");
            m_pbrUniforms.metallicRoughnessTexture = cache(m_pbrShader.program, "uMetallicRoughnessTexture");
            m_pbrUniforms.pointLightCount = cache(m_pbrShader.program, "uPointLightCount");
            m_pbrUniforms.pointLightPositions = cache(m_pbrShader.program, "uPointLightPositions[0]");
            m_pbrUniforms.pointLightColors = cache(m_pbrShader.program, "uPointLightColors[0]");
            m_pbrUniforms.pointLightRanges = cache(m_pbrShader.program, "uPointLightRanges[0]");
            m_pbrUniforms.pointLightIntensities = cache(m_pbrShader.program, "uPointLightIntensities[0]");
            m_pbrUniforms.lightSpaceMatrix = cache(m_pbrShader.program, "uLightSpaceMatrix");
            m_pbrUniforms.shadowMap = cache(m_pbrShader.program, "uShadowMap");
            m_pbrUniforms.normalMatrix = cache(m_pbrShader.program, "uNormalMatrix");
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
            m_skinnedUniforms.useNormalTexture = cache(m_skinnedShader.program, "uUseNormalTexture");
            m_skinnedUniforms.useMetallicRoughnessTexture = cache(m_skinnedShader.program, "uUseMetallicRoughnessTexture");
            m_skinnedUniforms.albedoTexture = cache(m_skinnedShader.program, "uAlbedoTexture");
            m_skinnedUniforms.normalTexture = cache(m_skinnedShader.program, "uNormalTexture");
            m_skinnedUniforms.metallicRoughnessTexture = cache(m_skinnedShader.program, "uMetallicRoughnessTexture");
            m_skinnedUniforms.boneMatrices = cache(m_skinnedShader.program, "uBoneMatrices[0]");
            m_skinnedUniforms.lightSpaceMatrix = cache(m_skinnedShader.program, "uLightSpaceMatrix");
            m_skinnedUniforms.shadowMap = cache(m_skinnedShader.program, "uShadowMap");
            m_skinnedUniforms.normalMatrix = cache(m_skinnedShader.program, "uNormalMatrix");
            m_skinnedUniforms.pointLightCount = cache(m_skinnedShader.program, "uPointLightCount");
            m_skinnedUniforms.pointLightPositions = cache(m_skinnedShader.program, "uPointLightPositions[0]");
            m_skinnedUniforms.pointLightColors = cache(m_skinnedShader.program, "uPointLightColors[0]");
            m_skinnedUniforms.pointLightRanges = cache(m_skinnedShader.program, "uPointLightRanges[0]");
            m_skinnedUniforms.pointLightIntensities = cache(m_skinnedShader.program, "uPointLightIntensities[0]");
        }
    }

    void CreateShadowShader()
    {
        const char* staticVertSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;

            uniform mat4 uLightSpaceMatrix;
            uniform mat4 uModel;

            void main()
            {
                gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
            }
        )";

        const char* fragmentSrc = R"(
            #version 450 core
            void main()
            {
            }
        )";

        GLuint svs = CompileShader(GL_VERTEX_SHADER, staticVertSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (svs && fs)
            m_shadowShader.program = LinkProgram(svs, fs);

        const char* skinnedVertSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 3) in vec4 aBoneWeights;
            layout(location = 4) in vec4 aBoneIndices;

            uniform mat4 uLightSpaceMatrix;
            uniform mat4 uModel;
            uniform mat4 uBoneMatrices[128];

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
                gl_Position = uLightSpaceMatrix * uModel * skinnedPos;
            }
        )";

        GLuint skvs = CompileShader(GL_VERTEX_SHADER, skinnedVertSrc);
        GLuint fs2 = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (skvs && fs2)
            m_shadowSkinnedShader.program = LinkProgram(skvs, fs2);
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
            uniform sampler2D uBloomTexture;
            uniform float uExposure;
            uniform bool uUseBloom;

            void main()
            {
                vec3 color = texture(uSceneTexture, vTexCoord).rgb;

                if (uUseBloom)
                {
                    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
                    color += bloomColor;
                }

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
        glGenTextures(1, &m_sceneDepthTexture);

        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, m_sceneDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneColorTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepthTexture, 0);

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
        if (m_sceneDepthTexture) { glDeleteTextures(1, &m_sceneDepthTexture); m_sceneDepthTexture = 0; }
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

    void CreateBloomShaders()
    {
        const char* extractSrc = R"(
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

        const char* extractFrag = R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec4 FragColor;
            uniform sampler2D uSceneTexture;
            uniform float uThreshold;
            void main()
            {
                vec3 color = texture(uSceneTexture, vTexCoord).rgb;
                float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
                if (brightness > uThreshold)
                    FragColor = vec4(color, 1.0);
                else
                    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            }
        )";

        GLuint evs = CompileShader(GL_VERTEX_SHADER, extractSrc);
        GLuint efs = CompileShader(GL_FRAGMENT_SHADER, extractFrag);
        if (evs && efs)
            m_bloomExtractShader.program = LinkProgram(evs, efs);

        const char* blurFrag = R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec4 FragColor;
            uniform sampler2D uTexture;
            uniform vec2 uTexelSize;
            uniform bool uHorizontal;
            void main()
            {
                float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
                vec3 result = texture(uTexture, vTexCoord).rgb * weights[0];
                vec2 offset = uHorizontal ? vec2(uTexelSize.x, 0.0) : vec2(0.0, uTexelSize.y);
                for (int i = 1; i < 5; ++i)
                {
                    result += texture(uTexture, vTexCoord + offset * float(i)).rgb * weights[i];
                    result += texture(uTexture, vTexCoord - offset * float(i)).rgb * weights[i];
                }
                FragColor = vec4(result, 1.0);
            }
        )";

        GLuint bvs = CompileShader(GL_VERTEX_SHADER, extractSrc);
        GLuint bfs = CompileShader(GL_FRAGMENT_SHADER, blurFrag);
        if (bvs && bfs)
            m_bloomBlurShader.program = LinkProgram(bvs, bfs);
    }

    void CreateBloomFBOs(int width, int height)
    {
        DestroyBloomFBOs();
        if (width <= 0 || height <= 0) return;

        int bloomW = width / 2;
        int bloomH = height / 2;

        glGenFramebuffers(1, &m_bloomExtractFBO);
        glGenTextures(1, &m_bloomExtractTexture);
        glBindTexture(GL_TEXTURE_2D, m_bloomExtractTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomW, bloomH, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomExtractFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomExtractTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        for (int i = 0; i < 2; ++i)
        {
            glGenFramebuffers(1, &m_bloomPingPongFBO[i]);
            glGenTextures(1, &m_bloomPingPongTexture[i]);
            glBindTexture(GL_TEXTURE_2D, m_bloomPingPongTexture[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomW, bloomH, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, m_bloomPingPongFBO[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomPingPongTexture[i], 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DestroyBloomFBOs()
    {
        if (m_bloomExtractFBO) { glDeleteFramebuffers(1, &m_bloomExtractFBO); m_bloomExtractFBO = 0; }
        if (m_bloomExtractTexture) { glDeleteTextures(1, &m_bloomExtractTexture); m_bloomExtractTexture = 0; }
        for (int i = 0; i < 2; ++i)
        {
            if (m_bloomPingPongFBO[i]) { glDeleteFramebuffers(1, &m_bloomPingPongFBO[i]); m_bloomPingPongFBO[i] = 0; }
            if (m_bloomPingPongTexture[i]) { glDeleteTextures(1, &m_bloomPingPongTexture[i]); m_bloomPingPongTexture[i] = 0; }
        }
    }

    void RenderBloom()
    {
        if (!m_bloomExtractShader.program || !m_bloomBlurShader.program) return;
        if (!m_bloomExtractFBO) return;

        EnsureScreenQuad();

        int bloomW = m_windowWidth / 2;
        int bloomH = m_windowHeight / 2;

        glDisable(GL_DEPTH_TEST);

        glUseProgram(m_bloomExtractShader.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(m_bloomExtractShader.program, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);
        GLint thresholdLoc = glGetUniformLocation(m_bloomExtractShader.program, "uThreshold");
        if (thresholdLoc >= 0) glUniform1f(thresholdLoc, 1.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomExtractFBO);
        glViewport(0, 0, bloomW, bloomH);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(m_screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(m_bloomBlurShader.program);
        GLint texLoc = glGetUniformLocation(m_bloomBlurShader.program, "uTexture");
        GLint texelSizeLoc = glGetUniformLocation(m_bloomBlurShader.program, "uTexelSize");
        GLint horizontalLoc = glGetUniformLocation(m_bloomBlurShader.program, "uHorizontal");

        if (texLoc >= 0) glUniform1i(texLoc, 0);

        bool horizontal = true;
        bool firstIteration = true;
        for (int i = 0; i < 10; ++i)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_bloomPingPongFBO[horizontal ? 1 : 0]);
            if (horizontalLoc >= 0) glUniform1i(horizontalLoc, horizontal ? 1 : 0);
            if (texelSizeLoc >= 0)
            {
                float texelX = 1.0f / static_cast<float>(bloomW);
                float texelY = 1.0f / static_cast<float>(bloomH);
                glUniform2f(texelSizeLoc, texelX, texelY);
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, firstIteration ? m_bloomExtractTexture : m_bloomPingPongTexture[horizontal ? 0 : 1]);
            glViewport(0, 0, bloomW, bloomH);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindVertexArray(m_screenQuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal = !horizontal;
            firstIteration = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_windowWidth, m_windowHeight);
        glUseProgram(0);
        glEnable(GL_DEPTH_TEST);
    }

    void EnsureScreenQuad()
    {
        if (m_screenQuadVAO != 0) return;
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
    GLShaderProgram m_shadowSkinnedShader{};
    GLShaderProgram m_postProcessShader{};
    GLShaderProgram m_bloomExtractShader{};
    GLShaderProgram m_bloomBlurShader{};
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
    GLuint m_sceneDepthTexture = 0;
    GLuint m_shadowMapFBO = 0;
    GLuint m_shadowMapTexture = 0;
    int m_shadowMapSize = 2048;
    GLuint m_screenQuadVAO = 0;
    GLuint m_screenQuadVBO = 0;
    Mat4 m_lightSpaceMatrix = Mat4(1.0f);
    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    GLuint m_bloomExtractFBO = 0;
    GLuint m_bloomExtractTexture = 0;
    GLuint m_bloomPingPongFBO[2] = {0, 0};
    GLuint m_bloomPingPongTexture[2] = {0, 0};
};
}
