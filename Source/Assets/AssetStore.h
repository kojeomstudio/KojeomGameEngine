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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
    float emissiveStrength = 1.0f;
    std::string albedoTexturePath;
    std::string normalTexturePath;
    std::string metallicRoughnessTexturePath;
    std::string emissiveTexturePath;
    std::string aoTexturePath;
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
        else if (ext == ".fbx" || ext == ".FBX")
        {
            return LoadFBXInternalLocked(path);
        }
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

        if (!FileSystem::FileExists(path))
        {
            KE_LOG_ERROR("Texture file not found: {}", path);
            return INVALID_HANDLE;
        }

        TextureData texData;
        stbi_set_flip_vertically_on_load(true);
        uint8_t* data = stbi_load(path.c_str(), &texData.width, &texData.height,
            &texData.channels, 0);
        if (!data)
        {
            KE_LOG_ERROR("Failed to load texture: {} - {}", path, stbi_failure_reason());
            return INVALID_HANDLE;
        }

        if (texData.width <= 0 || texData.height <= 0 || texData.channels <= 0 ||
            texData.channels > 4)
        {
            KE_LOG_ERROR("Invalid texture dimensions/channels: {} ({}x{}, {}ch)",
                path, texData.width, texData.height, texData.channels);
            stbi_image_free(data);
            return INVALID_HANDLE;
        }

        size_t pixelCount = static_cast<size_t>(texData.width) *
            static_cast<size_t>(texData.height) * static_cast<size_t>(texData.channels);
        if (pixelCount > static_cast<size_t>(256) * 1024 * 1024)
        {
            KE_LOG_ERROR("Texture too large: {} ({} pixels)", path, pixelCount);
            stbi_image_free(data);
            return INVALID_HANDLE;
        }

        texData.pixels.assign(data, data + pixelCount);
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

    AssetHandle RegisterInternalMesh(const std::string& name, const MeshData& mesh)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_meshPaths.find(name);
        if (it != m_meshPaths.end()) return it->second;
        AssetHandle handle = m_nextHandle++;
        m_meshPaths[name] = handle;
        m_meshes[handle] = mesh;
        return handle;
    }

    AssetHandle CreateTerrain(const std::string& name, int width, int height,
        float cellSize, float maxHeight, const std::vector<float>& heights)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
        {
            KE_LOG_ERROR("Invalid terrain dimensions: {}x{}", width, height);
            return INVALID_HANDLE;
        }
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
        if (boneCount > 128)
        {
            KE_LOG_WARN("Skeleton has {} bones, capping to 128: {}", boneCount, path);
            boneCount = 128;
        }
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

            if (sampler.interpolation == "STEP")
                animChannel.interpolation = AnimationInterpolation::Step;
            else
                animChannel.interpolation = AnimationInterpolation::Linear;

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

        float ticksPerSecond = 30.0f;
        if (!model.animations.empty())
        {
            const auto& sampler = anim.samplers[0];
            const auto& timeAcc = model.accessors[sampler.input];
            const auto& timeBuf = model.bufferViews[timeAcc.bufferView];
            const float* times = reinterpret_cast<const float*>(
                &model.buffers[timeBuf.buffer].data[timeBuf.byteOffset + timeAcc.byteOffset]);
            if (timeAcc.count >= 2 && times[timeAcc.count - 1] > times[0])
            {
                float duration = times[timeAcc.count - 1] - times[0];
                if (duration > 0.0f)
                {
                    float measuredTPS = static_cast<float>(timeAcc.count - 1) / duration;
                    if (measuredTPS > 1.0f && measuredTPS < 1000.0f)
                        ticksPerSecond = measuredTPS;
                }
            }
        }
        clipData.clip.SetTicksPerSecond(ticksPerSecond);

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
        float emissiveStrength = 1.0f;
        std::string albedoTexturePath;
        std::string normalTexturePath;
        std::string metallicRoughnessTexturePath;
        std::string emissiveTexturePath;
        std::string aoTexturePath;
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
        if (mat.emissiveTexture.index >= 0)
            info.emissiveTexturePath = resolveTexturePath(mat.emissiveTexture.index);
        if (mat.occlusionTexture.index >= 0)
            info.aoTexturePath = resolveTexturePath(mat.occlusionTexture.index);

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
                matData.emissiveStrength = matInfo.emissiveStrength;
                matData.albedoTexturePath = matInfo.albedoTexturePath;
                matData.normalTexturePath = matInfo.normalTexturePath;
                matData.metallicRoughnessTexturePath = matInfo.metallicRoughnessTexturePath;
                matData.emissiveTexturePath = matInfo.emissiveTexturePath;
                matData.aoTexturePath = matInfo.aoTexturePath;

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

    AssetHandle LoadFBX(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return LoadFBXInternalLocked(path);
    }

