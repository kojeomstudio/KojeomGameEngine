#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/OpenGL/OpenGLContext.h"
#include "Renderer/OpenGL/OpenGLShader.h"
#include "Renderer/OpenGL/OpenGLBuffer.h"
#include "Renderer/OpenGL/OpenGLTexture.h"
#include "Renderer/OpenGL/OpenGLFramebuffer.h"
#include "Core/Log.h"
#include <stb_image_write.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <string>
#include <limits>
#include <random>

namespace Kojeom
{
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
    GLint useEmissiveTexture = -1;
    GLint useAOTexture = -1;
    GLint albedoTexture = -1;
    GLint normalTexture = -1;
    GLint metallicRoughnessTexture = -1;
    GLint emissiveTexture = -1;
    GLint aoTexture = -1;
    GLint boneMatrices = -1;
    GLint pointLightCount = -1;
    GLint pointLightPositions = -1;
    GLint pointLightColors = -1;
    GLint pointLightRanges = -1;
    GLint pointLightIntensities = -1;
    GLint lightSpaceMatrix = -1;
    GLint shadowMap = -1;
    GLint normalMatrix = -1;
    GLint ambientGroundColor = -1;
    GLint ambientGroundBlend = -1;
    GLint brdfLUT = -1;
    GLint viewProjectionFrag = -1;
    GLint nearPlane = -1;
    GLint farPlane = -1;
};

class OpenGLRenderer : public IRendererBackend
{
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override { Shutdown(); }

    bool Initialize(void* windowHandle) override
    {
        if (!m_context.Initialize(windowHandle))
            return false;

        if (!CreateAllShaders())
            return false;

        CacheAllUniformLocations();
        CreateDefaultMesh();
        CreateDefaultTextures();
        CreateBRDFLUT();
        CreateSSAO();
        CreateBoneUBO();

        int fbWidth = 0, fbHeight = 0;
        OpenGLContext::GetFramebufferSize(windowHandle, &fbWidth, &fbHeight);
        if (fbWidth > 0 && fbHeight > 0)
        {
            m_windowWidth = fbWidth;
            m_windowHeight = fbHeight;
            m_context.SetViewport(0, 0, fbWidth, fbHeight);
            CreateSceneFBO(fbWidth, fbHeight);
            CreateBloomFBOs(fbWidth, fbHeight);
        }

        KE_LOG_INFO("OpenGL Renderer initialized");
        return true;
    }

    void Shutdown() override
    {
        if (!m_context.IsInitialized() && m_bufferManager.GetMeshCache().empty())
            return;

        m_shaderManager.ClearAll();
        m_bufferManager.ClearAll();
        m_textureManager.ClearAll();
        m_materialCache.clear();
        m_boneMatricesCache.clear();

        if (m_boneUBO) { glDeleteBuffers(1, &m_boneUBO); m_boneUBO = 0; }

        if (m_defaultAlbedoTexture) { glDeleteTextures(1, &m_defaultAlbedoTexture); m_defaultAlbedoTexture = 0; }
        if (m_defaultNormalTexture) { glDeleteTextures(1, &m_defaultNormalTexture); m_defaultNormalTexture = 0; }
        if (m_screenQuadVAO) { glDeleteVertexArrays(1, &m_screenQuadVAO); m_screenQuadVAO = 0; }
        if (m_screenQuadVBO) { glDeleteBuffers(1, &m_screenQuadVBO); m_screenQuadVBO = 0; }
        if (m_brdfLUT) { glDeleteTextures(1, &m_brdfLUT); m_brdfLUT = 0; }

        m_defaultCubeHandle = INVALID_HANDLE;
        m_defaultPlaneHandle = INVALID_HANDLE;
        m_defaultAlbedoTextureHandle = INVALID_HANDLE;

        DestroySceneFBO();
        DestroyBloomFBOs();
        DestroyShadowMap();
        DestroySSAO();
        DestroyFXAA();

        m_context.Shutdown();
        KE_LOG_INFO("OpenGL Renderer shutdown");
    }

    void OnResize(int width, int height) override
    {
        m_windowWidth = width;
        m_windowHeight = height;
        m_context.SetViewport(0, 0, width, height);
        CreateSceneFBO(width, height);
        CreateBloomFBOs(width, height);
        CreateSSAOFBOs(width, height);
        CreateFXAAFBO(width, height);
    }

    void BeginFrame() override
    {
        m_context.ClearColorDepth();
    }

    void EndFrame() override
    {
    }

    void Render(const RenderScene& scene) override
    {
        m_drawCallCount = 0;

        RenderShadowMap(scene);

        m_context.BindFramebuffer(m_sceneFBO);
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        m_context.ClearColorDepth();

        RenderStaticMeshes(scene);
        RenderSkinnedMeshes(scene);
        RenderTerrain(scene);

        m_context.BindDefaultFramebuffer();
        RenderSSAO(scene);
        RenderBloom();
        RenderPostProcess();
        RenderFXAA();

        RenderDebugOverlay();
    }

    uint32_t GetDrawCallCount() const { return m_drawCallCount; }

    AssetHandle UploadMesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
        int vertexStride = 8) override
    {
        AssetHandle handle = GenerateHandle();
        m_bufferManager.UploadMesh(vertices, indices, vertexStride, handle);
        return handle;
    }

    AssetHandle UploadSkinnedMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices) override
    {
        AssetHandle handle = GenerateHandle();
        m_bufferManager.UploadSkinnedMesh(vertices, indices, handle);
        return handle;
    }

    AssetHandle UploadTexture(int width, int height, int channels, const uint8_t* data) override
    {
        AssetHandle handle = GenerateHandle();
        m_textureManager.UploadTexture(width, height, channels, data, handle);
        return handle;
    }

    AssetHandle UploadTextureSRGB(int width, int height, int channels, const uint8_t* data) override
    {
        AssetHandle handle = GenerateHandle();
        m_textureManager.UploadTexture(width, height, channels, data, handle, true);
        return handle;
    }

    AssetHandle RegisterMaterial(const Vec3& albedo, float metallic, float roughness,
        AssetHandle albedoTexHandle = INVALID_HANDLE, bool hasTex = false,
        AssetHandle normalTexHandle = INVALID_HANDLE, bool hasNormalTex = false,
        const Vec3& emissiveColor = Vec3(0.0f), float emissiveStr = 1.0f,
        AssetHandle metallicRoughnessTexHandle = INVALID_HANDLE, bool hasMetallicRoughnessTex = false,
        float ao = 1.0f,
        AssetHandle emissiveTexHandle = INVALID_HANDLE, bool hasEmissiveTex = false,
        AssetHandle aoTexHandle = INVALID_HANDLE, bool hasAOTex = false) override
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
        mat.hasEmissiveTexture = hasEmissiveTex;
        mat.hasAOTexture = hasAOTex;
        mat.emissive = emissiveColor;
        mat.emissiveStrength = emissiveStr;
        if (hasTex && albedoTexHandle != INVALID_HANDLE)
        {
            auto* tex = m_textureManager.GetTexture(albedoTexHandle);
            if (tex) mat.albedoTexture = tex->texture;
        }
        if (hasNormalTex && normalTexHandle != INVALID_HANDLE)
        {
            auto* tex = m_textureManager.GetTexture(normalTexHandle);
            if (tex) mat.normalTexture = tex->texture;
        }
        if (hasMetallicRoughnessTex && metallicRoughnessTexHandle != INVALID_HANDLE)
        {
            auto* tex = m_textureManager.GetTexture(metallicRoughnessTexHandle);
            if (tex) mat.metallicRoughnessTexture = tex->texture;
        }
        if (hasEmissiveTex && emissiveTexHandle != INVALID_HANDLE)
        {
            auto* tex = m_textureManager.GetTexture(emissiveTexHandle);
            if (tex) mat.emissiveTexture = tex->texture;
        }
        if (hasAOTex && aoTexHandle != INVALID_HANDLE)
        {
            auto* tex = m_textureManager.GetTexture(aoTexHandle);
            if (tex) mat.aoTexture = tex->texture;
        }
        m_materialCache[handle] = mat;
        return handle;
    }

    void UploadBoneMatrices(AssetHandle handle, const std::vector<Mat4>& matrices) override
    {
        m_boneMatricesCache[handle] = matrices;
    }

    void RemoveMesh(AssetHandle handle) override { m_bufferManager.RemoveMesh(handle); }
    void RemoveTexture(AssetHandle handle) override { m_textureManager.RemoveTexture(handle); }

    bool SaveScreenshot(const std::string& path, int width, int height) override
    {
        if (width <= 0 || height <= 0) return false;
        if (path.empty()) return false;
        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Screenshot path validation failed: {}", path);
            return false;
        }

        std::string dir = FileSystem::GetDirectory(path);
        if (!dir.empty() && !FileSystem::FileExists(dir))
        {
            FileSystem::CreateDirectory(dir);
        }

        GLint prevFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
        m_context.BindDefaultFramebuffer();
        m_context.Flush();

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

    void SetDebugOverlayVisible(bool visible) { m_showDebugOverlay = visible; }
    bool IsDebugOverlayVisible() const { return m_showDebugOverlay; }

    void UpdateMaterialProperty(AssetHandle materialHandle, const std::string& propertyName, float value) override
    {
        auto it = m_materialCache.find(materialHandle);
        if (it == m_materialCache.end()) return;
        auto& mat = it->second;
        if (propertyName == "metallic") mat.metallic = value;
        else if (propertyName == "roughness") mat.roughness = value;
        else if (propertyName == "ao") mat.ao = value;
        else if (propertyName == "emissiveStrength") mat.emissiveStrength = value;
    }

    void UpdateMaterialProperty(AssetHandle materialHandle, const std::string& propertyName, const Vec3& value) override
    {
        auto it = m_materialCache.find(materialHandle);
        if (it == m_materialCache.end()) return;
        auto& mat = it->second;
        if (propertyName == "albedo") mat.albedo = value;
        else if (propertyName == "emissive") mat.emissive = value;
    }

    void UpdateDebugStats(float frameTimeMs, int entityCount, int loadedAssets)
    {
        m_debugStats.frameTimeMs = frameTimeMs;
        m_debugStats.entityCount = entityCount;
        m_debugStats.loadedAssets = loadedAssets;
        m_debugStats.fps = (frameTimeMs > 0.0f) ? (1000.0f / frameTimeMs) : 0.0f;
    }

