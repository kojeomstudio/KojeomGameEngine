#pragma once

#include <string>
#include <vector>
#include <cstring>
#include "Core/AppConfig.h"
#include "Core/Log.h"

namespace Kojeom
{
class CommandLine
{
public:
    static AppConfig Parse(int argc, char** argv)
    {
        AppConfig config;
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h")
            {
                PrintHelp();
                config.mode = AppConfig::Mode::Game;
                return config;
            }
            else if (arg == "--version")
            {
                KE_LOG_INFO("KojeomEngine version 0.1.0");
                return config;
            }
            else if (arg == "--mode" && i + 1 < argc)
            {
                config.mode = AppConfig::StringToMode(argv[++i]);
            }
            else if (arg == "--project" && i + 1 < argc)
            {
                config.projectPath = argv[++i];
            }
            else if (arg == "--scene" && i + 1 < argc)
            {
                config.scenePath = argv[++i];
            }
            else if (arg == "--backend" && i + 1 < argc)
            {
                config.backend = argv[++i];
            }
            else if (arg == "--frames" && i + 1 < argc)
            {
                config.frameLimit = std::stoi(argv[++i]);
            }
            else if (arg == "--headless")
            {
                config.headless = true;
                config.hiddenWindow = true;
            }
            else if (arg == "--hidden-window")
            {
                config.hiddenWindow = true;
            }
            else if (arg == "--screenshot" && i + 1 < argc)
            {
                config.screenshotPath = argv[++i];
            }
            else if (arg == "--result-json" && i + 1 < argc)
            {
                config.resultJsonPath = argv[++i];
            }
            else if (arg == "--log-file" && i + 1 < argc)
            {
                config.logFilePath = argv[++i];
            }
            else if (arg == "--width" && i + 1 < argc)
            {
                config.windowWidth = std::stoi(argv[++i]);
            }
            else if (arg == "--height" && i + 1 < argc)
            {
                config.windowHeight = std::stoi(argv[++i]);
            }
        }
        return config;
    }

    static void PrintHelp()
    {
        const char* help = R"(
KojeomEngine - Minimal 3D Game Engine

Usage: Simple3DEngine [options]

Options:
  --help                Show this help message
  --version             Show version
  --project <path>      Project directory path
  --scene <path>        Scene file path
  --backend <name>      Renderer backend (opengl, default: opengl)
  --mode <mode>         Run mode: game, smoke, validate-assets, render-test, scene-dump, benchmark
  --frames <count>      Number of frames to run (0 = unlimited)
  --headless            Run in headless mode (hidden window, no display)
  --hidden-window       Create hidden window
  --screenshot <path>   Save screenshot to file (render-test mode)
  --result-json <path>  Save test results as JSON
  --log-file <path>     Log file path (default: engine.log)
  --width <pixels>      Window width (default: 1280)
  --height <pixels>     Window height (default: 720)
)";
        KE_LOG_INFO("{}", help);
    }
};
}
