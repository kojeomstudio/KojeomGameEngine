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

struct RenderStats
{
    float frameTimeMs = 0.0f;
    float fps = 0.0f;
    int entityCount = 0;
    int loadedAssets = 0;
};

class DebugOverlay
{
public:
    void Draw(int screenWidth, int screenHeight, uint32_t drawCallCount, const RenderStats& stats)
    {
        if (m_overlayFBO == 0 || m_screenWidth != screenWidth || m_screenHeight != screenHeight)
        {
            RecreateResources(screenWidth, screenHeight);
        }

        GLint prevFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_overlayFBO);
        glBlitFramebuffer(0, 0, screenWidth, screenHeight,
            0, 0, screenWidth, screenHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

        DrawBackground(screenWidth, screenHeight);
        DrawStatsText(screenWidth, screenHeight, drawCallCount, stats);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_overlayFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFBO);
        glBlitFramebuffer(0, 0, screenWidth, screenHeight,
            0, 0, screenWidth, screenHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    }

    void RecreateResources(int width, int height)
    {
        if (m_overlayFBO) { glDeleteFramebuffers(1, &m_overlayFBO); m_overlayFBO = 0; }
        if (m_overlayTexture) { glDeleteTextures(1, &m_overlayTexture); m_overlayTexture = 0; }

        m_screenWidth = width;
        m_screenHeight = height;

        glGenTextures(1, &m_overlayTexture);
        glBindTexture(GL_TEXTURE_2D, m_overlayTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenFramebuffers(1, &m_overlayFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_overlayFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_overlayTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~DebugOverlay()
    {
        if (m_overlayFBO) glDeleteFramebuffers(1, &m_overlayFBO);
        if (m_overlayTexture) glDeleteTextures(1, &m_overlayTexture);
        if (m_bgVAO) glDeleteVertexArrays(1, &m_bgVAO);
        if (m_bgVBO) glDeleteBuffers(1, &m_bgVBO);
        if (m_bgProgram) glDeleteProgram(m_bgProgram);
        if (m_charProgram) glDeleteProgram(m_charProgram);
    }

private:
    void DrawBackground(int screenWidth, int screenHeight)
    {
        if (!m_bgVAO)
        {
            float bg[] = { 0.0f,1.0f, 0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f };
            glGenVertexArrays(1, &m_bgVAO);
            glGenBuffers(1, &m_bgVBO);
            glBindVertexArray(m_bgVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_bgVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
            glBindVertexArray(0);
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(GetBgProgram());
        GLint loc = glGetUniformLocation(GetBgProgram(), "uRect");
        float panelW = 280.0f;
        float panelH = 110.0f;
        float l = -1.0f;
        float r = -1.0f + 2.0f * (panelW / screenWidth);
        float b = 1.0f - 2.0f * (panelH / screenHeight);
        float t = 1.0f;
        glUniform4f(loc, l, b, r, t);
        glBindVertexArray(m_bgVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void DrawStatsText(int screenWidth, int screenHeight, uint32_t drawCallCount, const RenderStats& stats)
    {
        DrawDigitString(10, screenHeight - 25, std::to_string(static_cast<int>(stats.fps)), screenWidth, screenHeight);
        DrawDigitString(70, screenHeight - 25, "FPS", screenWidth, screenHeight);
        DrawDigitString(10, screenHeight - 45, std::to_string(static_cast<int>(stats.frameTimeMs * 100.0f) / 100.0f).substr(0, 5) + "ms", screenWidth, screenHeight);
        DrawDigitString(10, screenHeight - 65, "DC:" + std::to_string(drawCallCount), screenWidth, screenHeight);
        DrawDigitString(10, screenHeight - 85, "E:" + std::to_string(stats.entityCount), screenWidth, screenHeight);
        DrawDigitString(10, screenHeight - 105, "A:" + std::to_string(stats.loadedAssets), screenWidth, screenHeight);
    }

    void DrawDigitString(float x, float y, const std::string& text, int screenWidth, int screenHeight)
    {
        for (size_t i = 0; i < text.size(); ++i)
        {
            int code = GetCharCode(text[i]);
            if (code >= 0)
                DrawBitmapChar(x + i * 10.0f, y, code, screenWidth, screenHeight);
        }
    }

    int GetCharCode(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
        if (c == '.') return 36;
        if (c == ':') return 37;
        if (c == ' ') return -1;
        return -1;
    }

    void DrawBitmapChar(float x, float y, int charCode, int screenWidth, int screenHeight)
    {
        if (charCode < 0 || charCode >= 38) return;

        const int charW = 8;
        const int charH = 14;
        float scaleX = 2.0f * charW / static_cast<float>(screenWidth);
        float scaleY = 2.0f * charH / static_cast<float>(screenHeight);
        float ndcX = -1.0f + 2.0f * x / static_cast<float>(screenWidth);
        float ndcY = 1.0f - 2.0f * y / static_cast<float>(screenHeight);

        uint32_t bitmap = GetCharBitmap(charCode);

        struct BitmapVertex { float x, y, u, v; };
        std::vector<BitmapVertex> verts;

        for (int row = 0; row < charH; ++row)
        {
            for (int col = 0; col < charW; ++col)
            {
                int bit = (charW - 1 - col);
                if (bitmap & (1u << (row * charW + bit)))
                {
                    float px = ndcX + col * scaleX / charW;
                    float py = ndcY - row * scaleY / charH;
                    float pw = scaleX / charW;
                    float ph = scaleY / charH;
                    verts.push_back({px,      py,      0.0f, 0.0f});
                    verts.push_back({px + pw, py,      1.0f, 0.0f});
                    verts.push_back({px + pw, py - ph, 1.0f, 1.0f});
                    verts.push_back({px,      py,      0.0f, 0.0f});
                    verts.push_back({px + pw, py - ph, 1.0f, 1.0f});
                    verts.push_back({px,      py - ph, 0.0f, 1.0f});
                }
            }
        }

        if (verts.empty()) return;

        GLuint charProg = GetCharProgram();
        glUseProgram(charProg);

        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(BitmapVertex),
            verts.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BitmapVertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BitmapVertex),
            reinterpret_cast<void*>(2 * sizeof(float)));

        GLint colorLoc = glGetUniformLocation(charProg, "uColor");
        if (colorLoc >= 0) glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glUseProgram(0);
    }

    static uint32_t GetCharBitmap(int charCode)
    {
        static const uint32_t bitmaps[38] = {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x7EFFFF7E,
            0x18FFFF18,
            0x3CFFFF3C,
            0x7EFFFF7E,
            0xFFFFDBFF,
            0xFF00FF18,
            0x3C427E00,
            0x427E3C00,
            0x427E3C42,
            0x7E3C4200,
            0x0C0CFE0C,
            0x0C0CFEFE,
            0x7C82FE82,
            0x7C82827C,
            0xFE82827C,
            0xFE8282FE,
            0xFE9292FE,
            0xFE9292FE,
            0x7C82827C,
            0xFE82827C,
            0x7C8282FE,
            0x7C82FEFE,
            0xFE8282FE,
            0x7C82FEFE,
            0x7C82FEFE,
            0x00000000,
            0x00000000,
            0x00000000,
        };
        if (charCode >= 0 && charCode < 38)
            return bitmaps[charCode];
        return 0;
    }

    GLuint GetCharProgram()
    {
        if (m_charProgram) return m_charProgram;
        const char* vert = R"(
            #version 450 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aUV;
            void main()
            {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        const char* frag = R"(
            #version 450 core
            uniform vec4 uColor;
            out vec4 FragColor;
            void main()
            {
                FragColor = uColor;
            }
        )";
        m_charProgram = CompileSimpleProgram(vert, frag);
        return m_charProgram;
    }

    GLuint GetBgProgram()
    {
        if (m_bgProgram) return m_bgProgram;
        const char* vert = R"(
            #version 450 core
            layout(location = 0) in vec2 aPos;
            uniform vec4 uRect;
            void main()
            {
                vec2 pos = mix(uRect.xy, uRect.zw, aPos);
                gl_Position = vec4(pos, 0.0, 1.0);
            }
        )";
        const char* frag = R"(
            #version 450 core
            out vec4 FragColor;
            void main()
            {
                FragColor = vec4(0.0, 0.0, 0.0, 0.7);
            }
        )";
        m_bgProgram = CompileSimpleProgram(vert, frag);
        return m_bgProgram;
    }

    GLuint CompileSimpleProgram(const char* vertSrc, const char* fragSrc)
    {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertSrc, nullptr);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragSrc, nullptr);
        glCompileShader(fs);
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return prog;
    }

    GLuint m_overlayFBO = 0;
    GLuint m_overlayTexture = 0;
    GLuint m_bgProgram = 0;
    GLuint m_charProgram = 0;
    GLuint m_bgVAO = 0;
    GLuint m_bgVBO = 0;
    int m_screenWidth = 0;
    int m_screenHeight = 0;
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
    GLuint emissiveTexture = 0;
    GLuint aoTexture = 0;
    bool hasAlbedoTexture = false;
    bool hasNormalTexture = false;
    bool hasMetallicRoughnessTexture = false;
    bool hasEmissiveTexture = false;
    bool hasAOTexture = false;
};

class OpenGLTexture
{
public:
    OpenGLTexture() = default;
    ~OpenGLTexture() { ClearAll(); }

    AssetHandle UploadTexture(int width, int height, int channels, const uint8_t* data, AssetHandle preassignedHandle = INVALID_HANDLE, bool sRGB = false)
    {
        if (width <= 0 || height <= 0 || !data) return INVALID_HANDLE;
        if (channels < 1 || channels > 4) return INVALID_HANDLE;

        size_t expectedSize = static_cast<size_t>(width) * height * channels;
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

        if (GLAD_GL_EXT_texture_filter_anisotropic)
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
