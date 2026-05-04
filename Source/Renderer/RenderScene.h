#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "Math/Transform.h"

namespace Kojeom
{
using AssetHandle = uint64_t;
constexpr AssetHandle INVALID_HANDLE = 0;

struct LightData
{
    Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
    Vec3 color = Vec3(1.0f);
    float intensity = 1.0f;
    Vec3 ambientColor = Vec3(0.1f);
};

struct StaticDrawCommand
{
    AssetHandle meshHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;
    Mat4 worldMatrix = Mat4(1.0f);
};

struct SkinnedDrawCommand
{
    AssetHandle skeletalMeshHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;
    Mat4 worldMatrix = Mat4(1.0f);
    AssetHandle boneMatricesHandle = INVALID_HANDLE;
    uint32_t boneCount = 0;
};

struct TerrainDrawCommand
{
    AssetHandle terrainHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;
    Mat4 worldMatrix = Mat4(1.0f);
};

struct RenderScene
{
    CameraData camera;
    LightData light;
    std::vector<StaticDrawCommand> staticDrawCommands;
    std::vector<SkinnedDrawCommand> skinnedDrawCommands;
    std::vector<TerrainDrawCommand> terrainDrawCommands;

    void Clear()
    {
        staticDrawCommands.clear();
        skinnedDrawCommands.clear();
        terrainDrawCommands.clear();
    }

    size_t TotalDrawCommands() const
    {
        return staticDrawCommands.size() + skinnedDrawCommands.size() + terrainDrawCommands.size();
    }
};
}
