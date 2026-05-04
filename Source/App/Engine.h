#pragma once

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Core/AppConfig.h"
#include "Core/Log.h"
#include "Platform/GlfwWindow.h"
#include "Platform/GlfwInput.h"
#include "Renderer/Renderer.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"
#include "Assets/AssetStore.h"
#include "World/World.h"
#include "App/Clock.h"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace Kojeom
{
class Engine;

class IAppMode
{
public:
    virtual ~IAppMode() = default;
    virtual int Run(Engine& engine) = 0;
};

struct TestResult
{
    bool success = false;
    std::string mode;
    std::string backend;
    std::string scene;
    int frames = 0;
    float durationSeconds = 0.0f;
    float averageFrameMs = 0.0f;
    int entities = 0;
    int drawCalls = 0;
    int loadedAssets = 0;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    nlohmann::json ToJson() const
    {
        nlohmann::json j;
        j["success"] = success;
        j["mode"] = mode;
        j["backend"] = backend;
        j["scene"] = scene;
        j["frames"] = frames;
        j["durationSeconds"] = durationSeconds;
        j["averageFrameMs"] = averageFrameMs;
        j["entities"] = entities;
        j["drawCalls"] = drawCalls;
        j["loadedAssets"] = loadedAssets;
        j["errors"] = errors;
        j["warnings"] = warnings;
        return j;
    }

    bool WriteToFile(const std::string& path) const
    {
        auto json = ToJson().dump(2);
        return FileSystem::WriteTextFile(path, json);
    }
};

class Engine
{
public:
    Engine() = default;
    ~Engine() { Shutdown(); }

    bool Initialize(const AppConfig& config)
    {
        m_config = config;

        m_window = std::make_unique<GlfwWindow>();
        WindowDesc desc;
        desc.width = config.windowWidth;
        desc.height = config.windowHeight;
        desc.title = config.windowTitle;
        desc.hidden = config.hiddenWindow;

        if (!m_window->Create(desc))
        {
            KE_LOG_ERROR("Failed to create window");
            return false;
        }

        m_input = std::make_unique<GlfwInput>();

        auto* glfwWin = static_cast<GlfwWindow*>(m_window.get());
        glfwWin->SetKeyCallback([](GLFWwindow* w, int key, int scancode, int action, int mods)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));
            if (engine) engine->GetInput()->OnKeyEvent(key, scancode, action, mods);
        });
        glfwWin->SetCursorPosCallback([](GLFWwindow* w, double xpos, double ypos)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));
            if (engine) engine->GetInput()->OnMouseMoveEvent(xpos, ypos);
        });
        glfwWin->SetMouseButtonCallback([](GLFWwindow* w, int button, int action, int mods)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));
            if (engine) engine->GetInput()->OnMouseButtonEvent(button, action, mods);
        });
        glfwWin->SetFramebufferSizeCallback([](GLFWwindow* w, int width, int height)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));
            if (engine && engine->GetRenderer()) engine->GetRenderer()->OnResize(width, height);
        });

        glfwSetWindowUserPointer(glfwWin->GetGLFWWindow(), this);

        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(glfwWin->GetGLFWWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        auto rendererBackend = std::make_unique<OpenGLRenderer>();
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Initialize(std::move(rendererBackend), glfwWin->GetGLFWWindow()))
        {
            KE_LOG_ERROR("Failed to initialize renderer");
            return false;
        }

        m_assetStore = std::make_unique<AssetStore>();
        m_world = std::make_unique<World>();

        m_clock.Reset();
        m_running = true;

        KE_LOG_INFO("Engine initialized successfully");
        return true;
    }

    void Shutdown()
    {
        m_running = false;
        m_world.reset();
        m_assetStore.reset();
        m_renderer.reset();
        m_input.reset();
        m_window.reset();
        KE_LOG_INFO("Engine shutdown complete");
    }

    float TickOneFrame()
    {
        float delta = m_clock.Tick();

        m_input->BeginFrame();
        m_window->PollEvents();

        if (m_window->ShouldClose())
        {
            m_running = false;
            return delta;
        }

        m_world->Tick(delta);

        RenderScene scene = m_world->BuildRenderScene();
        m_renderer->Render(scene);

        m_window->SwapBuffers();

        return delta;
    }

    void Run()
    {
        while (m_running)
        {
            float delta = TickOneFrame();
            if (delta > 0.5f) continue;

            if (m_input->GetState().IsKeyPressed(KeyCode::Escape))
            {
                m_running = false;
            }
        }
    }

    bool IsRunning() const { return m_running; }
    void Stop() { m_running = false; }

    IWindow* GetWindow() { return m_window.get(); }
    IInput* GetInput() { return m_input.get(); }
    Renderer* GetRenderer() { return m_renderer.get(); }
    AssetStore* GetAssetStore() { return m_assetStore.get(); }
    World* GetWorld() { return m_world.get(); }
    Clock& GetClock() { return m_clock; }
    const AppConfig& GetConfig() const { return m_config; }

private:
    AppConfig m_config;
    std::unique_ptr<GlfwWindow> m_window;
    std::unique_ptr<GlfwInput> m_input;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<AssetStore> m_assetStore;
    std::unique_ptr<World> m_world;
    Clock m_clock;
    bool m_running = false;
};
}
