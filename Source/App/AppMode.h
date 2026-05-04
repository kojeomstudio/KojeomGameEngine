#pragma once

#include <glad/gl.h>
#include "App/Engine.h"
#include <chrono>
#include <stb_image_write.h>

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
                        if (!FileSystem::FileExists(meshPath))
                            result.errors.push_back("Mesh not found: " + meshPath);
                        else
                            result.loadedAssets++;
                    }
                    if (entity.contains("texture"))
                    {
                        std::string texPath = entity["texture"].get<std::string>();
                        if (!FileSystem::FileExists(texPath))
                            result.errors.push_back("Texture not found: " + texPath);
                        else
                            result.loadedAssets++;
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
            result.success = SaveScreenshot(engine, engine.GetConfig().screenshotPath) && result.success;
        }

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Render test: {} frames, {} draw calls", result.frames, result.drawCalls);
        return result.success ? 0 : 6;
    }

private:
    bool SaveScreenshot(Engine& engine, const std::string& path)
    {
        int width, height;
        engine.GetWindow()->GetSize(width, height);
        if (width <= 0 || height <= 0) return false;

        std::vector<uint8_t> pixels(width * height * 3);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        stbi_flip_vertically_on_write(1);
        int res = stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3);
        return res != 0;
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

        if (!engine.GetConfig().resultJsonPath.empty())
            result.WriteToFile(engine.GetConfig().resultJsonPath);

        KE_LOG_INFO("Benchmark: {} frames in {:.2f}s ({:.1f} ms/frame, {:.1f} FPS)",
            result.frames, result.durationSeconds, result.averageFrameMs,
            result.frames / result.durationSeconds);

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
