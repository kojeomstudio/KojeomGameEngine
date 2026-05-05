#pragma once

#include "Renderer/RenderScene.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace Kojeom
{
class IRendererBackend
{
public:
    virtual ~IRendererBackend() = default;
    virtual bool Initialize(void* windowHandle) = 0;
    virtual void Shutdown() = 0;
    virtual void OnResize(int width, int height) = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Render(const RenderScene& scene) = 0;
    virtual std::string GetName() const = 0;

    virtual AssetHandle UploadMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices, int vertexStride = 8) = 0;
    virtual AssetHandle UploadSkinnedMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices) = 0;
    virtual AssetHandle UploadTexture(int width, int height, int channels,
        const uint8_t* data) = 0;
    virtual AssetHandle RegisterMaterial(const Vec3& albedo, float metallic,
        float roughness, AssetHandle albedoTexHandle = INVALID_HANDLE,
        bool hasTex = false, AssetHandle normalTexHandle = INVALID_HANDLE,
        bool hasNormalTex = false, const Vec3& emissiveColor = Vec3(0.0f),
        float emissiveStr = 1.0f) = 0;
    virtual void UploadBoneMatrices(AssetHandle handle,
        const std::vector<Mat4>& matrices) = 0;
    virtual void RemoveMesh(AssetHandle handle) = 0;
    virtual void RemoveTexture(AssetHandle handle) = 0;
    virtual bool SaveScreenshot(const std::string& path, int width, int height) = 0;
    virtual AssetHandle GetDefaultCubeHandle() const = 0;
    virtual AssetHandle GetDefaultPlaneHandle() const = 0;
    virtual AssetHandle GetDefaultAlbedoTextureHandle() const = 0;
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() { Shutdown(); }

    bool Initialize(std::unique_ptr<IRendererBackend> backend, void* windowHandle)
    {
        m_backend = std::move(backend);
        if (!m_backend->Initialize(windowHandle))
        {
            m_backend.reset();
            return false;
        }
        return true;
    }

    void Shutdown()
    {
        if (m_backend)
        {
            m_backend->Shutdown();
            m_backend.reset();
        }
    }

    void OnResize(int width, int height)
    {
        if (m_backend) m_backend->OnResize(width, height);
    }

    void Render(const RenderScene& scene)
    {
        if (!m_backend) return;
        m_backend->BeginFrame();
        m_backend->Render(scene);
        m_backend->EndFrame();
    }

    AssetHandle UploadMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices, int vertexStride = 8)
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->UploadMesh(vertices, indices, vertexStride);
    }

    AssetHandle UploadSkinnedMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices)
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->UploadSkinnedMesh(vertices, indices);
    }

    AssetHandle UploadTexture(int width, int height, int channels,
        const uint8_t* data)
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->UploadTexture(width, height, channels, data);
    }

    AssetHandle RegisterMaterial(const Vec3& albedo, float metallic,
        float roughness, AssetHandle albedoTexHandle = INVALID_HANDLE,
        bool hasTex = false, AssetHandle normalTexHandle = INVALID_HANDLE,
        bool hasNormalTex = false, const Vec3& emissiveColor = Vec3(0.0f),
        float emissiveStr = 1.0f)
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->RegisterMaterial(albedo, metallic, roughness,
            albedoTexHandle, hasTex, normalTexHandle, hasNormalTex,
            emissiveColor, emissiveStr);
    }

    void UploadBoneMatrices(AssetHandle handle,
        const std::vector<Mat4>& matrices)
    {
        if (m_backend) m_backend->UploadBoneMatrices(handle, matrices);
    }

    void RemoveMesh(AssetHandle handle)
    {
        if (m_backend) m_backend->RemoveMesh(handle);
    }

    void RemoveTexture(AssetHandle handle)
    {
        if (m_backend) m_backend->RemoveTexture(handle);
    }

    bool SaveScreenshot(const std::string& path, int width, int height)
    {
        if (!m_backend) return false;
        return m_backend->SaveScreenshot(path, width, height);
    }

    AssetHandle GetDefaultCubeHandle() const
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->GetDefaultCubeHandle();
    }

    AssetHandle GetDefaultPlaneHandle() const
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->GetDefaultPlaneHandle();
    }

    AssetHandle GetDefaultAlbedoTextureHandle() const
    {
        if (!m_backend) return INVALID_HANDLE;
        return m_backend->GetDefaultAlbedoTextureHandle();
    }

    IRendererBackend* GetBackend() { return m_backend.get(); }

private:
    std::unique_ptr<IRendererBackend> m_backend;
};
}
