#include "Core/Log.h"
#include "Core/AppConfig.h"
#include "Core/FileSystem.h"
#include "App/CommandLine.h"
#include "App/Engine.h"
#include "App/AppMode.h"
#include "Game/SampleGame.h"

#include <iostream>

int main(int argc, char** argv)
{
    Kojeom::AppConfig config = Kojeom::CommandLine::Parse(argc, argv);

    Kojeom::Log::Init(config.logFilePath);
    KE_LOG_INFO("KojeomEngine starting in mode: {}", Kojeom::AppConfig::ModeToString(config.mode));

    Kojeom::Engine engine;
    if (!engine.Initialize(config))
    {
        KE_LOG_ERROR("Engine initialization failed");
        return 4;
    }

    bool useSceneFile = !config.scenePath.empty() &&
        Kojeom::FileSystem::FileExists(config.scenePath);

    Kojeom::SampleGame game;
    game.OnStart(engine);

    if (useSceneFile)
    {
        if (engine.LoadScene(config.scenePath))
            KE_LOG_INFO("Loaded scene: {}", config.scenePath);
        else
            KE_LOG_WARN("Failed to load scene: {}", config.scenePath);
    }

    if (config.mode == Kojeom::AppConfig::Mode::Game)
    {
        while (engine.IsRunning())
        {
            float delta = engine.TickOneFrame();
            game.OnUpdate(engine, delta);
            if (engine.GetInput()->GetState().IsKeyPressed(Kojeom::KeyCode::Escape))
                engine.Stop();
        }
    }
    else
    {
        auto appMode = Kojeom::CreateAppMode(config.mode);
        int exitCode = appMode->Run(engine);
        game.OnStop();
        engine.Shutdown();
        KE_LOG_INFO("Engine exited with code: {}", exitCode);
        return exitCode;
    }

    game.OnStop();
    engine.Shutdown();
    KE_LOG_INFO("Engine exited normally");
    return 0;
}
