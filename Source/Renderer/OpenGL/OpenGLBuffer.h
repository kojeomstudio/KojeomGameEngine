#pragma once

#include <glad/gl.h>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "Renderer/RenderScene.h"
#include "Core/Log.h"

namespace Kojeom
{
struct GLMeshData
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    uint32_t indexCount = 0;
};

class OpenGLBuffer
{
public:
    OpenGLBuffer() = default;
    ~OpenGLBuffer() { ClearAll(); }

    AssetHandle UploadMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices, int vertexStride,
        AssetHandle preassignedHandle)
    {
        if (vertices.empty() || indices.empty()) return INVALID_HANDLE;
        if (vertexStride < 3) return INVALID_HANDLE;

        AssetHandle handle = (preassignedHandle != INVALID_HANDLE) ? preassignedHandle : GenerateHandle();

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
        if (vertexStride >= 8)
        {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, strideBytes,
                reinterpret_cast<void*>(6 * sizeof(float)));
        }

        glBindVertexArray(0);

        m_meshCache[handle] = mesh;
        return handle;
    }

    AssetHandle UploadSkinnedMesh(const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices, AssetHandle preassignedHandle)
    {
        if (vertices.empty() || indices.empty()) return INVALID_HANDLE;

        AssetHandle handle = (preassignedHandle != INVALID_HANDLE) ? preassignedHandle : GenerateHandle();

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

    void CreateScreenQuad(GLuint& outVAO, GLuint& outVBO)
    {
        if (outVAO != 0) return;
        float quad[] = {
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
        };
        glGenVertexArrays(1, &outVAO);
        glGenBuffers(1, &outVBO);
        glBindVertexArray(outVAO);
        glBindBuffer(GL_ARRAY_BUFFER, outVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    const GLMeshData* GetMesh(AssetHandle handle) const
    {
        auto it = m_meshCache.find(handle);
        return (it != m_meshCache.end()) ? &it->second : nullptr;
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

    void ClearAll()
    {
        for (auto& [handle, mesh] : m_meshCache)
        {
            if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
            if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
            if (mesh.ibo) glDeleteBuffers(1, &mesh.ibo);
        }
        m_meshCache.clear();
    }

    void DrawMesh(AssetHandle handle) const
    {
        auto it = m_meshCache.find(handle);
        if (it != m_meshCache.end() && it->second.vao)
        {
            glBindVertexArray(it->second.vao);
            glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
    }

    void DrawMesh(const GLMeshData& mesh) const
    {
        if (mesh.vao)
        {
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
    }

    void DrawScreenQuad(GLuint vao) const
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    const std::unordered_map<AssetHandle, GLMeshData>& GetMeshCache() const { return m_meshCache; }

private:
    AssetHandle GenerateHandle() { return ++m_nextHandle; }

    std::unordered_map<AssetHandle, GLMeshData> m_meshCache;
    AssetHandle m_nextHandle = 0;
};
}
