#pragma once

#include "App/Engine.h"
#include <stb_image.h>
#include <chrono>

namespace Kojeom
{
class GameMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        engine.Run();
        return 0;
    }
};

class SmokeTestMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        int targetFrames = engine.GetConfig().frameLimit;
        if (targetFrames <= 0) targetFrames = 60;

        TestResult result;
        result.mode = "smoke";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        for (int i = 0; i < targetFrames && engine.IsRunning(); ++i)
        {
            engine.TickOneFrame();
            result.frames++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.durationSeconds = std::chrono::duration<float>(endTime - startTime).count();
        result.averageFrameMs = (result.frames > 0) ?
            (result.durationSeconds * 1000.0f / result.frames) : 0.0f;
        result.entities = static_cast<int>(engine.GetWorld()->GetEntityCount());
        result.success = (result.frames == targetFrames);

        if (!result.success)
            result.errors.push_back("Failed to complete all frames");

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Smoke test: {} frames in {:.2f}s ({:.1f} ms/frame)",
            result.frames, result.durationSeconds, result.averageFrameMs);

        return result.success ? 0 : 5;
    }
};

class AssetValidationMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        TestResult result;
        result.mode = "validate-assets";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        auto* assetStore = engine.GetAssetStore();
        auto sceneContent = FileSystem::ReadTextFile(engine.GetConfig().scenePath);
        if (sceneContent.empty())
        {
            result.errors.push_back("Scene file not found or empty: " + engine.GetConfig().scenePath);
            result.success = false;
            if (!engine.GetConfig().resultJsonPath.empty())
                result.WriteToFile(engine.GetConfig().resultJsonPath);
            return 2;
        }

        try
        {
            auto sceneJson = nlohmann::json::parse(sceneContent);

            if (sceneJson.contains("entities"))
            {
                result.entities = static_cast<int>(sceneJson["entities"].size());
                for (const auto& entity : sceneJson["entities"])
                {
                    if (entity.contains("mesh"))
                    {
                        std::string meshPath = entity["mesh"].get<std::string>();
                        if (meshPath == "$default_cube" || meshPath == "$default_plane")
                        {
                            result.loadedAssets++;
                        }
                        else if (!FileSystem::ValidatePath(meshPath))
                        {
                            result.errors.push_back("Invalid mesh path (traversal): " + meshPath);
                        }
                        else if (!FileSystem::FileExists(meshPath))
                        {
                            result.errors.push_back("Mesh not found: " + meshPath);
                        }
                        else
                        {
                            result.loadedAssets++;
                        }
                    }
                    if (entity.contains("texture"))
                    {
                        std::string texPath = entity["texture"].get<std::string>();
                        if (!FileSystem::ValidatePath(texPath))
                        {
                            result.errors.push_back("Invalid texture path (traversal): " + texPath);
                        }
                        else if (!FileSystem::FileExists(texPath))
                        {
                            result.errors.push_back("Texture not found: " + texPath);
                        }
                        else
                        {
                            result.loadedAssets++;
                        }
                    }
                    if (entity.contains("material"))
                    {
                        const auto& mat = entity["material"];
                        if (mat.contains("albedoTexture"))
                        {
                            std::string texPath = mat["albedoTexture"].get<std::string>();
                            if (!FileSystem::ValidatePath(texPath))
                                result.errors.push_back("Invalid albedo texture path: " + texPath);
                            else if (!FileSystem::FileExists(texPath))
                                result.errors.push_back("Albedo texture not found: " + texPath);
                        }
                        if (mat.contains("normalTexture"))
                        {
                            std::string texPath = mat["normalTexture"].get<std::string>();
                            if (!FileSystem::ValidatePath(texPath))
                                result.errors.push_back("Invalid normal texture path: " + texPath);
                            else if (!FileSystem::FileExists(texPath))
                                result.errors.push_back("Normal texture not found: " + texPath);
                        }
                        if (mat.contains("metallicRoughnessTexture"))
                        {
                            std::string texPath = mat["metallicRoughnessTexture"].get<std::string>();
                            if (!FileSystem::ValidatePath(texPath))
                                result.errors.push_back("Invalid metallic-roughness texture path: " + texPath);
                            else if (!FileSystem::FileExists(texPath))
                                result.errors.push_back("Metallic-roughness texture not found: " + texPath);
                        }
                    }
                    if (entity.contains("skeleton"))
                    {
                        std::string skelPath = entity["skeleton"].get<std::string>();
                        if (!FileSystem::ValidatePath(skelPath))
                            result.errors.push_back("Invalid skeleton path: " + skelPath);
                        else if (!FileSystem::FileExists(skelPath))
                            result.errors.push_back("Skeleton not found: " + skelPath);
                    }
                    if (entity.contains("skinnedMesh"))
                    {
                        std::string meshPath = entity["skinnedMesh"].get<std::string>();
                        if (!FileSystem::ValidatePath(meshPath))
                            result.errors.push_back("Invalid skinned mesh path: " + meshPath);
                        else if (!FileSystem::FileExists(meshPath))
                            result.errors.push_back("Skinned mesh not found: " + meshPath);
                    }
                    if (entity.contains("animationClip"))
                    {
                        std::string clipPath = entity["animationClip"].get<std::string>();
                        if (!FileSystem::ValidatePath(clipPath))
                            result.errors.push_back("Invalid animation clip path: " + clipPath);
                        else if (!FileSystem::FileExists(clipPath))
                            result.errors.push_back("Animation clip not found: " + clipPath);
                    }
                    if (entity.contains("terrain"))
                    {
                        const auto& terrainData = entity["terrain"];
                        if (terrainData.contains("heightmapImage"))
                        {
                            std::string hmPath = terrainData["heightmapImage"].get<std::string>();
                            if (!FileSystem::ValidatePath(hmPath))
                                result.errors.push_back("Invalid heightmap path: " + hmPath);
                            else if (!FileSystem::FileExists(hmPath))
                                result.errors.push_back("Heightmap not found: " + hmPath);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            result.errors.push_back(std::string("JSON parse error: ") + e.what());
        }

        result.success = result.errors.empty();

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Asset validation: {} entities, {} assets, {} errors",
            result.entities, result.loadedAssets, result.errors.size());

        return result.success ? 0 : 3;
    }
};

class RenderTestMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        int targetFrames = engine.GetConfig().frameLimit;
        if (targetFrames <= 0) targetFrames = 3;

        TestResult result;
        result.mode = "render-test";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        for (int i = 0; i < targetFrames && engine.IsRunning(); ++i)
        {
            engine.TickOneFrame();
            result.frames++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.durationSeconds = std::chrono::duration<float>(endTime - startTime).count();
        result.averageFrameMs = (result.frames > 0) ?
            (result.durationSeconds * 1000.0f / result.frames) : 0.0f;
        result.entities = static_cast<int>(engine.GetWorld()->GetEntityCount());

        RenderScene scene = engine.GetWorld()->BuildRenderScene();
        result.drawCalls = static_cast<int>(scene.TotalDrawCommands());
        result.success = (result.frames == targetFrames && result.drawCalls > 0);

        if (!engine.GetConfig().screenshotPath.empty())
        {
            int width, height;
            engine.GetWindow()->GetSize(width, height);
            bool saved = engine.GetRenderer()->SaveScreenshot(
                engine.GetConfig().screenshotPath, width, height);
            if (!saved)
            {
                result.errors.push_back("Failed to save screenshot");
                result.success = false;
            }
        }

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Render test: {} frames, {} draw calls", result.frames, result.drawCalls);
        return result.success ? 0 : 6;
    }
};

class SceneDumpMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        TestResult result;
        result.mode = "scene-dump";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        engine.TickOneFrame();

        RenderScene scene = engine.GetWorld()->BuildRenderScene();
        result.entities = static_cast<int>(engine.GetWorld()->GetEntityCount());
        result.drawCalls = static_cast<int>(scene.TotalDrawCommands());

        nlohmann::json dump;
        dump["staticDrawCommands"] = scene.staticDrawCommands.size();
        dump["skinnedDrawCommands"] = scene.skinnedDrawCommands.size();
        dump["terrainDrawCommands"] = scene.terrainDrawCommands.size();
        dump["camera"]["valid"] = scene.camera.viewProjectionMatrix != Mat4(1.0f);
        dump["camera"]["position"] = { scene.camera.position.x, scene.camera.position.y, scene.camera.position.z };
        dump["camera"]["forward"] = { scene.camera.forward.x, scene.camera.forward.y, scene.camera.forward.z };
        dump["entities"] = result.entities;
        dump["totalDrawCalls"] = result.drawCalls;

        nlohmann::json entityList = nlohmann::json::array();
        for (const auto& entity : engine.GetWorld()->GetEntities())
        {
            nlohmann::json e;
            e["name"] = entity->GetName();
            auto* transform = entity->GetTransform();
            e["position"] = { transform->transform.position.x, transform->transform.position.y, transform->transform.position.z };
            auto* mr = entity->GetMeshRendererComponent();
            if (mr && mr->meshHandle != INVALID_HANDLE)
                e["meshHandle"] = mr->meshHandle;
            auto* tc = entity->GetTerrainComponent();
            if (tc && tc->terrainHandle != INVALID_HANDLE)
                e["terrainHandle"] = tc->terrainHandle;
            auto* sm = entity->GetSkeletalMeshComponent();
            if (sm && sm->skeletalMeshHandle != INVALID_HANDLE)
                e["skeletalMeshHandle"] = sm->skeletalMeshHandle;
            entityList.push_back(e);
        }
        dump["entityDetails"] = entityList;

        nlohmann::json lightInfo;
        lightInfo["direction"] = { scene.light.direction.x, scene.light.direction.y, scene.light.direction.z };
        lightInfo["color"] = { scene.light.color.x, scene.light.color.y, scene.light.color.z };
        lightInfo["intensity"] = scene.light.intensity;
        lightInfo["pointLightCount"] = scene.light.pointLightCount;
        dump["light"] = lightInfo;

        std::string dumpPath = engine.GetConfig().resultJsonPath;
        if (dumpPath.empty()) dumpPath = "scene_dump.json";

        FileSystem::WriteTextFile(dumpPath, dump.dump(2));

        result.success = true;
        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Scene dump: {} static, {} skinned, {} terrain commands",
            scene.staticDrawCommands.size(), scene.skinnedDrawCommands.size(),
            scene.terrainDrawCommands.size());

        return 0;
    }
};

class ScreenshotCompareMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        TestResult result;
        result.mode = "screenshot-compare";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        int targetFrames = engine.GetConfig().frameLimit;
        if (targetFrames <= 0) targetFrames = 3;

        for (int i = 0; i < targetFrames && engine.IsRunning(); ++i)
        {
            engine.TickOneFrame();
            result.frames++;
        }

        int width, height;
        engine.GetWindow()->GetSize(width, height);

        std::string screenshotPath = engine.GetConfig().screenshotPath;
        if (screenshotPath.empty()) screenshotPath = "screenshot_compare.png";

        bool saved = engine.GetRenderer()->SaveScreenshot(screenshotPath, width, height);
        if (!saved)
        {
            result.errors.push_back("Failed to save screenshot for comparison");
            result.success = false;
            if (!engine.GetConfig().resultJsonPath.empty())
                result.WriteToFile(engine.GetConfig().resultJsonPath);
            return 6;
        }

        result.success = true;

        if (FileSystem::FileExists(screenshotPath))
        {
            int imgW, imgH, imgChannels;
            uint8_t* pixels = stbi_load(screenshotPath.c_str(), &imgW, &imgH, &imgChannels, 3);
            if (pixels && imgW > 0 && imgH > 0)
            {
                bool allBlack = true;
                size_t pixelCount = static_cast<size_t>(imgW) * imgH * 3;
                double totalBrightness = 0.0;
                size_t sampleCount = 0;
                for (size_t i = 0; i < pixelCount; i += 3)
                {
                    uint8_t r = pixels[i], g = pixels[i + 1], b = pixels[i + 2];
                    if (r != 0 || g != 0 || b != 0)
                        allBlack = false;
                    totalBrightness += (r * 0.299 + g * 0.587 + b * 0.114);
                    sampleCount++;
                }
                double avgBrightness = (sampleCount > 0) ? (totalBrightness / sampleCount) : 0.0;
                if (allBlack)
                {
                    result.warnings.push_back("Screenshot appears to be completely black");
                }
                else if (avgBrightness < 5.0)
                {
                    result.warnings.push_back("Screenshot is very dark (avg brightness: " +
                        std::to_string(static_cast<int>(avgBrightness)) + "/255)");
                }
                stbi_image_free(pixels);
            }
            else
            {
                result.warnings.push_back("Could not decode screenshot for black-check");
            }
        }

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Screenshot compare: {} frames rendered, screenshot saved to {}",
            result.frames, screenshotPath);
        return result.success ? 0 : 6;
    }
};

class BenchmarkMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        int targetFrames = engine.GetConfig().frameLimit;
        if (targetFrames <= 0) targetFrames = 600;

        TestResult result;
        result.mode = "benchmark";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        float minFrameMs = std::numeric_limits<float>::max();
        float maxFrameMs = 0.0f;

        for (int i = 0; i < targetFrames && engine.IsRunning(); ++i)
        {
            auto frameStart = std::chrono::high_resolution_clock::now();
            engine.TickOneFrame();
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float frameMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            minFrameMs = std::min(minFrameMs, frameMs);
            maxFrameMs = std::max(maxFrameMs, frameMs);
            result.frames++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.durationSeconds = std::chrono::duration<float>(endTime - startTime).count();
        result.averageFrameMs = (result.frames > 0) ?
            (result.durationSeconds * 1000.0f / result.frames) : 0.0f;
        result.entities = static_cast<int>(engine.GetWorld()->GetEntityCount());
        result.success = (result.frames == targetFrames);

        if (!engine.GetConfig().resultJsonPath.empty())
        {
            auto j = result.ToJson();
            j["minFrameMs"] = minFrameMs;
            j["maxFrameMs"] = maxFrameMs;
            FileSystem::WriteTextFile(engine.GetConfig().resultJsonPath, j.dump(2));
        }

        KE_LOG_INFO("Benchmark: {} frames in {:.2f}s ({:.1f} ms/frame, {:.1f} FPS, min={:.1f}ms, max={:.1f}ms)",
            result.frames, result.durationSeconds, result.averageFrameMs,
            result.frames / result.durationSeconds, minFrameMs, maxFrameMs);

        return result.success ? 0 : 5;
    }
};

class MaterialDumpMode : public IAppMode
{
public:
    int Run(Engine& engine) override
    {
        TestResult result;
        result.mode = "material-dump";
        result.backend = engine.GetConfig().backend;
        result.scene = engine.GetConfig().scenePath;

        engine.TickOneFrame();

        auto* assetStore = engine.GetAssetStore();
        auto stats = assetStore->GetStats();

        nlohmann::json matDump;
        matDump["totalMaterials"] = stats.materialCount;
        matDump["totalTextures"] = stats.textureCount;

        nlohmann::json materials = nlohmann::json::array();

        for (const auto& entity : engine.GetWorld()->GetEntities())
        {
            auto* mr = entity->GetMeshRendererComponent();
            AssetHandle matHandle = INVALID_HANDLE;
            if (mr) matHandle = mr->materialHandle;

            auto* sm = entity->GetSkeletalMeshComponent();
            if (sm && sm->materialHandle != INVALID_HANDLE)
                matHandle = sm->materialHandle;

            if (matHandle == INVALID_HANDLE) continue;

            auto* matData = assetStore->GetMaterial(matHandle);
            if (!matData) continue;

            nlohmann::json matJson;
            matJson["entity"] = entity->GetName();
            matJson["handle"] = matHandle;
            matJson["albedo"] = { matData->albedo.x, matData->albedo.y, matData->albedo.z };
            matJson["metallic"] = matData->metallic;
            matJson["roughness"] = matData->roughness;
            matJson["ao"] = matData->ao;
            matJson["emissive"] = { matData->emissive.x, matData->emissive.y, matData->emissive.z };
            if (!matData->albedoTexturePath.empty())
                matJson["albedoTexture"] = matData->albedoTexturePath;
            if (!matData->normalTexturePath.empty())
                matJson["normalTexture"] = matData->normalTexturePath;
            if (!matData->metallicRoughnessTexturePath.empty())
                matJson["metallicRoughnessTexture"] = matData->metallicRoughnessTexturePath;
            materials.push_back(matJson);
        }

        matDump["materials"] = materials;
        result.entities = static_cast<int>(engine.GetWorld()->GetEntityCount());
        result.loadedAssets = static_cast<int>(stats.materialCount);
        result.success = true;

        std::string dumpPath = engine.GetConfig().resultJsonPath;
        if (dumpPath.empty()) dumpPath = "material_dump.json";
        FileSystem::WriteTextFile(dumpPath, matDump.dump(2));

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Material dump: {} materials for {} entities",
            stats.materialCount, result.entities);
        return 0;
    }
};

inline std::unique_ptr<IAppMode> CreateAppMode(AppConfig::Mode mode)
{
    switch (mode)
    {
    case AppConfig::Mode::Game: return std::make_unique<GameMode>();
    case AppConfig::Mode::Smoke: return std::make_unique<SmokeTestMode>();
    case AppConfig::Mode::ValidateAssets: return std::make_unique<AssetValidationMode>();
    case AppConfig::Mode::RenderTest: return std::make_unique<RenderTestMode>();
    case AppConfig::Mode::SceneDump: return std::make_unique<SceneDumpMode>();
    case AppConfig::Mode::ScreenshotCompare: return std::make_unique<ScreenshotCompareMode>();
    case AppConfig::Mode::Benchmark: return std::make_unique<BenchmarkMode>();
    case AppConfig::Mode::MaterialDump: return std::make_unique<MaterialDumpMode>();
    }
    return std::make_unique<GameMode>();
}
}
