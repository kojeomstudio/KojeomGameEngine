#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Renderer/RenderScene.h"
#include "Core/StringId.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"

#include <tiny_gltf.h>

#include <stb_image.h>

namespace Kojeom
{
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct SkinnedVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 boneWeights = glm::vec4(0.0f);
    glm::vec4 boneIndices = glm::vec4(0.0f);
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    std::string name;
};

struct SkinnedMeshData
{
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;
    AssetHandle skeletonHandle = INVALID_HANDLE;
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

struct TerrainData
{
    std::vector<float> heightmap;
    int width = 0;
    int height = 0;
    float cellSize = 1.0f;
    float maxHeight = 10.0f;
    std::string name;
    float GetHeight(int x, int z) const
    {
        if (x < 0 || x >= width || z < 0 || z >= height) return 0.0f;
        return heightmap[z * width + x];
    }
    float GetHeightInterpolated(float worldX, float worldZ) const
    {
        float gx = worldX / cellSize;
        float gz = worldZ / cellSize;
        int x0 = static_cast<int>(glm::floor(gx));
        int z0 = static_cast<int>(glm::floor(gz));
        int x1 = x0 + 1;
        int z1 = z0 + 1;
        float fx = gx - static_cast<float>(x0);
        float fz = gz - static_cast<float>(z0);
        float h00 = GetHeight(x0, z0);
        float h10 = GetHeight(x1, z0);
        float h01 = GetHeight(x0, z1);
        float h11 = GetHeight(x1, z1);
        float h0 = glm::mix(h00, h10, fx);
        float h1 = glm::mix(h01, h11, fx);
        return glm::mix(h0, h1, fz);
    }
};

class AssetStore
{
public:
    AssetHandle LoadMesh(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_meshPaths.find(path);
        if (it != m_meshPaths.end()) return it->second;

        std::string ext = FileSystem::GetExtension(path);
        MeshData meshData;
        bool loaded = false;

        if (ext == ".obj")
            loaded = LoadOBJMesh(path, meshData);
        else if (ext == ".gltf" || ext == ".glb")
            loaded = LoadGLTFStaticMesh(path, meshData);
        else
        {
            KE_LOG_ERROR("Unsupported mesh format: {}", path);
            return INVALID_HANDLE;
        }

        if (!loaded)
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

    AssetHandle LoadSkinnedMesh(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_skinnedMeshPaths.find(path);
        if (it != m_skinnedMeshPaths.end()) return it->second;

        SkinnedMeshData meshData;
        if (!LoadGLTFSkinnedMesh(path, meshData))
        {
            KE_LOG_ERROR("Failed to load skinned mesh: {}", path);
            return INVALID_HANDLE;
        }

        AssetHandle handle = m_nextHandle++;
        m_skinnedMeshPaths[path] = handle;
        m_skinnedMeshes[handle] = std::move(meshData);
        KE_LOG_INFO("Loaded skinned mesh: {} ({} vertices, {} indices)",
            path, m_skinnedMeshes[handle].vertices.size(), m_skinnedMeshes[handle].indices.size());
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

    AssetHandle CreateTerrain(const std::string& name, int width, int height,
        float cellSize, float maxHeight, const std::vector<float>& heights)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        AssetHandle handle = m_nextHandle++;
        TerrainData terrain;
        terrain.name = name;
        terrain.width = width;
        terrain.height = height;
        terrain.cellSize = cellSize;
        terrain.maxHeight = maxHeight;
        terrain.heightmap = heights;
        if (terrain.heightmap.empty())
        {
            terrain.heightmap.resize(static_cast<size_t>(width) * height, 0.0f);
        }
        m_terrains[handle] = std::move(terrain);
        KE_LOG_INFO("Created terrain: {} ({}x{}, cellSize={})", name, width, height, cellSize);
        return handle;
    }

    AssetHandle LoadTerrainFromHeightmap(const std::string& imagePath, float cellSize, float maxHeight)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_terrainPaths.find(imagePath);
        if (it != m_terrainPaths.end()) return it->second;

        int imgW, imgH, imgChannels;
        stbi_set_flip_vertically_on_load(false);
        uint8_t* data = stbi_load(imagePath.c_str(), &imgW, &imgH, &imgChannels, 1);
        if (!data)
        {
            KE_LOG_ERROR("Failed to load heightmap: {}", imagePath);
            return INVALID_HANDLE;
        }

        std::vector<float> heights(static_cast<size_t>(imgW) * imgH);
        for (int i = 0; i < imgW * imgH; ++i)
        {
            heights[i] = (static_cast<float>(data[i]) / 255.0f) * maxHeight;
        }
        stbi_image_free(data);

        AssetHandle handle = m_nextHandle++;
        TerrainData terrain;
        terrain.name = imagePath;
        terrain.width = imgW;
        terrain.height = imgH;
        terrain.cellSize = cellSize;
        terrain.maxHeight = maxHeight;
        terrain.heightmap = std::move(heights);
        m_terrains[handle] = std::move(terrain);
        m_terrainPaths[imagePath] = handle;
        KE_LOG_INFO("Loaded terrain heightmap: {} ({}x{})", imagePath, imgW, imgH);
        return handle;
    }