public:
    std::unordered_map<AssetHandle, std::vector<Mat4>> m_boneMatricesCache;
    GLuint m_boneUBO = 0;

private:
    AssetHandle GenerateHandle() { return ++m_nextHandle; }

    Mat3 ComputeNormalMatrix(const Mat4& modelMatrix) const
    {
        return glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    }

    bool CreateAllShaders()
    {
        GLuint pbr = m_shaderManager.CreateProgram(PBRVertexSrc(), PBRFragmentSrc());
        if (!pbr)
        {
            KE_LOG_ERROR("Failed to compile/link PBR shader program");
            return false;
        }
        m_shaderManager.AddShader("pbr", pbr);

        GLuint skinned = m_shaderManager.CreateProgram(SkinnedVertexSrc(), SkinnedFragmentSrc());
        if (!skinned)
        {
            KE_LOG_ERROR("Failed to compile/link skinned shader program");
            return false;
        }
        m_shaderManager.AddShader("skinned", skinned);

        GLuint shadow = CreateShadowShader();
        if (!shadow)
        {
            KE_LOG_ERROR("Failed to compile/link shadow shader program");
            return false;
        }

        GLuint shadowSkinned = CreateShadowSkinnedShader();
        if (!shadowSkinned)
        {
            KE_LOG_ERROR("Failed to compile/link shadow skinned shader program");
            return false;
        }

        GLuint postProcess = m_shaderManager.CreateProgram(PostProcessVertexSrc(), PostProcessFragmentSrc());
        if (!postProcess)
        {
            KE_LOG_ERROR("Failed to compile/link post-process shader program");
            return false;
        }
        m_shaderManager.AddShader("postprocess", postProcess);

        CreateBloomShaders();
        CreateSSAOShaders();
        CreateFXASShader();

        KE_LOG_INFO("All shader programs compiled successfully");
        return true;
    }

    void CacheAllUniformLocations()
    {
        auto cache = [](GLuint program, const char* name) -> GLint
        {
            return glGetUniformLocation(program, name);
        };

        GLuint pbr = m_shaderManager.GetProgram("pbr");
        if (pbr)
        {
            m_pbrUniforms = {};
            m_pbrUniforms.viewProjection = cache(pbr, "uViewProjection");
            m_pbrUniforms.model = cache(pbr, "uModel");
            m_pbrUniforms.cameraPos = cache(pbr, "uCameraPos");
            m_pbrUniforms.lightDirection = cache(pbr, "uLightDirection");
            m_pbrUniforms.lightColor = cache(pbr, "uLightColor");
            m_pbrUniforms.lightIntensity = cache(pbr, "uLightIntensity");
            m_pbrUniforms.ambientColor = cache(pbr, "uAmbientColor");
            m_pbrUniforms.albedo = cache(pbr, "uAlbedo");
            m_pbrUniforms.metallic = cache(pbr, "uMetallic");
            m_pbrUniforms.roughness = cache(pbr, "uRoughness");
            m_pbrUniforms.ao = cache(pbr, "uAO");
            m_pbrUniforms.emissive = cache(pbr, "uEmissive");
            m_pbrUniforms.emissiveStrength = cache(pbr, "uEmissiveStrength");
            m_pbrUniforms.useAlbedoTexture = cache(pbr, "uUseAlbedoTexture");
            m_pbrUniforms.useNormalTexture = cache(pbr, "uUseNormalTexture");
            m_pbrUniforms.useMetallicRoughnessTexture = cache(pbr, "uUseMetallicRoughnessTexture");
            m_pbrUniforms.albedoTexture = cache(pbr, "uAlbedoTexture");
            m_pbrUniforms.normalTexture = cache(pbr, "uNormalTexture");
            m_pbrUniforms.metallicRoughnessTexture = cache(pbr, "uMetallicRoughnessTexture");
            m_pbrUniforms.pointLightCount = cache(pbr, "uPointLightCount");
            m_pbrUniforms.pointLightPositions = cache(pbr, "uPointLightPositions[0]");
            m_pbrUniforms.pointLightColors = cache(pbr, "uPointLightColors[0]");
            m_pbrUniforms.pointLightRanges = cache(pbr, "uPointLightRanges[0]");
            m_pbrUniforms.pointLightIntensities = cache(pbr, "uPointLightIntensities[0]");
            m_pbrUniforms.lightSpaceMatrix = cache(pbr, "uLightSpaceMatrix");
            m_pbrUniforms.shadowMap = cache(pbr, "uShadowMap");
            m_pbrUniforms.normalMatrix = cache(pbr, "uNormalMatrix");
            m_pbrUniforms.ambientGroundColor = cache(pbr, "uAmbientGroundColor");
            m_pbrUniforms.ambientGroundBlend = cache(pbr, "uAmbientGroundBlend");
            m_pbrUniforms.brdfLUT = cache(pbr, "uBRDFLUT");
            m_pbrUniforms.useEmissiveTexture = cache(pbr, "uUseEmissiveTexture");
            m_pbrUniforms.emissiveTexture = cache(pbr, "uEmissiveTexture");
            m_pbrUniforms.useAOTexture = cache(pbr, "uUseAOTexture");
            m_pbrUniforms.aoTexture = cache(pbr, "uAOTexture");
            m_cascadeSplitLoc = cache(pbr, "uCascadeSplitDistances");
            m_pbrUniforms.viewProjectionFrag = cache(pbr, "uViewProjection");
            m_pbrUniforms.nearPlane = cache(pbr, "uNearPlane");
            m_pbrUniforms.farPlane = cache(pbr, "uFarPlane");
        }

        GLuint skinned = m_shaderManager.GetProgram("skinned");
        if (skinned)
        {
            m_skinnedUniforms = {};
            m_skinnedUniforms.viewProjection = cache(skinned, "uViewProjection");
            m_skinnedUniforms.model = cache(skinned, "uModel");
            m_skinnedUniforms.cameraPos = cache(skinned, "uCameraPos");
            m_skinnedUniforms.lightDirection = cache(skinned, "uLightDirection");
            m_skinnedUniforms.lightColor = cache(skinned, "uLightColor");
            m_skinnedUniforms.lightIntensity = cache(skinned, "uLightIntensity");
            m_skinnedUniforms.ambientColor = cache(skinned, "uAmbientColor");
            m_skinnedUniforms.albedo = cache(skinned, "uAlbedo");
            m_skinnedUniforms.metallic = cache(skinned, "uMetallic");
            m_skinnedUniforms.roughness = cache(skinned, "uRoughness");
            m_skinnedUniforms.ao = cache(skinned, "uAO");
            m_skinnedUniforms.emissive = cache(skinned, "uEmissive");
            m_skinnedUniforms.emissiveStrength = cache(skinned, "uEmissiveStrength");
            m_skinnedUniforms.useAlbedoTexture = cache(skinned, "uUseAlbedoTexture");
            m_skinnedUniforms.useNormalTexture = cache(skinned, "uUseNormalTexture");
            m_skinnedUniforms.useMetallicRoughnessTexture = cache(skinned, "uUseMetallicRoughnessTexture");
            m_skinnedUniforms.albedoTexture = cache(skinned, "uAlbedoTexture");
            m_skinnedUniforms.normalTexture = cache(skinned, "uNormalTexture");
            m_skinnedUniforms.metallicRoughnessTexture = cache(skinned, "uMetallicRoughnessTexture");
            m_skinnedUniforms.boneMatrices = cache(skinned, "uBoneMatrices[0]");
            m_skinnedUniforms.lightSpaceMatrix = cache(skinned, "uLightSpaceMatrix");
            m_skinnedUniforms.shadowMap = cache(skinned, "uShadowMap");
            m_skinnedUniforms.normalMatrix = cache(skinned, "uNormalMatrix");
            m_skinnedUniforms.pointLightCount = cache(skinned, "uPointLightCount");
            m_skinnedUniforms.pointLightPositions = cache(skinned, "uPointLightPositions[0]");
            m_skinnedUniforms.pointLightColors = cache(skinned, "uPointLightColors[0]");
            m_skinnedUniforms.pointLightRanges = cache(skinned, "uPointLightRanges[0]");
            m_skinnedUniforms.pointLightIntensities = cache(skinned, "uPointLightIntensities[0]");
            m_skinnedUniforms.useEmissiveTexture = cache(skinned, "uUseEmissiveTexture");
            m_skinnedUniforms.emissiveTexture = cache(skinned, "uEmissiveTexture");
            m_skinnedUniforms.useAOTexture = cache(skinned, "uUseAOTexture");
            m_skinnedUniforms.aoTexture = cache(skinned, "uAOTexture");
            m_skinnedUniforms.viewProjectionFrag = cache(skinned, "uViewProjection");
            m_skinnedUniforms.nearPlane = cache(skinned, "uNearPlane");
            m_skinnedUniforms.farPlane = cache(skinned, "uFarPlane");
        }
    }

    void SetSceneUniforms(const RenderScene& scene, const UniformLocations& locs)
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
        if (locs.ambientGroundColor >= 0)
        {
            Vec3 groundColor = scene.light.ambientColor * 0.4f;
            glUniform3fv(locs.ambientGroundColor, 1, &groundColor[0]);
        }
        if (locs.ambientGroundBlend >= 0)
            glUniform1f(locs.ambientGroundBlend, 0.5f);
        if (locs.viewProjectionFrag >= 0)
            glUniformMatrix4fv(locs.viewProjectionFrag, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);
        if (locs.nearPlane >= 0)
            glUniform1f(locs.nearPlane, scene.camera.nearPlane);
        if (locs.farPlane >= 0)
            glUniform1f(locs.farPlane, scene.camera.farPlane);

        int plCount = std::min(scene.light.pointLightCount,
            static_cast<int>(LightData::MAX_POINT_LIGHTS));
        if (locs.pointLightCount >= 0)
            glUniform1i(locs.pointLightCount, plCount);
        if (plCount > 0)
        {
            float positions[12] = {}, colors[12] = {}, ranges[4] = {}, intensities[4] = {};
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
            if (locs.pointLightPositions >= 0) glUniform3fv(locs.pointLightPositions, plCount, positions);
            if (locs.pointLightColors >= 0) glUniform3fv(locs.pointLightColors, plCount, colors);
            if (locs.pointLightRanges >= 0) glUniform1fv(locs.pointLightRanges, plCount, ranges);
            if (locs.pointLightIntensities >= 0) glUniform1fv(locs.pointLightIntensities, plCount, intensities);
        }
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
                if (locs.useEmissiveTexture >= 0) glUniform1i(locs.useEmissiveTexture, mat.hasEmissiveTexture ? 1 : 0);
                if (locs.useAOTexture >= 0) glUniform1i(locs.useAOTexture, mat.hasAOTexture ? 1 : 0);

                if (mat.hasAlbedoTexture && mat.albedoTexture)
                {
                    m_textureManager.BindTexture(mat.albedoTexture, GL_TEXTURE0);
                    if (locs.albedoTexture >= 0) glUniform1i(locs.albedoTexture, 0);
                }
                if (mat.hasNormalTexture && mat.normalTexture)
                {
                    m_textureManager.BindTexture(mat.normalTexture, GL_TEXTURE1);
                    if (locs.normalTexture >= 0) glUniform1i(locs.normalTexture, 1);
                }
                if (mat.hasMetallicRoughnessTexture && mat.metallicRoughnessTexture)
                {
                    m_textureManager.BindTexture(mat.metallicRoughnessTexture, GL_TEXTURE3);
                    if (locs.metallicRoughnessTexture >= 0) glUniform1i(locs.metallicRoughnessTexture, 3);
                }
                if (mat.hasEmissiveTexture && mat.emissiveTexture)
                {
                    m_textureManager.BindTexture(mat.emissiveTexture, GL_TEXTURE5);
                    if (locs.emissiveTexture >= 0) glUniform1i(locs.emissiveTexture, 5);
                }
                if (mat.hasAOTexture && mat.aoTexture)
                {
                    m_textureManager.BindTexture(mat.aoTexture, GL_TEXTURE6);
                    if (locs.aoTexture >= 0) glUniform1i(locs.aoTexture, 6);
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
        if (locs.useEmissiveTexture >= 0) glUniform1i(locs.useEmissiveTexture, 0);
        if (locs.useAOTexture >= 0) glUniform1i(locs.useAOTexture, 0);
    }

    struct FrustumPlane { Vec3 normal; float d; };

    std::array<FrustumPlane, 6> ExtractFrustumPlanes(const Mat4& vp) const
    {
        std::array<FrustumPlane, 6> planes;
        Vec4 col0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
        Vec4 col1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
        Vec4 col2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
        Vec4 col3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

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

    void RenderStaticMeshes(const RenderScene& scene)
    {
        GLuint pbrProg = m_shaderManager.GetProgram("pbr");
        if (!pbrProg) return;
        glUseProgram(pbrProg);

        SetSceneUniforms(scene, m_pbrUniforms);

        if (m_pbrUniforms.shadowMap >= 0) glUniform1i(m_pbrUniforms.shadowMap, 2);
        if (m_pbrUniforms.lightSpaceMatrix >= 0)
            glUniformMatrix4fv(m_pbrUniforms.lightSpaceMatrix, 3, GL_FALSE, &m_cascadeLightSpaceMatrices[0][0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMapTexture);

        if (m_cascadeSplitLoc >= 0)
            glUniform3f(m_cascadeSplitLoc, m_cascadeSplitDistances[0], m_cascadeSplitDistances[1], m_cascadeSplitDistances[2]);

        if (m_pbrUniforms.brdfLUT >= 0 && m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
            glUniform1i(m_pbrUniforms.brdfLUT, 4);
            glActiveTexture(GL_TEXTURE0);
        }

        auto frustumPlanes = ExtractFrustumPlanes(scene.camera.viewProjectionMatrix);

        std::vector<size_t> sortedIndices(scene.staticDrawCommands.size());
        for (size_t i = 0; i < sortedIndices.size(); ++i) sortedIndices[i] = i;
        std::sort(sortedIndices.begin(), sortedIndices.end(), [&](size_t a, size_t b) {
            return scene.staticDrawCommands[a].materialHandle < scene.staticDrawCommands[b].materialHandle;
        });

        AssetHandle lastMaterial = INVALID_HANDLE + 1;
        for (size_t idx : sortedIndices)
        {
            const auto& cmd = scene.staticDrawCommands[idx];

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

            if (cmd.materialHandle != lastMaterial)
            {
                SetMaterialUniforms(cmd.materialHandle, m_pbrUniforms);
                lastMaterial = cmd.materialHandle;
            }

            const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.meshHandle);
            if (!mesh) mesh = &m_defaultMesh;

            if (mesh && mesh->vao)
            {
                glBindVertexArray(mesh->vao);
                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        m_textureManager.UnbindTexture(GL_TEXTURE2);
        m_textureManager.UnbindTexture(GL_TEXTURE1);
        m_textureManager.UnbindTexture(GL_TEXTURE0);
        if (m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        glUseProgram(0);
    }

    void RenderFXAA()
    {
        GLuint fxaaProg = m_shaderManager.GetProgram("fxaa");
        if (!fxaaProg || !m_fxaaEnabled)
        {
            m_context.BindDefaultFramebuffer();
            m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
            m_context.ClearColor();
            m_context.EnableDepthTest(false);
            m_bufferManager.CreateScreenQuad(m_screenQuadVAO, m_screenQuadVBO);
            GLuint ppProg = m_shaderManager.GetProgram("postprocess");
            if (ppProg)
            {
                glUseProgram(ppProg);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_fxaaColorTexture);
                glUniform1i(glGetUniformLocation(ppProg, "uSceneTexture"), 0);
                glUniform1i(glGetUniformLocation(ppProg, "uUseBloom"), 0);
                glUniform1i(glGetUniformLocation(ppProg, "uUseSSAO"), 0);
                glUniform1f(glGetUniformLocation(ppProg, "uExposure"), 1.0f);
                m_bufferManager.DrawScreenQuad(m_screenQuadVAO);
                glUseProgram(0);
            }
            m_context.EnableDepthTest(true);
            return;
        }

        m_bufferManager.CreateScreenQuad(m_screenQuadVAO, m_screenQuadVBO);
        m_context.EnableDepthTest(false);

        m_context.BindDefaultFramebuffer();
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        m_context.ClearColor();

        glUseProgram(fxaaProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_fxaaColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(fxaaProg, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);
        GLint screenSizeLoc = glGetUniformLocation(fxaaProg, "uScreenSize");
        if (screenSizeLoc >= 0)
            glUniform2f(screenSizeLoc, 1.0f / static_cast<float>(m_windowWidth), 1.0f / static_cast<float>(m_windowHeight));

        m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

        glUseProgram(0);
        m_context.EnableDepthTest(true);
    }

    void CreateFXAAFBO(int width, int height)
    {
        DestroyFXAA();
        if (width <= 0 || height <= 0) return;

        glGenTextures(1, &m_fxaaColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_fxaaColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &m_fxaaFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fxaaFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fxaaColorTexture, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            KE_LOG_ERROR("FXAA FBO incomplete: {}", static_cast<int>(status));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DestroyFXAA()
    {
        if (m_fxaaFBO) { glDeleteFramebuffers(1, &m_fxaaFBO); m_fxaaFBO = 0; }
        if (m_fxaaColorTexture) { glDeleteTextures(1, &m_fxaaColorTexture); m_fxaaColorTexture = 0; }
    }

    void CreateFXASShader()
    {
        const char* screenVertSrc = R"(
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

        const char* fxaaFrag = R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec4 FragColor;

            uniform sampler2D uSceneTexture;
            uniform vec2 uScreenSize;

            #define FXAA_QUALITY__PS 5
            #define FXAA_QUALITY__P0 1.0
            #define FXAA_QUALITY__P1 1.5
            #define FXAA_QUALITY__P2 2.0
            #define FXAA_QUALITY__P3 4.0
            #define FXAA_QUALITY__P4 12.0

            void main()
            {
                vec2 posM = vTexCoord;
                vec4 rgbyM = texture(uSceneTexture, posM);
                float lumaM = dot(rgbyM.rgb, vec3(0.2126, 0.7152, 0.0722));

                float lumaN = dot(texture(uSceneTexture, posM + vec2(0.0, -uScreenSize.y)).rgb, vec3(0.2126, 0.7152, 0.0722));
                float lumaS = dot(texture(uSceneTexture, posM + vec2(0.0,  uScreenSize.y)).rgb, vec3(0.2126, 0.7152, 0.0722));
                float lumaW = dot(texture(uSceneTexture, posM + vec2(-uScreenSize.x, 0.0)).rgb, vec3(0.2126, 0.7152, 0.0722));
                float lumaE = dot(texture(uSceneTexture, posM + vec2( uScreenSize.x, 0.0)).rgb, vec3(0.2126, 0.7152, 0.0722));

                float lumaNS = lumaN + lumaS;
                float lumaWE = lumaW + lumaE;
                float edgeHorz1 = abs(lumaN - lumaS) * 2.0 + abs(lumaM - lumaE);
                float edgeVert1 = abs(lumaE - lumaW) * 2.0 + abs(lumaM - lumaN);

                bool horzSpan = edgeHorz1 >= edgeVert1;
                float contrast = max(abs(lumaNS - 2.0 * lumaM), abs(lumaWE - 2.0 * lumaM));
                if (contrast < 0.0312)
                {
                    FragColor = rgbyM;
                    return;
                }

                float lengthSpan = horzSpan ? uScreenSize.y : uScreenSize.x;
                vec2 dir = horzSpan ? vec2(0.0, 1.0) : vec2(1.0, 0.0);
                float sign = horzSpan ?
                    (lumaN - lumaS) :
                    (lumaE - lumaW);
                dir = dir * (sign < 0.0 ? -1.0 : 1.0);

                float lumaEnd1 = dot(texture(uSceneTexture, posM + dir * lengthSpan * 0.5).rgb, vec3(0.2126, 0.7152, 0.0722));
                float lumaEnd2 = dot(texture(uSceneTexture, posM - dir * lengthSpan * 0.5).rgb, vec3(0.2126, 0.7152, 0.0722));
                lumaEnd1 -= lumaM;
                lumaEnd2 -= lumaM;

                bool done1 = abs(lumaEnd1) >= contrast;
                bool done2 = abs(lumaEnd2) >= contrast;
                if (done1 && done2)
                {
                    FragColor = rgbyM;
                    return;
                }

                float spanLength = 0.0;
                if (!done1) spanLength += FXAA_QUALITY__P1 / lengthSpan;
                if (!done2) spanLength += FXAA_QUALITY__P1 / lengthSpan;

                float subpixelOffset = clamp(0.5 + spanLength * 0.5, 0.0, 1.0) * 0.5;
                FragColor = texture(uSceneTexture, posM + dir * subpixelOffset * lengthSpan);
            }
        )";

        GLuint prog = m_shaderManager.CreateProgram(screenVertSrc, fxaaFrag);
        if (prog) m_shaderManager.AddShader("fxaa", prog);
    }

    void RenderSkinnedMeshes(const RenderScene& scene)
    {
        GLuint skinnedProg = m_shaderManager.GetProgram("skinned");
        if (!skinnedProg || scene.skinnedDrawCommands.empty()) return;
        glUseProgram(skinnedProg);

        SetSceneUniforms(scene, m_skinnedUniforms);

        if (m_skinnedUniforms.shadowMap >= 0) glUniform1i(m_skinnedUniforms.shadowMap, 2);
        if (m_skinnedUniforms.lightSpaceMatrix >= 0)
            glUniformMatrix4fv(m_skinnedUniforms.lightSpaceMatrix, 3, GL_FALSE, &m_cascadeLightSpaceMatrices[0][0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMapTexture);

        if (m_skinnedUniforms.brdfLUT >= 0 && m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
            glUniform1i(m_skinnedUniforms.brdfLUT, 4);
            glActiveTexture(GL_TEXTURE0);
        }

        auto frustumPlanes = ExtractFrustumPlanes(scene.camera.viewProjectionMatrix);

        for (const auto& cmd : scene.skinnedDrawCommands)
        {
            Vec3 worldPos = Vec3(cmd.worldMatrix * Vec4(cmd.boundsCenter, 1.0f));
            float cullRadius = (cmd.boundsRadius > 0.0f) ? cmd.boundsRadius : 5.0f;
            if (!IsSphereInFrustum(frustumPlanes, worldPos, cullRadius)) continue;

            if (m_skinnedUniforms.model >= 0)
                glUniformMatrix4fv(m_skinnedUniforms.model, 1, GL_FALSE, &cmd.worldMatrix[0][0]);

            if (m_skinnedUniforms.normalMatrix >= 0)
            {
                Mat3 normalMat = ComputeNormalMatrix(cmd.worldMatrix);
                glUniformMatrix3fv(m_skinnedUniforms.normalMatrix, 1, GL_FALSE, &normalMat[0][0]);
            }

            auto it = m_boneMatricesCache.find(cmd.boneMatricesHandle);
            if (it != m_boneMatricesCache.end())
            {
                UploadBoneMatricesToUBO(it->second);
            }

            SetMaterialUniforms(cmd.materialHandle, m_skinnedUniforms);

            const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.skeletalMeshHandle);
            if (mesh && mesh->vao)
            {
                glBindVertexArray(mesh->vao);
                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        m_textureManager.UnbindTexture(GL_TEXTURE2);
        m_textureManager.UnbindTexture(GL_TEXTURE1);
        m_textureManager.UnbindTexture(GL_TEXTURE0);
        if (m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void RenderTerrain(const RenderScene& scene)
    {
        GLuint pbrProg = m_shaderManager.GetProgram("pbr");
        if (!pbrProg || scene.terrainDrawCommands.empty()) return;
        glUseProgram(pbrProg);

        SetSceneUniforms(scene, m_pbrUniforms);

        if (m_pbrUniforms.shadowMap >= 0) glUniform1i(m_pbrUniforms.shadowMap, 2);
        if (m_pbrUniforms.lightSpaceMatrix >= 0)
            glUniformMatrix4fv(m_pbrUniforms.lightSpaceMatrix, 3, GL_FALSE, &m_cascadeLightSpaceMatrices[0][0][0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMapTexture);

        if (m_pbrUniforms.brdfLUT >= 0 && m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
            glUniform1i(m_pbrUniforms.brdfLUT, 4);
            glActiveTexture(GL_TEXTURE0);
        }

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

            const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.terrainHandle);
            if (mesh && mesh->vao)
            {
                glBindVertexArray(mesh->vao);
                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
                ++m_drawCallCount;
            }
        }

        m_textureManager.UnbindTexture(GL_TEXTURE2);
        m_textureManager.UnbindTexture(GL_TEXTURE1);
        m_textureManager.UnbindTexture(GL_TEXTURE0);
        if (m_brdfLUT)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        m_textureManager.UnbindTexture(GL_TEXTURE3);
        m_textureManager.UnbindTexture(GL_TEXTURE5);
        m_textureManager.UnbindTexture(GL_TEXTURE6);
        glUseProgram(0);
    }

    void RenderShadowMap(const RenderScene& scene)
    {
        GLuint shadowProg = m_shaderManager.GetProgram("shadow");
        if (!shadowProg) return;

        if (m_shadowMapFBO == 0)
            CreateShadowMap(2048, 2048);

        ComputeShadowCascades(scene);

        m_context.SetViewport(0, 0, m_shadowMapSize, m_shadowMapSize);

        for (int cascade = 0; cascade < SHADOW_CASCADE_COUNT; ++cascade)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadowMapTexture, 0, cascade);
            m_context.ClearDepth();

            glUseProgram(shadowProg);
            GLint lightSpaceLoc = glGetUniformLocation(shadowProg, "uLightSpaceMatrix");
            if (lightSpaceLoc >= 0)
                glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, &m_cascadeLightSpaceMatrices[cascade][0][0]);

            GLint modelLoc = glGetUniformLocation(shadowProg, "uModel");

            for (const auto& cmd : scene.staticDrawCommands)
            {
                if (modelLoc >= 0)
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);
                const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.meshHandle);
                if (!mesh) mesh = &m_defaultMesh;
                if (mesh && mesh->vao) m_bufferManager.DrawMesh(*mesh);
            }

            for (const auto& cmd : scene.terrainDrawCommands)
            {
                if (modelLoc >= 0)
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);
                const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.terrainHandle);
                if (mesh && mesh->vao) m_bufferManager.DrawMesh(*mesh);
            }

            GLuint shadowSkinnedProg = m_shaderManager.GetProgram("shadow_skinned");
            if (shadowSkinnedProg && !scene.skinnedDrawCommands.empty())
            {
                glUseProgram(shadowSkinnedProg);
                GLint skinnedLightSpaceLoc = glGetUniformLocation(shadowSkinnedProg, "uLightSpaceMatrix");
                if (skinnedLightSpaceLoc >= 0)
                    glUniformMatrix4fv(skinnedLightSpaceLoc, 1, GL_FALSE, &m_cascadeLightSpaceMatrices[cascade][0][0]);
                GLint skinnedModelLoc = glGetUniformLocation(shadowSkinnedProg, "uModel");

                for (const auto& cmd : scene.skinnedDrawCommands)
                {
                    if (skinnedModelLoc >= 0)
                        glUniformMatrix4fv(skinnedModelLoc, 1, GL_FALSE, &cmd.worldMatrix[0][0]);
                    auto boneIt = m_boneMatricesCache.find(cmd.boneMatricesHandle);
                    if (boneIt != m_boneMatricesCache.end())
                        UploadBoneMatricesToUBO(boneIt->second);
                    const GLMeshData* mesh = m_bufferManager.GetMesh(cmd.skeletalMeshHandle);
                    if (mesh && mesh->vao) m_bufferManager.DrawMesh(*mesh);
                }
            }
        }

        m_context.BindDefaultFramebuffer();
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        glUseProgram(0);
    }

    void ComputeShadowCascades(const RenderScene& scene)
    {
        float cascadeSplits[SHADOW_CASCADE_COUNT];
        float nearP = scene.camera.nearPlane;
        float farP = scene.camera.farPlane;
        float clipRange = farP - nearP;

        float cascadeLambda = 0.75f;
        for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i)
        {
            float p = static_cast<float>(i + 1) / static_cast<float>(SHADOW_CASCADE_COUNT);
            float logSplit = nearP * std::pow(farP / nearP, p);
            float linSplit = nearP + clipRange * p;
            cascadeSplits[i] = cascadeLambda * logSplit + (1.0f - cascadeLambda) * linSplit;
        }

        Mat4 invViewProj = glm::inverse(scene.camera.viewProjectionMatrix);

        Vec3 lightDir = glm::normalize(-scene.light.direction);

        for (int cascade = 0; cascade < SHADOW_CASCADE_COUNT; ++cascade)
        {
            float splitDist = cascadeSplits[cascade];
            float prevSplitDist = (cascade == 0) ? nearP : cascadeSplits[cascade - 1];

            Vec3 frustumCorners[8];
            int cornerIdx = 0;
            for (int z = 0; z < 2; ++z)
            {
                for (int y = -1; y <= 1; y += 2)
                {
                    for (int x = -1; x <= 1; x += 2)
                    {
                        Vec4 pt = invViewProj * Vec4(static_cast<float>(x), static_cast<float>(y),
                            (z == 0) ? -1.0f : 1.0f, 1.0f);
                        frustumCorners[cornerIdx++] = Vec3(pt) / pt.w;
                    }
                }
            }

            for (int i = 0; i < 4; ++i)
            {
                Vec3 ray = frustumCorners[i + 4] - frustumCorners[i];
                Vec3 nearCorner = frustumCorners[i] + ray * (prevSplitDist - nearP) / clipRange;
                Vec3 farCorner = frustumCorners[i] + ray * (splitDist - nearP) / clipRange;
                frustumCorners[i + 4] = farCorner;
                frustumCorners[i] = nearCorner;
            }

            Vec3 center(0.0f);
            for (int i = 0; i < 8; ++i)
                center += frustumCorners[i];
            center /= 8.0f;

            float radius = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                float dist = glm::length(frustumCorners[i] - center);
                radius = glm::max(radius, dist);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            Vec3 maxExtents(radius);
            Vec3 minExtents = -maxExtents;

            Vec3 upVec = (std::abs(glm::dot(glm::normalize(lightDir), Vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
                ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
            Mat4 lightView = glm::lookAt(center - lightDir * radius, center, upVec);
            Mat4 lightProj = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y,
                -maxExtents.z * 4.0f, maxExtents.z * 4.0f);
            m_cascadeLightSpaceMatrices[cascade] = lightProj * lightView;

            float d = splitDist;
            float z_ndc = (farP + nearP) / (farP - nearP) - 2.0f * farP * nearP / ((farP - nearP) * d);
            m_cascadeSplitDistances[cascade] = (z_ndc + 1.0f) * 0.5f;
        }
    }

    void RenderPostProcess()
    {
        GLuint ppProg = m_shaderManager.GetProgram("postprocess");
        if (!ppProg) return;

        if (!m_fxaaFBO)
            CreateFXAAFBO(m_windowWidth, m_windowHeight);

        m_bufferManager.CreateScreenQuad(m_screenQuadVAO, m_screenQuadVBO);

        m_context.BindFramebuffer(m_fxaaFBO);
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        m_context.ClearColor();
        m_context.EnableDepthTest(false);

        glUseProgram(ppProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(ppProg, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);

        bool useBloom = (m_bloomPingPongFBO[0] != 0);
        GLint useBloomLoc = glGetUniformLocation(ppProg, "uUseBloom");
        if (useBloomLoc >= 0) glUniform1i(useBloomLoc, useBloom ? 1 : 0);

        bool useSSAO = (m_ssaoBlurTexture != 0 && m_ssaoEnabled);
        GLint useSSAOLoc = glGetUniformLocation(ppProg, "uUseSSAO");
        if (useSSAOLoc >= 0) glUniform1i(useSSAOLoc, useSSAO ? 1 : 0);

        if (useSSAO)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_ssaoBlurTexture);
            GLint ssaoTexLoc = glGetUniformLocation(ppProg, "uSSAOTexture");
            if (ssaoTexLoc >= 0) glUniform1i(ssaoTexLoc, 2);
        }

        if (useBloom)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_bloomPingPongTexture[0]);
            GLint bloomTexLoc = glGetUniformLocation(ppProg, "uBloomTexture");
            if (bloomTexLoc >= 0) glUniform1i(bloomTexLoc, 1);
        }

        GLint exposureLoc = glGetUniformLocation(ppProg, "uExposure");
        if (exposureLoc >= 0) glUniform1f(exposureLoc, 1.0f);

        m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

        m_context.EnableDepthTest(true);
        glUseProgram(0);
    }

    void RenderDebugOverlay()
    {
        if (!m_showDebugOverlay) return;
        m_debugOverlay.Draw(m_windowWidth, m_windowHeight, m_drawCallCount, m_debugStats);
    }

    void RenderBloom()
    {
        GLuint extractProg = m_shaderManager.GetProgram("bloom_extract");
        GLuint blurProg = m_shaderManager.GetProgram("bloom_blur");
        if (!extractProg || !blurProg) return;
        if (!m_bloomExtractFBO) return;

        m_bufferManager.CreateScreenQuad(m_screenQuadVAO, m_screenQuadVBO);

        int bloomW = m_windowWidth / 2;
        int bloomH = m_windowHeight / 2;

        m_context.EnableDepthTest(false);

        glUseProgram(extractProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        GLint sceneTexLoc = glGetUniformLocation(extractProg, "uSceneTexture");
        if (sceneTexLoc >= 0) glUniform1i(sceneTexLoc, 0);
        GLint thresholdLoc = glGetUniformLocation(extractProg, "uThreshold");
        if (thresholdLoc >= 0) glUniform1f(thresholdLoc, 1.0f);

        m_context.BindFramebuffer(m_bloomExtractFBO);
        m_context.SetViewport(0, 0, bloomW, bloomH);
        m_context.ClearColor();
        m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

        glUseProgram(blurProg);
        GLint texLoc = glGetUniformLocation(blurProg, "uTexture");
        GLint texelSizeLoc = glGetUniformLocation(blurProg, "uTexelSize");
        GLint horizontalLoc = glGetUniformLocation(blurProg, "uHorizontal");
        if (texLoc >= 0) glUniform1i(texLoc, 0);

        bool horizontal = true;
        bool firstIteration = true;
        for (int i = 0; i < 10; ++i)
        {
            m_context.BindFramebuffer(m_bloomPingPongFBO[horizontal ? 1 : 0]);
            if (horizontalLoc >= 0) glUniform1i(horizontalLoc, horizontal ? 1 : 0);
            if (texelSizeLoc >= 0)
                glUniform2f(texelSizeLoc, 1.0f / static_cast<float>(bloomW), 1.0f / static_cast<float>(bloomH));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, firstIteration ? m_bloomExtractTexture : m_bloomPingPongTexture[horizontal ? 0 : 1]);
            m_context.SetViewport(0, 0, bloomW, bloomH);
            m_context.ClearColor();
            m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

            horizontal = !horizontal;
            firstIteration = false;
        }

        m_context.BindDefaultFramebuffer();
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        glUseProgram(0);
        m_context.EnableDepthTest(true);
    }

    void CreateDefaultMesh()
    {
        std::vector<float> cubeVerts = {
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        };

        std::vector<uint32_t> cubeIdx = {
            0,  1,  2,   2,  3,  0,
            4,  5,  6,   6,  7,  4,
            8,  9,  10,  10, 11, 8,
            12, 13, 14,  14, 15, 12,
            16, 17, 18,  18, 19, 16,
            20, 21, 22,  22, 23, 20,
        };

        m_defaultCubeHandle = UploadMesh(cubeVerts, cubeIdx, 12);
        auto* mesh = m_bufferManager.GetMesh(m_defaultCubeHandle);
        if (mesh) m_defaultMesh = *mesh;

        std::vector<float> planeVerts = {
            -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 50.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f, 50.0f, 50.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f, 50.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        };
        std::vector<uint32_t> planeIdx = { 0, 1, 2, 2, 3, 0 };
        m_defaultPlaneHandle = UploadMesh(planeVerts, planeIdx, 12);
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

    void CreateSceneFBO(int width, int height)
    {
        DestroySceneFBO();
        if (width <= 0 || height <= 0) return;

        glGenTextures(1, &m_sceneNormalTexture);
        glBindTexture(GL_TEXTURE_2D, m_sceneNormalTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &m_sceneColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_sceneDepthTexture = m_textureManager.CreateDepthTexture(width, height, GL_DEPTH_COMPONENT24);

        glGenFramebuffers(1, &m_sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneColorTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_sceneNormalTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepthTexture, 0);

        GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, drawBuffers);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            KE_LOG_ERROR("Scene FBO incomplete: {}", static_cast<int>(status));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        KE_LOG_INFO("Scene FBO created with normal buffer ({}x{})", width, height);
    }

    void DestroySceneFBO()
    {
        if (m_sceneFBO) { glDeleteFramebuffers(1, &m_sceneFBO); m_sceneFBO = 0; }
        if (m_sceneColorTexture) { glDeleteTextures(1, &m_sceneColorTexture); m_sceneColorTexture = 0; }
        if (m_sceneDepthTexture) { glDeleteTextures(1, &m_sceneDepthTexture); m_sceneDepthTexture = 0; }
        if (m_sceneNormalTexture) { glDeleteTextures(1, &m_sceneNormalTexture); m_sceneNormalTexture = 0; }
    }

    void CreateShadowMap(int width, int height)
    {
        DestroyShadowMap();
        m_shadowMapSize = width;

        glGenTextures(1, &m_shadowMapTexture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMapTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24,
            width, height, SHADOW_CASCADE_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        glGenFramebuffers(1, &m_shadowMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadowMapTexture, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DestroyShadowMap()
    {
        if (m_shadowMapFBO) { glDeleteFramebuffers(1, &m_shadowMapFBO); m_shadowMapFBO = 0; }
        if (m_shadowMapTexture) { glDeleteTextures(1, &m_shadowMapTexture); m_shadowMapTexture = 0; }
    }

    void CreateBloomFBOs(int width, int height)
    {
        DestroyBloomFBOs();
        if (width <= 0 || height <= 0) return;

        int bloomW = width / 2;
        int bloomH = height / 2;

        m_bloomExtractTexture = m_textureManager.CreateStandaloneTexture(bloomW, bloomH, GL_RGBA16F, GL_RGBA, GL_FLOAT);
        OpenGLFramebuffer fboCreator;
        m_bloomExtractFBO = fboCreator.CreateColorFBO(bloomW, bloomH, m_bloomExtractTexture);

        for (int i = 0; i < 2; ++i)
        {
            m_bloomPingPongTexture[i] = m_textureManager.CreateStandaloneTexture(bloomW, bloomH, GL_RGBA16F, GL_RGBA, GL_FLOAT);
            m_bloomPingPongFBO[i] = fboCreator.CreateColorFBO(bloomW, bloomH, m_bloomPingPongTexture[i]);
        }
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

    void CreateSSAO()
    {
        DestroySSAO();

        m_ssaoKernel.resize(64);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::mt19937 rng(42);

        for (int i = 0; i < 64; ++i)
        {
            Vec3 sample(
                dist(rng) * 2.0f - 1.0f,
                dist(rng) * 2.0f - 1.0f,
                dist(rng)
            );
            sample = glm::normalize(sample);
            float scale = static_cast<float>(i) / 64.0f;
            scale = 0.1f + scale * scale * 0.9f;
            sample *= scale;
            m_ssaoKernel[i] = sample;
        }

        std::vector<Vec3> noiseData(16);
        for (int i = 0; i < 16; ++i)
        {
            noiseData[i] = Vec3(
                dist(rng) * 2.0f - 1.0f,
                dist(rng) * 2.0f - 1.0f,
                0.0f
            );
        }

        glGenTextures(1, &m_ssaoNoiseTexture);
        glBindTexture(GL_TEXTURE_2D, m_ssaoNoiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noiseData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        CreateSSAOFBOs(m_windowWidth, m_windowHeight);
    }

    void CreateSSAOFBOs(int width, int height)
    {
        if (m_ssaoFBO) { glDeleteFramebuffers(1, &m_ssaoFBO); m_ssaoFBO = 0; }
        if (m_ssaoColorTexture) { glDeleteTextures(1, &m_ssaoColorTexture); m_ssaoColorTexture = 0; }
        if (m_ssaoBlurFBO) { glDeleteFramebuffers(1, &m_ssaoBlurFBO); m_ssaoBlurFBO = 0; }
        if (m_ssaoBlurTexture) { glDeleteTextures(1, &m_ssaoBlurTexture); m_ssaoBlurTexture = 0; }

        if (width <= 0 || height <= 0) return;

        glGenTextures(1, &m_ssaoColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_ssaoColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &m_ssaoFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenTextures(1, &m_ssaoBlurTexture);
        glBindTexture(GL_TEXTURE_2D, m_ssaoBlurTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &m_ssaoBlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoBlurTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DestroySSAO()
    {
        if (m_ssaoFBO) { glDeleteFramebuffers(1, &m_ssaoFBO); m_ssaoFBO = 0; }
        if (m_ssaoColorTexture) { glDeleteTextures(1, &m_ssaoColorTexture); m_ssaoColorTexture = 0; }
        if (m_ssaoBlurFBO) { glDeleteFramebuffers(1, &m_ssaoBlurFBO); m_ssaoBlurFBO = 0; }
        if (m_ssaoBlurTexture) { glDeleteTextures(1, &m_ssaoBlurTexture); m_ssaoBlurTexture = 0; }
        if (m_ssaoNoiseTexture) { glDeleteTextures(1, &m_ssaoNoiseTexture); m_ssaoNoiseTexture = 0; }
        m_ssaoKernel.clear();
    }

    void RenderSSAO(const RenderScene& scene)
    {
        GLuint ssaoProg = m_shaderManager.GetProgram("ssao");
        GLuint ssaoBlurProg = m_shaderManager.GetProgram("ssao_blur");
        if (!ssaoProg || !ssaoBlurProg || !m_ssaoFBO || !m_ssaoEnabled) return;

        m_bufferManager.CreateScreenQuad(m_screenQuadVAO, m_screenQuadVBO);
        m_context.EnableDepthTest(false);

        glUseProgram(ssaoProg);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_sceneDepthTexture);
        GLint depthLoc = glGetUniformLocation(ssaoProg, "uDepthTexture");
        if (depthLoc >= 0) glUniform1i(depthLoc, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_ssaoNoiseTexture);
        GLint noiseLoc = glGetUniformLocation(ssaoProg, "uNoiseTexture");
        if (noiseLoc >= 0) glUniform1i(noiseLoc, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_sceneNormalTexture);
        GLint normalLoc = glGetUniformLocation(ssaoProg, "uNormalTexture");
        if (normalLoc >= 0) glUniform1i(normalLoc, 2);

        Mat4 invVP = glm::inverse(scene.camera.viewProjectionMatrix);
        GLint invVPLoc = glGetUniformLocation(ssaoProg, "uInverseViewProjection");
        if (invVPLoc >= 0) glUniformMatrix4fv(invVPLoc, 1, GL_FALSE, &invVP[0][0]);

        GLint vpLoc = glGetUniformLocation(ssaoProg, "uViewProjection");
        if (vpLoc >= 0) glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &scene.camera.viewProjectionMatrix[0][0]);

        for (int i = 0; i < 64 && i < static_cast<int>(m_ssaoKernel.size()); ++i)
        {
            if (i < static_cast<int>(m_ssaoKernelLocs.size()) && m_ssaoKernelLocs[i] >= 0)
                glUniform3fv(m_ssaoKernelLocs[i], 1, &m_ssaoKernel[i][0]);
        }

        GLint screenSizeLoc = glGetUniformLocation(ssaoProg, "uScreenSize");
        if (screenSizeLoc >= 0) glUniform2f(screenSizeLoc, static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight));

        GLint radiusLoc = glGetUniformLocation(ssaoProg, "uRadius");
        if (radiusLoc >= 0) glUniform1f(radiusLoc, 0.5f);

        GLint biasLoc = glGetUniformLocation(ssaoProg, "uBias");
        if (biasLoc >= 0) glUniform1f(biasLoc, 0.025f);

        m_context.BindFramebuffer(m_ssaoFBO);
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        m_context.ClearColor();
        m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

        glUseProgram(ssaoBlurProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ssaoColorTexture);
        GLint ssaoTexLoc = glGetUniformLocation(ssaoBlurProg, "uSSAOTexture");
        if (ssaoTexLoc >= 0) glUniform1i(ssaoTexLoc, 0);

        m_context.BindFramebuffer(m_ssaoBlurFBO);
        m_context.ClearColor();
        m_bufferManager.DrawScreenQuad(m_screenQuadVAO);

        m_context.BindDefaultFramebuffer();
        m_context.SetViewport(0, 0, m_windowWidth, m_windowHeight);
        glUseProgram(0);
        m_context.EnableDepthTest(true);
    }

    void CreateBRDFLUT()
    {
        const int lutSize = 512;
        GLuint fbo, tex;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, lutSize, lutSize, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        const char* vertSrc = R"(
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

        const char* fragSrc = R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec2 FragColor;
            const float PI = 3.14159265359;
            float RadicalInverse_VdC(uint bits)
            {
                bits = (bits << 16u) | (bits >> 16u);
                bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
                bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
                bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
                bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
                return float(bits) * 2.3283064365386963e-10;
            }
            vec2 Hammersley(uint i, uint N)
            {
                return vec2(float(i) / float(N), RadicalInverse_VdC(i));
            }
            vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
            {
                float a = roughness * roughness;
                float phi = 2.0 * PI * Xi.x;
                float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
                float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
                vec3 H;
                H.x = cos(phi) * sinTheta;
                H.y = sin(phi) * sinTheta;
                H.z = cosTheta;
                vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
                vec3 tangent = normalize(cross(up, N));
                vec3 bitangent = cross(N, tangent);
                vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
                return normalize(sampleVec);
            }
            float GeometrySchlickGGX(float NdotV, float roughness)
            {
                float a = roughness;
                float k = (a * a) / 2.0;
                return NdotV / (NdotV * (1.0 - k) + k);
            }
            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
            {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                float ggx2 = GeometrySchlickGGX(NdotV, roughness);
                float ggx1 = GeometrySchlickGGX(NdotL, roughness);
                return ggx1 * ggx2;
            }
            vec2 IntegrateBRDF(float NdotV, float roughness)
            {
                vec3 V;
                V.x = sqrt(1.0 - NdotV * NdotV);
                V.y = 0.0;
                V.z = NdotV;
                float A = 0.0;
                float B = 0.0;
                vec3 N = vec3(0.0, 0.0, 1.0);
                const uint SAMPLE_COUNT = 1024u;
                for (uint i = 0u; i < SAMPLE_COUNT; ++i)
                {
                    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
                    vec3 H = ImportanceSampleGGX(Xi, N, roughness);
                    vec3 L = normalize(2.0 * dot(V, H) * H - V);
                    float NdotL = max(L.z, 0.0);
                    float NdotH = max(H.z, 0.0);
                    float VdotH = max(dot(V, H), 0.0);
                    if (NdotL > 0.0)
                    {
                        float G = GeometrySmith(N, V, L, roughness);
                        float G_Vis = (G * VdotH) / (NdotH * NdotV);
                        float Fc = pow(1.0 - VdotH, 5.0);
                        A += (1.0 - Fc) * G_Vis;
                        B += Fc * G_Vis;
                    }
                }
                return vec2(A, B) / float(SAMPLE_COUNT);
            }
            void main()
            {
                vec2 integratedBRDF = IntegrateBRDF(vTexCoord.x, vTexCoord.y);
                FragColor = integratedBRDF;
            }
        )";

        GLuint prog = m_shaderManager.CreateProgram(vertSrc, fragSrc);
        if (!prog) return;

        GLuint localQuadVAO = 0, localQuadVBO = 0;
        float quad[] = {
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
        };
        glGenVertexArrays(1, &localQuadVAO);
        glGenBuffers(1, &localQuadVBO);
        glBindVertexArray(localQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, localQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float)));

        glViewport(0, 0, lutSize, lutSize);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(localQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteProgram(prog);
        glDeleteVertexArrays(1, &localQuadVAO);
        glDeleteBuffers(1, &localQuadVBO);
        glDeleteFramebuffers(1, &fbo);

        glViewport(0, 0, m_windowWidth, m_windowHeight);
        m_brdfLUT = tex;
        KE_LOG_INFO("BRDF LUT generated ({}x{})", lutSize, lutSize);
    }

    GLuint CreateShadowShader()
    {
        const char* vertSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            uniform mat4 uLightSpaceMatrix;
            uniform mat4 uModel;
            void main()
            {
                gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
            }
        )";
        const char* fragSrc = R"(
            #version 450 core
            void main() {}
        )";
        GLuint prog = m_shaderManager.CreateProgram(vertSrc, fragSrc);
        if (prog) m_shaderManager.AddShader("shadow", prog);
        return prog;
    }

    GLuint CreateShadowSkinnedShader()
    {
        const char* vertSrc = R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 3) in vec4 aBoneWeights;
            layout(location = 4) in vec4 aBoneIndices;

            layout(std140, binding = 0) uniform BoneBlock
            {
                mat4 uBoneMatrices[128];
            };

            uniform mat4 uLightSpaceMatrix;
            uniform mat4 uModel;
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
        const char* fragSrc = R"(
            #version 450 core
            void main() {}
        )";
        GLuint prog = m_shaderManager.CreateProgram(vertSrc, fragSrc);
        if (prog) m_shaderManager.AddShader("shadow_skinned", prog);
        return prog;
    }

    void CreateBloomShaders()
    {
        const char* screenVertSrc = R"(
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
                float softThreshold = uThreshold * 0.5;
                float contribution = clamp(brightness - softThreshold, 0.0, uThreshold + softThreshold);
                contribution = smoothstep(0.0, uThreshold, contribution);
                float knee = 1.0 / max(uThreshold - softThreshold, 0.001);
                float curve = clamp((brightness - softThreshold) * knee, 0.0, 1.0);
                FragColor = vec4(color * curve, 1.0);
            }
        )";

        GLuint extractProg = m_shaderManager.CreateProgram(screenVertSrc, extractFrag);
        if (extractProg) m_shaderManager.AddShader("bloom_extract", extractProg);

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

        GLuint blurProg = m_shaderManager.CreateProgram(screenVertSrc, blurFrag);
        if (blurProg) m_shaderManager.AddShader("bloom_blur", blurProg);
    }

    void CreateBoneUBO()
    {
        glGenBuffers(1, &m_boneUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_boneUBO);
        GLsizeiptr bufferSize = 128 * sizeof(Mat4);
        glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_boneUBO);
    }

    void UploadBoneMatricesToUBO(const std::vector<Mat4>& matrices)
    {
        if (!m_boneUBO || matrices.empty()) return;
        GLsizei boneCount = static_cast<GLsizei>(std::min(matrices.size(), static_cast<size_t>(128)));
        GLsizeiptr dataSize = boneCount * sizeof(Mat4);
        constexpr GLsizeiptr kUBOSize = 128 * sizeof(Mat4);
        glBindBuffer(GL_UNIFORM_BUFFER, m_boneUBO);
        if (dataSize > kUBOSize)
        {
            KE_LOG_ERROR("Bone matrix data exceeds UBO size: {} > {}", dataSize, kUBOSize);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            return;
        }
        glBufferSubData(GL_UNIFORM_BUFFER, 0, dataSize, matrices.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void CreateSSAOShaders()
    {
        const char* screenVertSrc = R"(
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

        const char* ssaoFrag = R"(
            #version 450 core
            in vec2 vTexCoord;
            out float FragColor;

            uniform sampler2D uDepthTexture;
            uniform sampler2D uNoiseTexture;
            uniform sampler2D uNormalTexture;
            uniform mat4 uViewProjection;
            uniform mat4 uInverseViewProjection;
            uniform vec3 uKernel[64];
            uniform vec2 uScreenSize;
            uniform float uRadius;
            uniform float uBias;

            vec3 ReconstructWorldPos(vec2 uv, float depth)
            {
                vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
                vec4 worldPos = uInverseViewProjection * ndc;
                return worldPos.xyz / worldPos.w;
            }

            void main()
            {
                float depth = texture(uDepthTexture, vTexCoord).r;
                if (depth >= 1.0) { FragColor = 1.0; return; }

                vec3 fragPos = ReconstructWorldPos(vTexCoord, depth);

                vec2 encodedNormal = texture(uNormalTexture, vTexCoord).rg;
                vec3 normal = normalize(vec3(encodedNormal * 2.0 - 1.0, 0.0));
                float nzSq = 1.0 - dot(normal.xy, normal.xy);
                normal.z = (nzSq > 0.0) ? sqrt(nzSq) : 0.0;

                vec2 noiseScale = uScreenSize / 4.0;
                vec3 randomVec = texture(uNoiseTexture, vTexCoord * noiseScale).xyz;

                vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
                vec3 bitangent = cross(normal, tangent);
                mat3 TBN = mat3(tangent, bitangent, normal);

                float occlusion = 0.0;
                int validSamples = 0;
                for (int i = 0; i < 64; ++i)
                {
                    vec3 sampleDir = TBN * uKernel[i];
                    vec3 samplePos = fragPos + sampleDir * uRadius;

                    vec4 sampleNDC = uViewProjection * vec4(samplePos, 1.0);
                    sampleNDC /= sampleNDC.w;
                    vec2 sampleUV = sampleNDC.xy * 0.5 + 0.5;

                    if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                        continue;

                    float sampleDepth = texture(uDepthTexture, sampleUV).r;
                    vec3 sampleWorldPos = ReconstructWorldPos(sampleUV, sampleDepth);

                    vec3 sampleDiff = sampleWorldPos - fragPos;
                    float dist = length(sampleDiff);
                    float rangeCheck = smoothstep(0.0, 1.0, uRadius / max(dist, 0.001));
                    occlusion += (dist <= uRadius + uBias ? 1.0 : 0.0) * rangeCheck;
                    validSamples++;
                }

                FragColor = 1.0 - (occlusion / max(validSamples, 1));
            }
        )";

        GLuint ssaoProg = m_shaderManager.CreateProgram(screenVertSrc, ssaoFrag);
        if (ssaoProg)
        {
            m_shaderManager.AddShader("ssao", ssaoProg);
            m_ssaoKernelLocs.clear();
            m_ssaoKernelLocs.reserve(64);
            for (int i = 0; i < 64; ++i)
            {
                std::string uname = "uKernel[" + std::to_string(i) + "]";
                m_ssaoKernelLocs.push_back(glGetUniformLocation(ssaoProg, uname.c_str()));
            }
        }

        const char* ssaoBlurFrag = R"(
            #version 450 core
            in vec2 vTexCoord;
            out float FragColor;

            uniform sampler2D uSSAOTexture;

            void main()
            {
                vec2 texelSize = 1.0 / vec2(textureSize(uSSAOTexture, 0));
                float result = 0.0;
                for (int x = -2; x <= 2; ++x)
                {
                    for (int y = -2; y <= 2; ++y)
                    {
                        vec2 offset = vec2(float(x), float(y)) * texelSize;
                        result += texture(uSSAOTexture, vTexCoord + offset).r;
                    }
                }
                FragColor = result / 25.0;
            }
        )";

        GLuint ssaoBlurProg = m_shaderManager.CreateProgram(screenVertSrc, ssaoBlurFrag);
        if (ssaoBlurProg) m_shaderManager.AddShader("ssao_blur", ssaoBlurProg);
    }

    static const char* PBRVertexSrc()
    {
        return R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            layout(location = 5) in vec4 aTangent;

            uniform mat4 uViewProjection;
            uniform mat4 uModel;
            uniform mat3 uNormalMatrix;

            out vec3 vWorldPos;
            out vec3 vNormal;
            out vec2 vTexCoord;
            out vec4 vTangent;

            void main()
            {
                vec4 worldPos = uModel * vec4(aPosition, 1.0);
                vWorldPos = worldPos.xyz;
                vNormal = uNormalMatrix * aNormal;
                vTexCoord = aTexCoord;
                vTangent = vec4(uNormalMatrix * aTangent.xyz, aTangent.w);
                gl_Position = uViewProjection * worldPos;
            }
        )";
    }

    static const char* PBRFragmentSrc()
    {
        return R"(
            #version 450 core
            in vec3 vWorldPos;
            in vec3 vNormal;
            in vec2 vTexCoord;
            in vec4 vTangent;

            uniform vec3 uCameraPos;
            uniform vec3 uLightDirection;
            uniform vec3 uLightColor;
            uniform float uLightIntensity;
            uniform vec3 uAmbientColor;
            uniform vec3 uAmbientGroundColor;
            uniform float uAmbientGroundBlend;

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
            uniform bool uUseEmissiveTexture;
            uniform sampler2D uEmissiveTexture;
            uniform bool uUseAOTexture;
            uniform sampler2D uAOTexture;

            uniform mat4 uLightSpaceMatrix[3];
            uniform sampler2DArray uShadowMap;
            uniform vec3 uCascadeSplitDistances;

            uniform mat4 uViewProjection;
            uniform float uNearPlane;
            uniform float uFarPlane;

            uniform sampler2D uBRDFLUT;

            uniform int uPointLightCount;
            uniform vec3 uPointLightPositions[4];
            uniform vec3 uPointLightColors[4];
            uniform float uPointLightRanges[4];
            uniform float uPointLightIntensities[4];

            layout(location = 0) out vec4 FragColor;
            layout(location = 1) out vec2 NormalOutput;

            const float PI = 3.14159265359;
            const int SHADOW_CASCADE_COUNT = 3;

            float LinearizeDepth(float depth, float nearP, float farP)
            {
                return (2.0 * nearP * farP) / (farP + nearP - depth * (farP - nearP));
            }

            float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
            {
                vec4 clipPos = uViewProjection * vec4(fragPos, 1.0);
                float ndcDepth = clipPos.z / clipPos.w;
                float viewDepth = LinearizeDepth(ndcDepth, uNearPlane, uFarPlane);
                float linearDepth = (ndcDepth + 1.0) * 0.5;

                int cascadeIndex = SHADOW_CASCADE_COUNT - 1;
                for (int i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i)
                {
                    if (linearDepth < uCascadeSplitDistances[i])
                    {
                        cascadeIndex = i;
                        break;
                    }
                }

                vec4 fragPosLightSpace = uLightSpaceMatrix[cascadeIndex] * vec4(fragPos, 1.0);
                vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
                projCoords = projCoords * 0.5 + 0.5;

                float currentDepth = projCoords.z;
                if (currentDepth > 1.0) return 0.0;

                float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
                bias *= 1.0 / (max(uCascadeSplitDistances[cascadeIndex], 0.01) * 100.0 + 1.0);

                const vec2 poissonDisk[16] = vec2[](
                    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
                    vec2(-0.09418410, -0.92938870), vec2(0.34495938, 0.29387760),
                    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
                    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
                    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
                    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
                    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
                    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
                );

                float shadow = 0.0;
                vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
                float spreadRadius = 1.5;
                for (int i = 0; i < 16; ++i)
                {
                    vec2 offset = poissonDisk[i] * texelSize * spreadRadius;
                    float pcfDepth = texture(uShadowMap, vec3(projCoords.xy + offset, cascadeIndex)).r;
                    shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                }
                shadow /= 16.0;

                float nextSplit = uCascadeSplitDistances[min(cascadeIndex + 1, SHADOW_CASCADE_COUNT - 1)];
                float splitSize = uCascadeSplitDistances[cascadeIndex];
                float fadeRange = (nextSplit - splitSize) * 0.3;
                if (fadeRange > 0.001 && linearDepth > splitSize - fadeRange)
                {
                    float fade = (linearDepth - (splitSize - fadeRange)) / fadeRange;
                    shadow *= (1.0 - smoothstep(0.0, 1.0, fade));
                    if (cascadeIndex < SHADOW_CASCADE_COUNT - 1)
                    {
                        vec4 nextLightSpace = uLightSpaceMatrix[cascadeIndex + 1] * vec4(fragPos, 1.0);
                        vec3 nextProj = nextLightSpace.xyz / nextLightSpace.w;
                        nextProj = nextProj * 0.5 + 0.5;
                        float nextShadow = 0.0;
                        for (int i = 0; i < 16; ++i)
                        {
                            vec2 offset = poissonDisk[i] * texelSize * spreadRadius;
                            float pcf = texture(uShadowMap, vec3(nextProj.xy + offset, cascadeIndex + 1)).r;
                            nextShadow += nextProj.z - bias > pcf ? 1.0 : 0.0;
                        }
                        nextShadow /= 16.0;
                        shadow += smoothstep(0.0, 1.0, fade) * nextShadow;
                    }
                }

                return shadow;
            }

            vec3 GetNormal()
            {
                vec3 N = normalize(vNormal);
                if (uUseNormalTexture)
                {
                    vec3 dpdx = dFdx(vWorldPos);
                    vec3 dpdy = dFdy(vWorldPos);
                    vec2 duvdx = dFdx(vTexCoord);
                    vec2 duvdy = dFdy(vTexCoord);
                    vec3 dp2perp = cross(dpdy, N);
                    vec3 dp1perp = cross(N, dpdx);
                    float det = dot(dpdx, dp2perp);
                    float invDet = 1.0 / (abs(det) + 1e-8);
                    vec3 dpdu = (dp2perp * duvdx.x + dp1perp * duvdy.x) * invDet;
                    vec3 dpdv = (dp2perp * duvdx.y + dp1perp * duvdy.y) * invDet;
                    float signDet = sign(det);
                    vec3 T = normalize(dpdu * signDet);
                    vec3 B = normalize(cross(N, T));
                    float handedness = dot(cross(dpdu, dpdv), N) * signDet;
                    if (handedness < 0.0) B = -B;
                    vec3 tangentNormal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
                    N = normalize(T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z);
                }
                else if (vTangent.w != 0.0)
                {
                    vec3 T = normalize(vTangent.xyz);
                    vec3 B = cross(N, T) * vTangent.w;
                    N = normalize(N);
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

            vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
            {
                return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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
                float metallic = clamp(uMetallic, 0.0, 1.0);
                float roughness = uRoughness;
                float ao = uAO;

                if (uUseAOTexture)
                    ao *= texture(uAOTexture, vTexCoord).r;

                if (uUseMetallicRoughnessTexture)
                {
                    vec3 mrSample = texture(uMetallicRoughnessTexture, vTexCoord).rgb;
                    roughness = mrSample.g * roughness;
                    metallic = mrSample.b * metallic;
                }

                roughness = clamp(roughness, 0.01, 1.0);

                float specularAO = sqrt(clamp(ao, 0.0, 1.0));

                vec3 N = GetNormal();
                vec3 V = normalize(uCameraPos - vWorldPos);
                float NdotV = max(dot(N, V), 0.0);

                float shadow = ShadowCalculation(vWorldPos, N, normalize(-uLightDirection));

                vec3 L = normalize(-uLightDirection);
                vec3 radiance = uLightColor * uLightIntensity;
                vec3 Lo = CalcLighting(L, radiance, N, V, albedo, shadow);

                for (int i = 0; i < uPointLightCount && i < 4; ++i)
                {
                    vec3 lightVec = uPointLightPositions[i] - vWorldPos;
                    float dist = length(lightVec);
                    if (dist > uPointLightRanges[i]) continue;
                    L = lightVec / dist;
                    float distSq = dist * dist;
                    float cutoffRadius = uPointLightRanges[i];
                    float constant = 1.0;
                    float linear = 2.0 / cutoffRadius;
                    float quadratic = 1.0 / (cutoffRadius * cutoffRadius);
                    float attenuation = 1.0 / (constant + linear * dist + quadratic * distSq);
                    float cutoffFade = 1.0 - smoothstep(cutoffRadius * 0.7, cutoffRadius, dist);
                    attenuation *= cutoffFade * cutoffFade;
                    radiance = uPointLightColors[i] * uPointLightIntensities[i] * attenuation;
                    Lo += CalcLighting(L, radiance, N, V, albedo, 0.0);
                }

                vec3 F0 = mix(vec3(0.04), albedo, metallic);
                vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
                vec3 kD = (vec3(1.0 - F) * (1.0 - metallic));

                float hemisphere = 0.5 + 0.5 * N.y;

                vec3 skyUp = uAmbientColor * 1.6;
                vec3 skyHorizon = uAmbientColor * 1.0;
                vec3 skyDown = uAmbientGroundColor * 0.6;
                vec3 irradiance;
                float wrappedHemi = clamp(hemisphere + 0.2, 0.0, 1.0);
                if (wrappedHemi > 0.5)
                {
                    float t = (wrappedHemi - 0.5) * 2.0;
                    irradiance = mix(skyHorizon, skyUp, t * t);
                }
                else
                {
                    float t = wrappedHemi * 2.0;
                    irradiance = mix(skyDown, skyHorizon, t);
                }

                float horizonOcclusion = smoothstep(-0.1, 0.3, N.y);
                irradiance *= mix(0.6, 1.0, horizonOcclusion);

                float backLight = max(dot(N, normalize(-uLightDirection)), 0.0);
                irradiance += uAmbientColor * 0.12 * backLight * backLight;
                irradiance += uAmbientColor * 0.06;

                vec3 diffuseAmbient = irradiance * kD * albedo * ao;

                vec2 brdf = texture(uBRDFLUT, vec2(NdotV, roughness)).rg;
                vec3 Fss = F * brdf.x + vec3(brdf.y);
                float Ess = brdf.x + brdf.y;
                vec3 Fms = Fss * Fss / (vec3(1.0) - Fss * (1.0 - Ess) + 1e-5);
                vec3 multiScatterContrib = Fms * irradiance * ao;
                vec3 singleScatterContrib = specularAO * Fss * irradiance;
                vec3 specularAmbient = singleScatterContrib + multiScatterContrib * 0.5;
                vec3 ambient = diffuseAmbient + specularAmbient;

                vec3 emissiveContrib = uEmissive * uEmissiveStrength;
                if (uUseEmissiveTexture)
                    emissiveContrib *= texture(uEmissiveTexture, vTexCoord).rgb;
                vec3 color = ambient + Lo + emissiveContrib;

                FragColor = vec4(color, 1.0);
                NormalOutput = N.xy * 0.5 + 0.5;
            }
        )";
    }

    static const char* SkinnedVertexSrc()
    {
        return R"(
            #version 450 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            layout(location = 3) in vec4 aBoneWeights;
            layout(location = 4) in vec4 aBoneIndices;

            layout(std140, binding = 0) uniform BoneBlock
            {
                mat4 uBoneMatrices[128];
            };

            uniform mat4 uViewProjection;
            uniform mat4 uModel;
            uniform mat3 uNormalMatrix;

            out vec3 vWorldPos;
            out vec3 vNormal;
            out vec2 vTexCoord;
            out vec4 vTangent;

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
                vec3 skinnedNormal = mat3(skinMatrix) * aNormal;

                vec4 worldPos = uModel * skinnedPos;
                vWorldPos = worldPos.xyz;
                vNormal = normalize(uNormalMatrix * skinnedNormal);
                vTexCoord = aTexCoord;

                vec3 tangent = normalize(cross(vNormal, vec3(0.0, 1.0, 0.0)));
                float dotCheck = abs(dot(vNormal, vec3(0.0, 1.0, 0.0)));
                if (dotCheck > 0.999)
                    tangent = normalize(cross(vNormal, vec3(1.0, 0.0, 0.0)));
                vTangent = vec4(tangent, 1.0);

                gl_Position = uViewProjection * worldPos;
            }
        )";
    }

    static const char* SkinnedFragmentSrc()
    {
        return PBRFragmentSrc();
    }

    static const char* PostProcessVertexSrc()
    {
        return R"(
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
    }

    static const char* PostProcessFragmentSrc()
    {
        return R"(
            #version 450 core
            in vec2 vTexCoord;
            out vec4 FragColor;

            uniform sampler2D uSceneTexture;
            uniform sampler2D uBloomTexture;
            uniform sampler2D uSSAOTexture;
            uniform float uExposure;
            uniform bool uUseBloom;
            uniform bool uUseSSAO;

            vec3 ACESFilm(vec3 x)
            {
                float a = 2.51;
                float b = 0.03;
                float c = 2.43;
                float d = 0.59;
                float e = 0.14;
                return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
            }

            void main()
            {
                vec3 color = texture(uSceneTexture, vTexCoord).rgb;

                if (uUseSSAO)
                {
                    float ssao = texture(uSSAOTexture, vTexCoord).r;
                    color = mix(color, color * ssao, 0.6);
                }

                if (uUseBloom)
                {
                    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
                    color += bloomColor;
                }

                color *= uExposure;
                color = ACESFilm(color);
                color = pow(color, vec3(1.0 / 2.2));

                float vignetteStrength = 0.35;
                vec2 vignetteCenter = vTexCoord - 0.5;
                float vignetteDist = dot(vignetteCenter, vignetteCenter);
                float vignette = 1.0 - vignetteStrength * vignetteDist * 2.0;
                vignette = clamp(vignette, 0.0, 1.0);
                vignette = smoothstep(0.0, 1.0, vignette);
                color *= vignette;

                FragColor = vec4(color, 1.0);
            }
        )";
    }

