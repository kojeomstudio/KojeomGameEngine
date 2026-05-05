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
#include "Animation/Skeleton.h"
#include "Animation/AnimationClip.h"

#include <tiny_gltf.h>

#include <stb_image.h>

#include <algorithm>

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

struct SkeletonData
{
    Skeleton skeleton;
    std::string name;
};

struct AnimationClipData
{
    AnimationClip clip;
    std::string name;
    std::string skeletonPath;
};

class AssetStore
{
public:
    AssetHandle LoadMesh(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Path validation failed for mesh: {}", path);
            return INVALID_HANDLE;
        }

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

        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Path validation failed for skinned mesh: {}", path);
            return INVALID_HANDLE;
        }

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

        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Path validation failed for texture: {}", path);
            return INVALID_HANDLE;
        }

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

    AssetHandle LoadSkeleton(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_skeletonPaths.find(path);
        if (it != m_skeletonPaths.end()) return it->second;

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
            KE_LOG_ERROR("glTF load failed for skeleton: {} - {}", path, err);
            return INVALID_HANDLE;
        }
        if (!warn.empty()) KE_LOG_WARN("glTF warning: {}", warn);

        SkeletonData skelData;
        skelData.name = FileSystem::GetFileName(path);

        if (model.skins.empty())
        {
            KE_LOG_ERROR("glTF has no skins: {}", path);
            return INVALID_HANDLE;
        }

        const auto& skin = model.skins[0];

        int boneCount = static_cast<int>(skin.joints.size());
        for (int i = 0; i < boneCount; ++i)
        {
            Bone bone;
            bone.name = model.nodes[skin.joints[i]].name;
            bone.parentIndex = -1;
            skelData.skeleton.AddBone(bone);
        }

        for (int i = 0; i < boneCount; ++i)
        {
            int nodeIdx = skin.joints[i];
            const auto& node = model.nodes[nodeIdx];
            for (int child : node.children)
            {
                for (int j = 0; j < boneCount; ++j)
                {
                    if (skin.joints[j] == child)
                    {
                        auto& bones = const_cast<std::vector<Bone>&>(skelData.skeleton.GetBones());
                        bones[j].parentIndex = i;
                        break;
                    }
                }
            }
        }

        if (skin.inverseBindMatrices >= 0)
        {
            const auto& acc = model.accessors[skin.inverseBindMatrices];
            const auto& bufView = model.bufferViews[acc.bufferView];
            const auto& buf = model.buffers[bufView.buffer];
            const float* data = reinterpret_cast<const float*>(
                &buf.data[bufView.byteOffset + acc.byteOffset]);

            auto& bones = const_cast<std::vector<Bone>&>(skelData.skeleton.GetBones());
            for (int i = 0; i < boneCount; ++i)
            {
                glm::mat4 mat;
                memcpy(&mat, data + i * 16, sizeof(float) * 16);
                bones[i].inverseBindMatrix = glm::transpose(mat);
            }
        }

        if (!skelData.skeleton.Validate())
        {
            KE_LOG_ERROR("Skeleton validation failed: {}", path);
            return INVALID_HANDLE;
        }

        AssetHandle handle = m_nextHandle++;
        m_skeletonPaths[path] = handle;
        m_skeletons[handle] = std::move(skelData);
        KE_LOG_INFO("Loaded skeleton: {} ({} bones)", path, boneCount);
        return handle;
    }

    AssetHandle LoadAnimationClip(const std::string& path, AssetHandle skeletonHandle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_animationClipPaths.find(path);
        if (it != m_animationClipPaths.end()) return it->second;

        auto* skelData = GetSkeleton(skeletonHandle);
        if (!skelData)
        {
            KE_LOG_ERROR("Invalid skeleton handle for animation: {}", path);
            return INVALID_HANDLE;
        }

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
            KE_LOG_ERROR("glTF load failed for animation: {} - {}", path, err);
            return INVALID_HANDLE;
        }
        if (!warn.empty()) KE_LOG_WARN("glTF warning: {}", warn);

        if (model.animations.empty())
        {
            KE_LOG_ERROR("glTF has no animations: {}", path);
            return INVALID_HANDLE;
        }

        AnimationClipData clipData;
        clipData.name = FileSystem::GetFileName(path);
        clipData.skeletonPath = path;

        const auto& anim = model.animations[0];
        clipData.clip.SetName(anim.name.empty() ? "unnamed" : anim.name);

        for (const auto& channel : anim.channels)
        {
            const auto& sampler = anim.samplers[channel.sampler];
            const auto& targetNode = model.nodes[channel.target_node];

            int32_t boneIndex = skelData->skeleton.FindBoneIndex(targetNode.name);
            if (boneIndex < 0) continue;

            AnimationChannel animChannel;
            animChannel.boneIndex = boneIndex;
            animChannel.boneName = targetNode.name;

            const auto& timeAcc = model.accessors[sampler.input];
            const auto& timeBuf = model.bufferViews[timeAcc.bufferView];
            const float* times = reinterpret_cast<const float*>(
                &model.buffers[timeBuf.buffer].data[timeBuf.byteOffset + timeAcc.byteOffset]);

            float maxTime = 0.0f;
            for (int t = 0; t < static_cast<int>(timeAcc.count); ++t)
                maxTime = std::max(maxTime, times[t]);
            if (maxTime > clipData.clip.GetDuration())
                clipData.clip.SetDuration(maxTime);

            const auto& valAcc = model.accessors[sampler.output];
            const auto& valBuf = model.bufferViews[valAcc.bufferView];
            const float* values = reinterpret_cast<const float*>(
                &model.buffers[valBuf.buffer].data[valBuf.byteOffset + valAcc.byteOffset]);

            if (channel.target_path == "translation")
            {
                for (int t = 0; t < static_cast<int>(timeAcc.count); ++t)
                {
                    VectorKey key;
                    key.time = times[t];
                    key.value = glm::vec3(values[t * 3], values[t * 3 + 1], values[t * 3 + 2]);
                    animChannel.positionKeys.push_back(key);
                }
            }
            else if (channel.target_path == "rotation")
            {
                for (int t = 0; t < static_cast<int>(timeAcc.count); ++t)
                {
                    QuatKey key;
                    key.time = times[t];
                    key.value = glm::quat(values[t * 4 + 3], values[t * 4], values[t * 4 + 1], values[t * 4 + 2]);
                    animChannel.rotationKeys.push_back(key);
                }
            }
            else if (channel.target_path == "scale")
            {
                for (int t = 0; t < static_cast<int>(timeAcc.count); ++t)
                {
                    VectorKey key;
                    key.time = times[t];
                    key.value = glm::vec3(values[t * 3], values[t * 3 + 1], values[t * 3 + 2]);
                    animChannel.scaleKeys.push_back(key);
                }
            }

            clipData.clip.AddChannel(animChannel);
        }

        clipData.clip.SetTicksPerSecond(30.0f);

        AssetHandle handle = m_nextHandle++;
        m_animationClipPaths[path] = handle;
        m_animationClips[handle] = std::move(clipData);
        KE_LOG_INFO("Loaded animation clip: {} ({} channels, {:.2f}s)",
            path, clipData.clip.GetChannelCount(), clipData.clip.GetDurationSeconds());
        return handle;
    }

    struct GLTFMaterialInfo
    {
        glm::vec4 baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;
        glm::vec3 emissiveFactor = glm::vec3(0.0f);
        std::string albedoTexturePath;
        std::string normalTexturePath;
        std::string metallicRoughnessTexturePath;
        bool hasMaterial = false;
    };

    GLTFMaterialInfo ExtractGLTFMaterial(const tinygltf::Model& model, int materialIndex, const std::string& basePath)
    {
        GLTFMaterialInfo info;
        if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
            return info;

        const auto& mat = model.materials[materialIndex];
        info.hasMaterial = true;

        if (mat.values.count("baseColorFactor"))
        {
            auto& c = mat.values.at("baseColorFactor").number_array;
            if (c.size() >= 3)
                info.baseColorFactor = glm::vec4(static_cast<float>(c[0]), static_cast<float>(c[1]), static_cast<float>(c[2]),
                    c.size() >= 4 ? static_cast<float>(c[3]) : 1.0f);
        }
        if (mat.values.count("metallicFactor"))
            info.metallicFactor = static_cast<float>(mat.values.at("metallicFactor").number_value);
        if (mat.values.count("roughnessFactor"))
            info.roughnessFactor = static_cast<float>(mat.values.at("roughnessFactor").number_value);
        if (mat.additionalValues.count("emissiveFactor"))
        {
            auto& e = mat.additionalValues.at("emissiveFactor").number_array;
            if (e.size() >= 3)
                info.emissiveFactor = glm::vec3(static_cast<float>(e[0]), static_cast<float>(e[1]), static_cast<float>(e[2]));
        }

        auto resolveTexturePath = [&](int textureIndex) -> std::string
        {
            if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
                return "";
            const auto& texture = model.textures[textureIndex];
            if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
                return "";
            const auto& image = model.images[texture.source];
            if (!image.uri.empty())
                return basePath + image.uri;
            return "";
        };

        if (mat.values.count("baseColorTexture"))
            info.albedoTexturePath = resolveTexturePath(mat.values.at("baseColorTexture").TextureIndex());
        if (mat.normalTexture.index >= 0)
            info.normalTexturePath = resolveTexturePath(mat.normalTexture.index);
        if (mat.values.count("metallicRoughnessTexture"))
            info.metallicRoughnessTexturePath = resolveTexturePath(mat.values.at("metallicRoughnessTexture").TextureIndex());

        return info;
    }

    AssetHandle LoadGLTFWithMaterial(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Path validation failed for glTF: {}", path);
            return INVALID_HANDLE;
        }

        auto it = m_meshPaths.find(path);
        if (it != m_meshPaths.end()) return it->second;

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
            return INVALID_HANDLE;
        }
        if (!warn.empty()) KE_LOG_WARN("glTF warning: {}", warn);

        if (model.meshes.empty())
        {
            KE_LOG_ERROR("glTF has no meshes: {}", path);
            return INVALID_HANDLE;
        }

        std::string basePath = FileSystem::GetDirectory(path);

        const auto& mesh = model.meshes[0];
        MeshData meshData;
        meshData.name = mesh.name;

        int materialIndex = -1;

        for (const auto& primitive : mesh.primitives)
        {
            if (materialIndex < 0 && primitive.material >= 0)
                materialIndex = primitive.material;

            if (!primitive.attributes.count("POSITION"))
            {
                KE_LOG_WARN("glTF primitive missing POSITION attribute, skipping");
                continue;
            }

            const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posBuf = model.bufferViews[posAcc.bufferView];
            size_t posDataSize = model.buffers[posBuf.buffer].data.size();
            size_t posOffset = posBuf.byteOffset + posAcc.byteOffset;
            size_t requiredPosBytes = posAcc.count * 3 * sizeof(float);
            if (posOffset + requiredPosBytes > posDataSize)
            {
                KE_LOG_ERROR("glTF position buffer out of bounds: {}", path);
                return INVALID_HANDLE;
            }
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posOffset]);

            const float* normData = nullptr;
            if (primitive.attributes.count("NORMAL") > 0)
            {
                const auto& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normBuf = model.bufferViews[normAcc.bufferView];
                size_t normOffset = normBuf.byteOffset + normAcc.byteOffset;
                size_t requiredNormBytes = normAcc.count * 3 * sizeof(float);
                if (normOffset + requiredNormBytes > model.buffers[normBuf.buffer].data.size())
                {
                    KE_LOG_WARN("glTF normal buffer out of bounds, ignoring normals");
                    normData = nullptr;
                }
                else
                {
                    normData = reinterpret_cast<const float*>(
                        &model.buffers[normBuf.buffer].data[normOffset]);
                }
            }

            const float* uvData = nullptr;
            if (primitive.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvBuf = model.bufferViews[uvAcc.bufferView];
                size_t uvOffset = uvBuf.byteOffset + uvAcc.byteOffset;
                size_t requiredUvBytes = uvAcc.count * 2 * sizeof(float);
                if (uvOffset + requiredUvBytes > model.buffers[uvBuf.buffer].data.size())
                {
                    KE_LOG_WARN("glTF UV buffer out of bounds, ignoring UVs");
                    uvData = nullptr;
                }
                else
                {
                    uvData = reinterpret_cast<const float*>(
                        &model.buffers[uvBuf.buffer].data[uvOffset]);
                }
            }

            int vertexCount = static_cast<int>(posAcc.count);
            uint32_t baseIdx = static_cast<uint32_t>(meshData.vertices.size());

            for (int i = 0; i < vertexCount; ++i)
            {
                Vertex v{};
                v.position = glm::vec3(posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2]);
                if (normData) v.normal = glm::vec3(normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2]);
                if (uvData) v.uv = glm::vec2(uvData[i * 2], uvData[i * 2 + 1]);
                meshData.vertices.push_back(v);
            }

            if (primitive.indices >= 0)
            {
                const auto& idxAcc = model.accessors[primitive.indices];
                const auto& idxBuf = model.bufferViews[idxAcc.bufferView];
                const auto& buf = model.buffers[idxBuf.buffer];
                size_t idxOffset = idxBuf.byteOffset + idxAcc.byteOffset;

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    size_t requiredIdxBytes = idxAcc.count * sizeof(uint16_t);
                    if (idxOffset + requiredIdxBytes > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF index buffer out of bounds: {}", path);
                        return INVALID_HANDLE;
                    }
                    const uint16_t* idx16 = reinterpret_cast<const uint16_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
                    {
                        meshData.indices.push_back(baseIdx + idx16[i]);
                        meshData.indices.push_back(baseIdx + idx16[i + 1]);
                        meshData.indices.push_back(baseIdx + idx16[i + 2]);
                    }
                }
                else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    size_t requiredIdxBytes = idxAcc.count * sizeof(uint32_t);
                    if (idxOffset + requiredIdxBytes > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF index buffer out of bounds: {}", path);
                        return INVALID_HANDLE;
                    }
                    const uint32_t* idx32 = reinterpret_cast<const uint32_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
                    {
                        meshData.indices.push_back(baseIdx + idx32[i]);
                        meshData.indices.push_back(baseIdx + idx32[i + 1]);
                        meshData.indices.push_back(baseIdx + idx32[i + 2]);
                    }
                }
            }
            else
            {
                for (int i = 0; i < vertexCount; i += 3)
                {
                    meshData.indices.push_back(baseIdx + i);
                    meshData.indices.push_back(baseIdx + i + 1);
                    meshData.indices.push_back(baseIdx + i + 2);
                }
            }
        }

        ComputeBounds(meshData.vertices, meshData.boundsMin, meshData.boundsMax);

        if (meshData.vertices.empty())
        {
            KE_LOG_ERROR("glTF mesh has no vertices: {}", path);
            return INVALID_HANDLE;
        }

        glm::vec3 extents = meshData.boundsMax - meshData.boundsMin;
        bool hasDegenerateBounds = (extents.x < 0.0001f || extents.y < 0.0001f || extents.z < 0.0001f);
        if (hasDegenerateBounds)
        {
            KE_LOG_WARN("glTF mesh has degenerate bounds: {} (extents: {:.3f}, {:.3f}, {:.3f})",
                path, extents.x, extents.y, extents.z);
        }

        AssetHandle handle = m_nextHandle++;
        m_meshPaths[path] = handle;
        m_meshes[handle] = std::move(meshData);

        if (materialIndex >= 0)
        {
            auto matInfo = ExtractGLTFMaterial(model, materialIndex, basePath);
            if (matInfo.hasMaterial)
            {
                MaterialData matData;
                matData.albedo = glm::vec3(matInfo.baseColorFactor);
                matData.metallic = matInfo.metallicFactor;
                matData.roughness = matInfo.roughnessFactor;
                matData.emissive = matInfo.emissiveFactor;
                matData.albedoTexturePath = matInfo.albedoTexturePath;
                matData.normalTexturePath = matInfo.normalTexturePath;
                matData.metallicRoughnessTexturePath = matInfo.metallicRoughnessTexturePath;

                AssetHandle matHandle = m_nextHandle++;
                m_materials[matHandle] = matData;
                m_meshMaterialMap[handle] = matHandle;
                KE_LOG_INFO("Imported material for mesh {} (albedo: [{:.2f},{:.2f},{:.2f}], metallic: {:.2f}, roughness: {:.2f})",
                    path, matData.albedo.x, matData.albedo.y, matData.albedo.z, matData.metallic, matData.roughness);
            }
        }

        KE_LOG_INFO("Loaded mesh with material: {} ({} vertices, {} indices)",
            path, m_meshes[handle].vertices.size(), m_meshes[handle].indices.size());
        return handle;
    }

    AssetHandle GetMaterialForMesh(AssetHandle meshHandle) const
    {
        auto it = m_meshMaterialMap.find(meshHandle);
        return (it != m_meshMaterialMap.end()) ? it->second : INVALID_HANDLE;
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

    SkeletonData* GetSkeleton(AssetHandle handle)
    {
        auto it = m_skeletons.find(handle);
        return (it != m_skeletons.end()) ? &it->second : nullptr;
    }

    const SkeletonData* GetSkeleton(AssetHandle handle) const
    {
        auto it = m_skeletons.find(handle);
        return (it != m_skeletons.end()) ? &it->second : nullptr;
    }

    AnimationClipData* GetAnimationClip(AssetHandle handle)
    {
        auto it = m_animationClips.find(handle);
        return (it != m_animationClips.end()) ? &it->second : nullptr;
    }

    const AnimationClipData* GetAnimationClip(AssetHandle handle) const
    {
        auto it = m_animationClips.find(handle);
        return (it != m_animationClips.end()) ? &it->second : nullptr;
    }

    void Clear()
    {
        m_meshes.clear();
        m_skinnedMeshes.clear();
        m_textures.clear();
        m_materials.clear();
        m_terrains.clear();
        m_skeletons.clear();
        m_animationClips.clear();
        m_meshPaths.clear();
        m_skinnedMeshPaths.clear();
        m_texturePaths.clear();
        m_terrainPaths.clear();
        m_skeletonPaths.clear();
        m_animationClipPaths.clear();
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
            if (!primitive.attributes.count("POSITION"))
            {
                KE_LOG_WARN("glTF primitive missing POSITION, skipping");
                continue;
            }

            const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posBuf = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posBuf.byteOffset + posAcc.byteOffset]);

            const float* normData = nullptr;
            if (primitive.attributes.count("NORMAL") > 0)
            {
                const auto& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normBuf = model.bufferViews[normAcc.bufferView];
                size_t normOffset = normBuf.byteOffset + normAcc.byteOffset;
                if (normOffset + normAcc.count * 3 * sizeof(float) <= model.buffers[normBuf.buffer].data.size())
                {
                    normData = reinterpret_cast<const float*>(
                        &model.buffers[normBuf.buffer].data[normOffset]);
                }
            }

            const float* uvData = nullptr;
            if (primitive.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvBuf = model.bufferViews[uvAcc.bufferView];
                size_t uvOffset = uvBuf.byteOffset + uvAcc.byteOffset;
                if (uvOffset + uvAcc.count * 2 * sizeof(float) <= model.buffers[uvBuf.buffer].data.size())
                {
                    uvData = reinterpret_cast<const float*>(
                        &model.buffers[uvBuf.buffer].data[uvOffset]);
                }
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
                size_t idxOffset = idxBuf.byteOffset + idxAcc.byteOffset;

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    size_t requiredBytes = idxAcc.count * sizeof(uint16_t);
                    if (idxOffset + requiredBytes > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF index buffer out of bounds: {}", path);
                        return false;
                    }
                    const uint16_t* idx16 = reinterpret_cast<const uint16_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx16[i]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 2]);
                    }
                }
                else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    size_t requiredBytes = idxAcc.count * sizeof(uint32_t);
                    if (idxOffset + requiredBytes > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF index buffer out of bounds: {}", path);
                        return false;
                    }
                    const uint32_t* idx32 = reinterpret_cast<const uint32_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
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
            if (!primitive.attributes.count("POSITION")) continue;

            const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posBuf = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posBuf.byteOffset + posAcc.byteOffset]);

            const float* normData = nullptr;
            if (primitive.attributes.count("NORMAL") > 0)
            {
                const auto& normAcc = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normBuf = model.bufferViews[normAcc.bufferView];
                size_t normOff = normBuf.byteOffset + normAcc.byteOffset;
                if (normOff + normAcc.count * 3 * sizeof(float) <= model.buffers[normBuf.buffer].data.size())
                    normData = reinterpret_cast<const float*>(&model.buffers[normBuf.buffer].data[normOff]);
            }

            const float* uvData = nullptr;
            if (primitive.attributes.count("TEXCOORD_0") > 0)
            {
                const auto& uvAcc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvBuf = model.bufferViews[uvAcc.bufferView];
                size_t uvOff = uvBuf.byteOffset + uvAcc.byteOffset;
                if (uvOff + uvAcc.count * 2 * sizeof(float) <= model.buffers[uvBuf.buffer].data.size())
                    uvData = reinterpret_cast<const float*>(&model.buffers[uvBuf.buffer].data[uvOff]);
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
                size_t idxOffset = idxBuf.byteOffset + idxAcc.byteOffset;

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    if (idxOffset + idxAcc.count * sizeof(uint16_t) > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF skinned index buffer out of bounds: {}", path);
                        return false;
                    }
                    const uint16_t* idx16 = reinterpret_cast<const uint16_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
                    {
                        outMesh.indices.push_back(baseIdx + idx16[i]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 1]);
                        outMesh.indices.push_back(baseIdx + idx16[i + 2]);
                    }
                }
                else
                {
                    if (idxOffset + idxAcc.count * sizeof(uint32_t) > buf.data.size())
                    {
                        KE_LOG_ERROR("glTF skinned index buffer out of bounds: {}", path);
                        return false;
                    }
                    const uint32_t* idx32 = reinterpret_cast<const uint32_t*>(&buf.data[idxOffset]);
                    for (int i = 0; i < static_cast<int>(idxAcc.count); i += 3)
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
    std::unordered_map<AssetHandle, SkeletonData> m_skeletons;
    std::unordered_map<AssetHandle, AnimationClipData> m_animationClips;
    std::unordered_map<AssetHandle, AssetHandle> m_meshMaterialMap;
    std::unordered_map<std::string, AssetHandle> m_meshPaths;
    std::unordered_map<std::string, AssetHandle> m_skinnedMeshPaths;
    std::unordered_map<std::string, AssetHandle> m_texturePaths;
    std::unordered_map<std::string, AssetHandle> m_terrainPaths;
    std::unordered_map<std::string, AssetHandle> m_skeletonPaths;
    std::unordered_map<std::string, AssetHandle> m_animationClipPaths;
    std::mutex m_mutex;
};
}