    const MeshData* GetMesh(AssetHandle handle) const
    {
        auto it = m_meshes.find(handle);
        return (it != m_meshes.end()) ? &it->second : nullptr;
    }

    const SkinnedMeshData* GetSkinnedMesh(AssetHandle handle) const
    {
        auto it = m_skinnedMeshes.find(handle);
        return (it != m_skinnedMeshes.end()) ? &it->second : nullptr;
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

    const TerrainData* GetTerrain(AssetHandle handle) const
    {
        auto it = m_terrains.find(handle);
        return (it != m_terrains.end()) ? &it->second : nullptr;
    }

    void Clear()
    {
        m_meshes.clear();
        m_skinnedMeshes.clear();
        m_textures.clear();
        m_materials.clear();
        m_terrains.clear();
        m_meshPaths.clear();
        m_skinnedMeshPaths.clear();
        m_texturePaths.clear();
        m_terrainPaths.clear();
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
                        if (p1 + 1 < face.size()) vt = std::stoi(face.substr(p1 + 1));
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
                    Vertex vtx{};
                    if (iv > 0 && iv <= static_cast<int>(positions.size()))
                        vtx.position = positions[iv - 1];
                    if (ivn > 0 && ivn <= static_cast<int>(normals.size()))
                        vtx.normal = normals[ivn - 1];
                    if (ivt > 0 && ivt <= static_cast<int>(uvs.size()))
                        vtx.uv = uvs[ivt - 1];
                    outMesh.vertices.push_back(vtx);
                }
                outMesh.indices.push_back(baseIdx);
                outMesh.indices.push_back(baseIdx + 1);
                outMesh.indices.push_back(baseIdx + 2);
            }
        }

