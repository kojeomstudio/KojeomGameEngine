#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "Core/Log.h"

namespace Kojeom
{
struct AppConfig
{
    enum class Mode
    {
        Game,
        Smoke,
        ValidateAssets,
        RenderTest,
        SceneDump,
        Benchmark
    };

    Mode mode = Mode::Game;
    std::string projectPath = ".";
    std::string scenePath = "Scenes/TestScene.json";
    std::string backend = "opengl";
    int frameLimit = 0;
    bool headless = false;
    bool hiddenWindow = false;
    std::string screenshotPath;
    std::string resultJsonPath;
    std::string logFilePath = "engine.log";
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "KojeomEngine";

    static std::string ModeToString(Mode m)
    {
        switch (m)
        {
        case Mode::Game: return "game";
        case Mode::Smoke: return "smoke";
        case Mode::ValidateAssets: return "validate-assets";
        case Mode::RenderTest: return "render-test";
        case Mode::SceneDump: return "scene-dump";
        case Mode::Benchmark: return "benchmark";
        }
        return "unknown";
    }

    static Mode StringToMode(const std::string& s)
    {
        if (s == "game") return Mode::Game;
        if (s == "smoke") return Mode::Smoke;
        if (s == "validate-assets") return Mode::ValidateAssets;
        if (s == "render-test") return Mode::RenderTest;
        if (s == "scene-dump") return Mode::SceneDump;
        if (s == "benchmark") return Mode::Benchmark;
        return Mode::Game;
    }
};
}