private:
    OpenGLContext m_context;
    OpenGLShader m_shaderManager;
    OpenGLBuffer m_bufferManager;
    OpenGLTexture m_textureManager;
    std::unordered_map<AssetHandle, GLMaterialData> m_materialCache;

    UniformLocations m_pbrUniforms{};
    UniformLocations m_skinnedUniforms{};
    GLMeshData m_defaultMesh{};
    AssetHandle m_defaultCubeHandle = INVALID_HANDLE;
    AssetHandle m_defaultPlaneHandle = INVALID_HANDLE;
    GLuint m_defaultAlbedoTexture = 0;
    GLuint m_defaultNormalTexture = 0;
    AssetHandle m_defaultAlbedoTextureHandle = INVALID_HANDLE;
    AssetHandle m_nextHandle = 0;
    uint32_t m_drawCallCount = 0;

    GLuint m_sceneFBO = 0;
    GLuint m_sceneColorTexture = 0;
    GLuint m_sceneDepthTexture = 0;
    GLuint m_sceneNormalTexture = 0;
    GLuint m_shadowMapFBO = 0;
    GLuint m_shadowMapTexture = 0;
    int m_shadowMapSize = 2048;
    GLuint m_screenQuadVAO = 0;
    GLuint m_screenQuadVBO = 0;
    static constexpr int SHADOW_CASCADE_COUNT = 3;
    Mat4 m_cascadeLightSpaceMatrices[3] = {};
    float m_cascadeSplitDistances[3] = {};
    GLint m_cascadeSplitLoc = -1;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    GLuint m_bloomExtractFBO = 0;
    GLuint m_bloomExtractTexture = 0;
    GLuint m_bloomPingPongFBO[2] = {0, 0};
    GLuint m_bloomPingPongTexture[2] = {0, 0};
    GLuint m_brdfLUT = 0;

    GLuint m_ssaoFBO = 0;
    GLuint m_ssaoColorTexture = 0;
    GLuint m_ssaoBlurFBO = 0;
    GLuint m_ssaoBlurTexture = 0;
    GLuint m_ssaoNoiseTexture = 0;
    std::vector<Vec3> m_ssaoKernel;
    bool m_ssaoEnabled = true;

    bool m_showDebugOverlay = false;
    DebugOverlay m_debugOverlay;
    RenderStats m_debugStats{};

    GLuint m_fxaaFBO = 0;
    GLuint m_fxaaColorTexture = 0;
    bool m_fxaaEnabled = true;

    std::vector<GLint> m_ssaoKernelLocs;
};
}