        ComputeBounds(outMesh.vertices, outMesh.boundsMin, outMesh.boundsMax);
        return !outMesh.vertices.empty();
    }

    bool LoadGLTFStaticMesh(const std::string& path, MeshData& outMesh)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = false;
        if (FileSystem::GetExtension(path) == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        else
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

        if (!ret)
        {
            KE_LOG_ERROR("glTF load failed: {} - {}", path, err);
            return false;
        }
        if (!warn.empty()) KE_LOG_WARN("glTF warning: {}", warn);

        if (model.meshes.empty())
        {
            KE_LOG_ERROR("glTF has no meshes: {}", path);
            return false;
        }

        const auto& mesh = model.meshes[0];
        outMesh.name = mesh.name;

        for (const auto& primitive : mesh.primitives)
        {
            const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posBuf = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posBuf.byteOffset + posAcc.byteOffset]);

            const float* normData = nullptr;
            if (primitive.attributes.count("NORMAL") > 0)
            {
                const auto& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normBuf = model.bufferViews[normAcc.bufferView];
                normData = reinterpret_cast<const float*>(
                    &model.buffers[normBuf.buffer].data[normBuf.byteOffset + normAcc.byteOffset]);
            }

            const float* uvData = nullptr;
            if (primitive.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvBuf = model.bufferViews[uvAcc.bufferView];
                uvData = reinterpret_cast<const float*>(
                    &model.buffers[uvBuf.buffer].data[uvBuf.byteOffset + uvAcc.byteOffset]);
            }

            int vertexCount = static_cast<int>(posAcc.count);
            uint32_t baseIdx = static_cast<uint32_t>(outMesh.vertices.size());

            for (int i = 0; i < vertexCount; ++i)
            {
                Vertex v{};
                v.position = glm::vec3(posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2]);
                if (normData) v.normal = glm::vec3(normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2]);
                if (uvData) v.uv = glm::vec2(uvData[i * 2], uvData[i * 2 + 1]);
                outMesh.vertices.push_back(v);
            }

            if (primitive.indices >= 0)
            {
                const auto& idxAcc = model.accessors[primitive.indices];
                const auto& idxBuf = model.bufferViews[idxAcc.bufferView];
                const auto& buf = model.buffers[idxBuf.buffer];

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    const uint16_t* idx16 = reinterpret_cast<const uint16_t*>(
                        &buf.data[idxBuf.byteOffset + idxAcc.byteOffset]);
                    for (int i = 0; i < idxAcc.count; i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx16[i]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 2]);
                    }
                }
                else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    const uint32_t* idx32 = reinterpret_cast<const uint32_t*>(
                        &buf.data[idxBuf.byteOffset + idxAcc.byteOffset]);
                    for (int i = 0; i < idxAcc.count; i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx32[i]);
                        outMesh.indices.push_back(baseIdx + idx32[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx32[i + 2]);
                    }
                }
            }
            else
            {
                for (int i = 0; i < vertexCount; i += 3)
                {
                    outMesh.indices.push_back(baseIdx + i);
                    outMesh.indices.push_back(baseIdx + i + 1);
                    outMesh.indices.push_back(baseIdx + i + 2);
                }
            }
        }

        ComputeBounds(outMesh.vertices, outMesh.boundsMin, outMesh.boundsMax);
        return !outMesh.vertices.empty();
    }

    bool LoadGLTFSkinnedMesh(const std::string& path, SkinnedMeshData& outMesh)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = false;
        if (FileSystem::GetExtension(path) == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        else
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

        if (!ret)
        {
            KE_LOG_ERROR("glTF load failed: {} - {}", path, err);
            return false;
        }

        if (model.meshes.empty()) return false;

        const auto& mesh = model.meshes[0];
        outMesh.name = mesh.name;

        for (const auto& primitive : mesh.primitives)
        {
            const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posBuf = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posBuf.byteOffset + posAcc.byteOffset]);

            const float* normData = nullptr;
            if (primitive.attributes.count("NORMAL") > 0)
            {
                const auto& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normBuf = model.bufferViews[normAcc.bufferView];
                normData = reinterpret_cast<const float*>(
                    &model.buffers[normBuf.buffer].data[normBuf.byteOffset + normAcc.byteOffset]);
            }

            const float* uvData = nullptr;
            if (primitive.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvBuf = model.bufferViews[uvAcc.bufferView];
                uvData = reinterpret_cast<const float*>(
                    &model.buffers[uvBuf.buffer].data[uvBuf.byteOffset + uvAcc.byteOffset]);
            }

            const float* weightData = nullptr;
            if (primitive.attributes.count("WEIGHTS_0") > 0)
            {
                const auto& wAcc = model.accessors[primitive.attributes.at("WEIGHTS_0")];
                const auto& wBuf = model.bufferViews[wAcc.bufferView];
                weightData = reinterpret_cast<const float*>(
                    &model.buffers[wBuf.buffer].data[wBuf.byteOffset + wAcc.byteOffset]);
            }

            const uint16_t* jointData = nullptr;
            const uint8_t* jointData8 = nullptr;
            int jointComponentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            if (primitive.attributes.count("JOINTS_0") > 0)
            {
                const auto& jAcc = model.accessors[primitive.attributes.at("JOINTS_0")];
                const auto& jBuf = model.bufferViews[jAcc.bufferView];
                jointComponentType = jAcc.componentType;
                if (jointComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    jointData = reinterpret_cast<const uint16_t*>(
                        &model.buffers[jBuf.buffer].data[jBuf.byteOffset + jAcc.byteOffset]);
                else
                    jointData8 = reinterpret_cast<const uint8_t*>(
                        &model.buffers[jBuf.buffer].data[jBuf.byteOffset + jAcc.byteOffset]);
            }

            int vertexCount = static_cast<int>(posAcc.count);
            uint32_t baseIdx = static_cast<uint32_t>(outMesh.vertices.size());

            for (int i = 0; i < vertexCount; ++i)
            {
                SkinnedVertex v{};
                v.position = glm::vec3(posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2]);
                if (normData) v.normal = glm::vec3(normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2]);
                if (uvData) v.uv = glm::vec2(uvData[i * 2], uvData[i * 2 + 1]);
                if (weightData) v.boneWeights = glm::vec4(weightData[i * 4], weightData[i * 4 + 1], weightData[i * 4 + 2], weightData[i * 4 + 3]);
                if (jointData) v.boneIndices = glm::vec4(static_cast<float>(jointData[i * 4]), static_cast<float>(jointData[i * 4 + 1]), static_cast<float>(jointData[i * 4 + 2]), static_cast<float>(jointData[i * 4 + 3]));
                else if (jointData8) v.boneIndices = glm::vec4(static_cast<float>(jointData8[i * 4]), static_cast<float>(jointData8[i * 4 + 1]), static_cast<float>(jointData8[i * 4 + 2]), static_cast<float>(jointData8[i * 4 + 3]));
                outMesh.vertices.push_back(v);
            }

            if (primitive.indices >= 0)
            {
                const auto& idxAcc = model.accessors[primitive.indices];
                const auto& idxBuf = model.bufferViews[idxAcc.bufferView];
                const auto& buf = model.buffers[idxBuf.buffer];

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    const uint16_t* idx16 = reinterpret_cast<const uint16_t*>(
                        &buf.data[idxBuf.byteOffset + idxAcc.byteOffset]);
                    for (int i = 0; i < idxAcc.count; i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx16[i]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 2]);
                    }
                }
                else
                {
                    const uint32_t* idx32 = reinterpret_cast<const uint32_t*>(
                        &buf.data[idxBuf.byteOffset + idxAcc.byteOffset]);
                    for (int i = 0; i < idxAcc.count; i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx32[i]);
                        outMesh.indices.push_back(baseIdx + idx32[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx32[i + 2]);
                    }
                }
            }
            else
            {
                for (int i = 0; i < vertexCount; i += 3)
                {
                    outMesh.indices.push_back(baseIdx + i);
                    outMesh.indices.push_back(baseIdx + i + 1);
                    outMesh.indices.push_back(baseIdx + i + 2);
                }
            }
        }

        return !outMesh.vertices.empty();
    }

    void ComputeBounds(const std::vector<Vertex>& vertices, glm::vec3& bMin, glm::vec3& bMax)
    {
        if (vertices.empty()) return;
        bMin = bMax = vertices[0].position;
        for (const auto& v : vertices)
        {
            bMin = glm::min(bMin, v.position);
            bMax = glm::max(bMax, v.position);
        }
    }

    AssetHandle m_nextHandle = 1;
    std::unordered_map<AssetHandle, MeshData> m_meshes;
    std::unordered_map<AssetHandle, SkinnedMeshData> m_skinnedMeshes;
    std::unordered_map<AssetHandle, TextureData> m_textures;
    std::unordered_map<AssetHandle, MaterialData> m_materials;
    std::unordered_map<AssetHandle, TerrainData> m_terrains;
    std::unordered_map<std::string, AssetHandle> m_meshPaths;
    std::unordered_map<std::string, AssetHandle> m_skinnedMeshPaths;
    std::unordered_map<std::string, AssetHandle> m_texturePaths;
    std::unordered_map<std::string, AssetHandle> m_terrainPaths;
    std::mutex m_mutex;
};
}
