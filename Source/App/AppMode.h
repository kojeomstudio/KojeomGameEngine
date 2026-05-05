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
                        if (!FileSystem::ValidatePath(meshPath))
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
        dump["entities"] = result.entities;
        dump["totalDrawCalls"] = result.drawCalls;

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
                for (size_t i = 0; i < pixelCount; i += 3)
                {
                    if (pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0)
                    {
                        allBlack = false;
                        break;
                    }
                }
                if (allBlack)
                {
                    result.warnings.push_back("Screenshot appears to be completely black");
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

inline std::unique_ptr<IAppMode> CreateAppMode(AppConfig::Mode mode)
{
    switch (mode)
    {
    case AppConfig::Mode::Game: return std::make_unique<GameMode>();
    case AppConfig::Mode::Smoke: return std::make_unique<SmokeTestMode>();
    case AppConfig::Mode::ValidateAssets: return std::make_unique<AssetValidationMode>();
    case AppConfig::Mode::RenderTest: return std::make_unique<RenderTestMode>();
    case AppConfig::Mode::SceneDump: return std::make_unique<SceneDumpMode>();
    case AppConfig::Mode::Benchmark: return std::make_unique<BenchmarkMode>();
    }
    return std::make_unique<GameMode>();
}
}
