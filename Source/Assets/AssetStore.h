#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <glm/glm.hpp>
#include "Renderer/RenderScene.h"
#include "Core/StringId.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"

#include <stb_image.h>

namespace Kojeom
{
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    std::string name;
};

struct TextureData
{
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string path;
};

struct MaterialData
{
    glm::vec3 albedo = glm::vec3(0.8f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    std::string albedoTexturePath;
    std::string normalTexturePath;
    std::string metallicRoughnessTexturePath;
};

class AssetStore
{
public:
    AssetHandle LoadMesh(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_meshPaths.find(path);
        if (it != m_meshPaths.end()) return it->second;

        MeshData meshData;
        if (!LoadOBJMesh(path, meshData))
        {
            KE_LOG_ERROR("Failed to load mesh: {}", path);
            return INVALID_HANDLE;
        }

        AssetHandle handle = m_nextHandle++;
        m_meshPaths[path] = handle;
        m_meshes[handle] = std::move(meshData);
        KE_LOG_INFO("Loaded mesh: {} ({} vertices, {} indices)",
            path, m_meshes[handle].vertices.size(), m_meshes[handle].indices.size());
        return handle;
    }

    AssetHandle LoadTexture(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_texturePaths.find(path);
        if (it != m_texturePaths.end()) return it->second;

        TextureData texData;
        stbi_set_flip_vertically_on_load(true);
        uint8_t* data = stbi_load(path.c_str(), &texData.width, &texData.height,
            &texData.channels, 0);
        if (!data)
        {
            KE_LOG_ERROR("Failed to load texture: {}", path);
            return INVALID_HANDLE;
        }

        texData.pixels.assign(data, data + texData.width * texData.height * texData.channels);
        texData.path = path;
        stbi_image_free(data);

        AssetHandle handle = m_nextHandle++;
        m_texturePaths[path] = handle;
        m_textures[handle] = std::move(texData);
        KE_LOG_INFO("Loaded texture: {} ({}x{}, {} channels)", path,
            m_textures[handle].width, m_textures[handle].height, m_textures[handle].channels);
        return handle;
    }

    AssetHandle RegisterMaterial(const MaterialData& material)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        AssetHandle handle = m_nextHandle++;
        m_materials[handle] = material;
        return handle;
    }

    const MeshData* GetMesh(AssetHandle handle) const
    {
        auto it = m_meshes.find(handle);
        return (it != m_meshes.end()) ? &it->second : nullptr;
    }

    const TextureData* GetTexture(AssetHandle handle) const
    {
        auto it = m_textures.find(handle);
        return (it != m_textures.end()) ? &it->second : nullptr;
    }

    const MaterialData* GetMaterial(AssetHandle handle) const
    {
        auto it = m_materials.find(handle);
        return (it != m_materials.end()) ? &it->second : nullptr;
    }

    void Clear()
    {
        m_meshes.clear();
        m_textures.clear();
        m_materials.clear();
        m_meshPaths.clear();
        m_texturePaths.clear();
    }

private:
    bool LoadOBJMesh(const std::string& path, MeshData& outMesh)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;

        auto content = FileSystem::ReadTextFile(path);
        if (content.empty())
        {
            if (!FileSystem::FileExists(path))
            {
                KE_LOG_ERROR("Mesh file not found: {}", path);
                return false;
            }
        }

        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line))
        {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                glm::vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vn")
            {
                glm::vec3 norm;
                iss >> norm.x >> norm.y >> norm.z;
                normals.push_back(norm);
            }
            else if (prefix == "vt")
            {
                glm::vec2 uv;
                iss >> uv.x >> uv.y;
                uvs.push_back(uv);
            }
            else if (prefix == "f")
            {
                std::string v1, v2, v3;
                iss >> v1 >> v2 >> v3;
                auto parseFace = [](const std::string& face)
                {
                    int v = 0, vt = 0, vn = 0;
                    size_t p1 = face.find('/');
                    if (p1 == std::string::npos) { v = std::stoi(face); return std::make_tuple(v, vt, vn); }
                    v = std::stoi(face.substr(0, p1));
                    size_t p2 = face.find('/', p1 + 1);
                    if (p2 == std::string::npos)
                    {
                        vt = std::stoi(face.substr(p1 + 1));
                        return std::make_tuple(v, vt, vn);
                    }
                    if (p2 > p1 + 1) vt = std::stoi(face.substr(p1 + 1, p2 - p1 - 1));
                    if (face.size() > p2 + 1) vn = std::stoi(face.substr(p2 + 1));
                    return std::make_tuple(v, vt, vn);
                };

                auto [iv1, ivt1, ivn1] = parseFace(v1);
                auto [iv2, ivt2, ivn2] = parseFace(v2);
                auto [iv3, ivt3, ivn3] = parseFace(v3);

                uint32_t baseIdx = static_cast<uint32_t>(outMesh.vertices.size());
                for (auto [iv, ivt, ivn] : { std::make_tuple(iv1, ivt1, ivn1),
                    std::make_tuple(iv2, ivt2, ivn2), std::make_tuple(iv3, ivt3, ivn3) })
                {
                    Vertex v{};
                    if (iv > 0 && iv <= static_cast<int>(positions.size()))
                        v.position = positions[iv - 1];
                    if (ivn > 0 && ivn <= static_cast<int>(normals.size()))
                        v.normal = normals[ivn - 1];
                    if (ivt > 0 && ivt <= static_cast<int>(uvs.size()))
                        v.uv = uvs[ivt - 1];
                    outMesh.vertices.push_back(v);
                }
                outMesh.indices.push_back(baseIdx);
                outMesh.indices.push_back(baseIdx + 1);
                outMesh.indices.push_back(baseIdx + 2);
            }
        }

        if (!outMesh.vertices.empty())
        {
            outMesh.boundsMin = outMesh.boundsMax = outMesh.vertices[0].position;
            for (const auto& v : outMesh.vertices)
            {
                outMesh.boundsMin = glm::min(outMesh.boundsMin, v.position);
                outMesh.boundsMax = glm::max(outMesh.boundsMax, v.position);
            }
        }

        return !outMesh.vertices.empty();
    }

    AssetHandle m_nextHandle = 1;
    std::unordered_map<AssetHandle, MeshData> m_meshes;
    std::unordered_map<AssetHandle, TextureData> m_textures;
    std::unordered_map<AssetHandle, MaterialData> m_materials;
    std::unordered_map<std::string, AssetHandle> m_meshPaths;
    std::unordered_map<std::string, AssetHandle> m_texturePaths;
    std::mutex m_mutex;
};
}
