#pragma once

#include <glad/gl.h>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <stb_image.h>
#include "Renderer/RenderScene.h"
#include "Math/Transform.h"
#include "Core/Log.h"

namespace Kojeom
{
struct GLTextureData
{
    GLuint texture = 0;
    int width = 0;
    int height = 0;
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

class OpenGLTexture
{
public:
    OpenGLTexture() = default;
    ~OpenGLTexture() { ClearAll(); }

    AssetHandle UploadTexture(int width, int height, int channels, const uint8_t* data, AssetHandle preassignedHandle = INVALID_HANDLE, bool sRGB = false)
    {
        if (width <= 0 || height <= 0 || !data) return INVALID_HANDLE;

        AssetHandle handle = (preassignedHandle != INVALID_HANDLE) ? preassignedHandle : GenerateHandle();

        GLTextureData tex{};
        tex.width = width;
        tex.height = height;

        glGenTextures(1, &tex.texture);
        glBindTexture(GL_TEXTURE_2D, tex.texture);

        GLenum format = GL_RGB;
        GLenum internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
        if (channels == 4)
        {
            format = GL_RGBA;
            internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        m_textureCache[handle] = tex;
        return handle;
    }

    GLuint CreateStandaloneTexture(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint CreateDepthTexture(int width, int height, GLenum internalFormat)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint CreateShadowMapTexture(int width, int height)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    const GLTextureData* GetTexture(AssetHandle handle) const
    {
        auto it = m_textureCache.find(handle);
        return (it != m_textureCache.end()) ? &it->second : nullptr;
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

    void ClearAll()
    {
        for (auto& [handle, tex] : m_textureCache)
        {
            if (tex.texture) glDeleteTextures(1, &tex.texture);
        }
        m_textureCache.clear();
    }

    void BindTexture(GLuint texture, GLenum unit) const
    {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    void UnbindTexture(GLenum unit) const
    {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const std::unordered_map<AssetHandle, GLTextureData>& GetTextureCache() const { return m_textureCache; }

private:
    AssetHandle GenerateHandle() { return ++m_nextHandle; }

    std::unordered_map<AssetHandle, GLTextureData> m_textureCache;
    AssetHandle m_nextHandle = 0;
};
}