private:
    AssetHandle LoadFBXInternalLocked(const std::string& path)
    {
        if (!FileSystem::ValidatePath(path))
        {
            KE_LOG_ERROR("Path validation failed for FBX: {}", path);
            return INVALID_HANDLE;
        }

        auto it = m_meshPaths.find(path);
        if (it != m_meshPaths.end()) return it->second;

        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path, flags);
        if (!scene || !scene->HasMeshes())
        {
            KE_LOG_ERROR("Assimp failed to load: {} - {}", path, importer.GetErrorString());
            return INVALID_HANDLE;
        }

        MeshData meshData;
        meshData.name = FileSystem::GetFileName(path);

        uint32_t baseIdx = 0;
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            const aiMesh* aiMesh = scene->mMeshes[m];

            for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
            {
                Vertex v{};
                v.position = glm::vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
                if (aiMesh->HasNormals())
                    v.normal = glm::vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
                if (aiMesh->HasTextureCoords(0))
                    v.uv = glm::vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y);
                meshData.vertices.push_back(v);
            }

            for (unsigned int f = 0; f < aiMesh->mNumFaces; ++f)
            {
                const aiFace& face = aiMesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
                    meshData.indices.push_back(baseIdx + face.mIndices[idx]);
            }

            baseIdx = static_cast<uint32_t>(meshData.vertices.size());
        }

        ComputeBounds(meshData.vertices, meshData.boundsMin, meshData.boundsMax);

        if (meshData.vertices.empty())
        {
            KE_LOG_ERROR("FBX mesh has no vertices: {}", path);
            return INVALID_HANDLE;
        }

        AssetHandle handle = m_nextHandle++;
        m_meshPaths[path] = handle;
        m_meshes[handle] = std::move(meshData);

        if (scene->HasMaterials() && scene->mNumMaterials > 0)
        {
            const aiMaterial* aiMat = scene->mMaterials[0];
            MaterialData matData;

            aiColor4D color;
            if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
                matData.albedo = glm::vec3(color.r, color.g, color.b);

            float val;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &val) == AI_SUCCESS)
                matData.metallic = val;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &val) == AI_SUCCESS)
                matData.roughness = val;

            aiString texPath;
            std::string basePath = FileSystem::GetDirectory(path);
            if (aiGetMaterialTexture(aiMat, aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                matData.albedoTexturePath = basePath + texPath.C_Str();
            if (aiGetMaterialTexture(aiMat, aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
                matData.normalTexturePath = basePath + texPath.C_Str();
            if (aiGetMaterialTexture(aiMat, aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
                matData.metallicRoughnessTexturePath = basePath + texPath.C_Str();
            else if (aiGetMaterialTexture(aiMat, aiTextureType_SHININESS, 0, &texPath) == AI_SUCCESS)
                matData.metallicRoughnessTexturePath = basePath + texPath.C_Str();
            if (aiGetMaterialTexture(aiMat, aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS)
                matData.emissiveTexturePath = basePath + texPath.C_Str();
            if (aiGetMaterialTexture(aiMat, aiTextureType_LIGHTMAP, 0, &texPath) == AI_SUCCESS)
                matData.aoTexturePath = basePath + texPath.C_Str();
            else if (aiGetMaterialTexture(aiMat, aiTextureType_AMBIENT_OCCLUSION, 0, &texPath) == AI_SUCCESS)
                matData.aoTexturePath = basePath + texPath.C_Str();

            AssetHandle matHandle = m_nextHandle++;
            m_materials[matHandle] = matData;
            m_meshMaterialMap[handle] = matHandle;
            KE_LOG_INFO("Imported FBX material (albedo: [{:.2f},{:.2f},{:.2f}], metallic: {:.2f}, roughness: {:.2f})",
                matData.albedo.x, matData.albedo.y, matData.albedo.z, matData.metallic, matData.roughness);
        }

        KE_LOG_INFO("Loaded FBX: {} ({} vertices, {} indices, {} submeshes)",
            path, m_meshes[handle].vertices.size(), m_meshes[handle].indices.size(), scene->mNumMeshes);
        return handle;
    }

public:
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

    std::string FindMeshPath(AssetHandle handle) const
    {
        for (const auto& [path, h] : m_meshPaths)
            if (h == handle) return path;
        return "";
    }

    std::string FindSkinnedMeshPath(AssetHandle handle) const
    {
        for (const auto& [path, h] : m_skinnedMeshPaths)
            if (h == handle) return path;
        return "";
    }

    std::string FindSkeletonPath(AssetHandle handle) const
    {
        for (const auto& [path, h] : m_skeletonPaths)
            if (h == handle) return path;
        return "";
    }

    std::string FindAnimationClipPath(AssetHandle handle) const
    {
        for (const auto& [path, h] : m_animationClipPaths)
            if (h == handle) return path;
        return "";
    }

    std::string FindTexturePath(AssetHandle handle) const
    {
        for (const auto& [path, h] : m_texturePaths)
            if (h == handle) return path;
        return "";
    }

    struct AssetStats
    {
        size_t meshCount = 0;
        size_t skinnedMeshCount = 0;
        size_t textureCount = 0;
        size_t materialCount = 0;
        size_t terrainCount = 0;
        size_t skeletonCount = 0;
        size_t animationClipCount = 0;
        size_t totalVertexCount = 0;
        size_t totalIndexCount = 0;
        size_t totalTextureBytes = 0;
    };

    AssetStats GetStats() const
    {
        AssetStats stats;
        stats.meshCount = m_meshes.size();
        stats.skinnedMeshCount = m_skinnedMeshes.size();
        stats.textureCount = m_textures.size();
        stats.materialCount = m_materials.size();
        stats.terrainCount = m_terrains.size();
        stats.skeletonCount = m_skeletons.size();
        stats.animationClipCount = m_animationClips.size();

        for (const auto& [h, mesh] : m_meshes)
        {
            stats.totalVertexCount += mesh.vertices.size();
            stats.totalIndexCount += mesh.indices.size();
        }
        for (const auto& [h, mesh] : m_skinnedMeshes)
        {
            stats.totalVertexCount += mesh.vertices.size();
            stats.totalIndexCount += mesh.indices.size();
        }
        for (const auto& [h, tex] : m_textures)
        {
            stats.totalTextureBytes += tex.pixels.size();
        }
        return stats;
    }

    bool UnloadMesh(AssetHandle handle)
    {
        auto it = m_meshes.find(handle);
        if (it == m_meshes.end()) return false;
        m_meshes.erase(it);
        for (auto pathIt = m_meshPaths.begin(); pathIt != m_meshPaths.end(); ++pathIt)
        {
            if (pathIt->second == handle) { m_meshPaths.erase(pathIt); break; }
        }
        m_meshMaterialMap.erase(handle);
        return true;
    }

    bool UnloadTexture(AssetHandle handle)
    {
        auto it = m_textures.find(handle);
        if (it == m_textures.end()) return false;
        m_textures.erase(it);
        for (auto pathIt = m_texturePaths.begin(); pathIt != m_texturePaths.end(); ++pathIt)
        {
            if (pathIt->second == handle) { m_texturePaths.erase(pathIt); break; }
        }
        return true;
    }

    bool UnloadAnimationClip(AssetHandle handle)
    {
        auto it = m_animationClips.find(handle);
        if (it == m_animationClips.end()) return false;
        m_animationClips.erase(it);
        for (auto pathIt = m_animationClipPaths.begin(); pathIt != m_animationClipPaths.end(); ++pathIt)
        {
            if (pathIt->second == handle) { m_animationClipPaths.erase(pathIt); break; }
        }
        return true;
    }

    std::vector<std::string> ValidateAllAssets() const
    {
        std::vector<std::string> issues;

        for (const auto& [h, mesh] : m_meshes)
        {
            if (mesh.vertices.empty())
                issues.push_back("Mesh handle " + std::to_string(h) + " has no vertices");
            if (mesh.indices.size() % 3 != 0)
                issues.push_back("Mesh handle " + std::to_string(h) + " has non-triangle indices");
            for (size_t i = 0; i < mesh.indices.size(); ++i)
            {
                if (mesh.indices[i] >= mesh.vertices.size())
                {
                    issues.push_back("Mesh handle " + std::to_string(h) + " has out-of-range index at " + std::to_string(i));
                    break;
                }
            }
        }

        for (const auto& [h, mesh] : m_skinnedMeshes)
        {
            if (mesh.vertices.empty())
                issues.push_back("SkinnedMesh handle " + std::to_string(h) + " has no vertices");
            for (const auto& v : mesh.vertices)
            {
                float total = v.boneWeights.x + v.boneWeights.y + v.boneWeights.z + v.boneWeights.w;
                if (total < 0.001f || total > 1.01f)
                {
                    issues.push_back("SkinnedMesh handle " + std::to_string(h) + " has invalid bone weights");
                    break;
                }
            }
        }

        for (const auto& [h, skel] : m_skeletons)
        {
            if (!skel.skeleton.Validate())
                issues.push_back("Skeleton handle " + std::to_string(h) + " validation failed");
        }

        for (const auto& [h, tex] : m_textures)
        {
            if (tex.width <= 0 || tex.height <= 0)
                issues.push_back("Texture handle " + std::to_string(h) + " has invalid dimensions");
            if (tex.pixels.empty())
                issues.push_back("Texture handle " + std::to_string(h) + " has no pixel data");
        }

        return issues;
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
                std::vector<std::string> faceVerts;
                std::string vertStr;
                while (iss >> vertStr)
                    faceVerts.push_back(vertStr);

                if (faceVerts.size() < 3) continue;

                auto parseFace = [](const std::string& face)
                {
                    int v = 0, vt = 0, vn = 0;
                    size_t p1 = face.find('/');
                    if (p1 == std::string::npos)
                    {
                        try { v = std::stoi(face); } catch (...) { v = 0; }
                        return std::make_tuple(v, vt, vn);
                    }
                    try { v = std::stoi(face.substr(0, p1)); } catch (...) { v = 0; }
                    size_t p2 = face.find('/', p1 + 1);
                    if (p2 == std::string::npos)
                    {
                        if (p1 + 1 < face.size())
                        {
                            try { vt = std::stoi(face.substr(p1 + 1)); } catch (...) { vt = 0; }
                        }
                        return std::make_tuple(v, vt, vn);
                    }
                    if (p2 > p1 + 1)
                    {
                        try { vt = std::stoi(face.substr(p1 + 1, p2 - p1 - 1)); } catch (...) { vt = 0; }
                    }
                    if (face.size() > p2 + 1)
                    {
                        try { vn = std::stoi(face.substr(p2 + 1)); } catch (...) { vn = 0; }
                    }
                    return std::make_tuple(v, vt, vn);
                };

                auto addVertex = [&](const std::string& faceStr)
                {
                    auto [iv, ivt, ivn] = parseFace(faceStr);
                    if (iv < 0)
                        iv = static_cast<int>(positions.size()) + iv + 1;
                    if (ivn < 0)
                        ivn = static_cast<int>(normals.size()) + ivn + 1;
                    if (ivt < 0)
                        ivt = static_cast<int>(uvs.size()) + ivt + 1;
                    if (iv <= 0 || iv > static_cast<int>(positions.size()))
                    {
                        KE_LOG_WARN("OBJ face vertex index out of range, using default");
                        outMesh.vertices.push_back(Vertex{});
                        return;
                    }
                    Vertex vtx{};
                    vtx.position = positions[iv - 1];
                    if (ivn > 0 && ivn <= static_cast<int>(normals.size()))
                        vtx.normal = normals[ivn - 1];
                    if (ivt > 0 && ivt <= static_cast<int>(uvs.size()))
                        vtx.uv = uvs[ivt - 1];
                    outMesh.vertices.push_back(vtx);
                };

                for (size_t i = 1; i + 1 < faceVerts.size(); ++i)
                {
                    uint32_t baseIdx = static_cast<uint32_t>(outMesh.vertices.size());
                    addVertex(faceVerts[0]);
                    addVertex(faceVerts[i]);
                    addVertex(faceVerts[i + 1]);
                    outMesh.indices.push_back(baseIdx);
                    outMesh.indices.push_back(baseIdx + 1);
                    outMesh.indices.push_back(baseIdx + 2);
                }
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
            size_t posDataSize = model.buffers[posBuf.buffer].data.size();
            size_t posOffset = posBuf.byteOffset + posAcc.byteOffset;
            size_t requiredPosBytes = posAcc.count * 3 * sizeof(float);
            if (posOffset + requiredPosBytes > posDataSize)
            {
                KE_LOG_ERROR("glTF position buffer out of bounds: {}", path);
                return false;
            }
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posOffset]);

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
        ValidateDegenerateTriangles(outMesh.indices, outMesh.vertices);

        if (outMesh.vertices.empty() || outMesh.indices.empty())
        {
            KE_LOG_ERROR("glTF mesh has no valid geometry: {}", path);
            return false;
        }

        return true;
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
            size_t posDataSize = model.buffers[posBuf.buffer].data.size();
            size_t posOffset = posBuf.byteOffset + posAcc.byteOffset;
            size_t requiredPosBytes = posAcc.count * 3 * sizeof(float);
            if (posOffset + requiredPosBytes > posDataSize)
            {
                KE_LOG_ERROR("glTF skinned position buffer out of bounds: {}", path);
                return false;
            }
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBuf.buffer].data[posOffset]);

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
                size_t wOffset = wBuf.byteOffset + wAcc.byteOffset;
                size_t requiredWBytes = wAcc.count * 4 * sizeof(float);
                if (wOffset + requiredWBytes > model.buffers[wBuf.buffer].data.size())
                {
                    KE_LOG_WARN("glTF WEIGHTS_0 buffer out of bounds, ignoring weights");
                    weightData = nullptr;
                }
                else
                {
                    weightData = reinterpret_cast<const float*>(
                        &model.buffers[wBuf.buffer].data[wOffset]);
                }
            }

            const uint16_t* jointData = nullptr;
            const uint8_t* jointData8 = nullptr;
            int jointComponentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            if (primitive.attributes.count("JOINTS_0") > 0)
            {
                const auto& jAcc = model.accessors[primitive.attributes.at("JOINTS_0")];
                const auto& jBuf = model.bufferViews[jAcc.bufferView];
                jointComponentType = jAcc.componentType;
                size_t jOffset = jBuf.byteOffset + jAcc.byteOffset;
                if (jointComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    size_t requiredJBytes = jAcc.count * 4 * sizeof(uint16_t);
                    if (jOffset + requiredJBytes > model.buffers[jBuf.buffer].data.size())
                    {
                        KE_LOG_WARN("glTF JOINTS_0 buffer out of bounds, ignoring joints");
                        jointData = nullptr;
                    }
                    else
                    {
                        jointData = reinterpret_cast<const uint16_t*>(
                            &model.buffers[jBuf.buffer].data[jOffset]);
                    }
                }
                else
                {
                    size_t requiredJBytes = jAcc.count * 4 * sizeof(uint8_t);
                    if (jOffset + requiredJBytes > model.buffers[jBuf.buffer].data.size())
                    {
                        KE_LOG_WARN("glTF JOINTS_0 buffer out of bounds, ignoring joints");
                        jointData8 = nullptr;
                    }
                    else
                    {
                        jointData8 = reinterpret_cast<const uint8_t*>(
                            &model.buffers[jBuf.buffer].data[jOffset]);
                    }
                }
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

        ComputeBoundsSkinned(outMesh.vertices, outMesh.boundsMin, outMesh.boundsMax);
        ValidateAndFixWeights(outMesh);

        if (outMesh.vertices.empty() || outMesh.indices.empty())
        {
            KE_LOG_ERROR("glTF skinned mesh has no valid geometry: {}", path);
            return false;
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

    void ComputeBoundsSkinned(const std::vector<SkinnedVertex>& vertices, glm::vec3& bMin, glm::vec3& bMax)
    {
        if (vertices.empty()) return;
        bMin = bMax = vertices[0].position;
        for (const auto& v : vertices)
        {
            bMin = glm::min(bMin, v.position);
            bMax = glm::max(bMax, v.position);
        }
    }

    void ValidateAndFixWeights(SkinnedMeshData& mesh)
    {
        for (auto& v : mesh.vertices)
        {
            float total = v.boneWeights.x + v.boneWeights.y + v.boneWeights.z + v.boneWeights.w;
            if (total < 0.001f)
            {
                v.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                v.boneIndices = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
            else if (std::abs(total - 1.0f) > 0.01f)
            {
                v.boneWeights /= total;
            }
        }
    }

    void ValidateDegenerateTriangles(std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices)
    {
        std::vector<uint32_t> cleaned;
        cleaned.reserve(indices.size());
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            if (i0 == i1 || i1 == i2 || i0 == i2) continue;
            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) continue;
            cleaned.push_back(i0);
            cleaned.push_back(i1);
            cleaned.push_back(i2);
        }
        indices = std::move(cleaned);
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
