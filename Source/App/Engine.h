#pragma once

#include "Core/AppConfig.h"
#include "Core/Log.h"
#include "Core/FileSystem.h"
#include "Platform/GlfwWindow.h"
#include "Platform/GlfwInput.h"
#include "Renderer/Renderer.h"
#include "Assets/AssetStore.h"
#include "World/World.h"
#include "App/Clock.h"
#include <memory>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace Kojeom
{
std::unique_ptr<IRendererBackend> CreateOpenGLBackend();

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
        auto parentDir = FileSystem::GetDirectory(path);
        if (!parentDir.empty())
            FileSystem::CreateDirectory(parentDir);
        auto json = ToJson().dump(2);
        return FileSystem::WriteTextFile(path, json);
    }
};

class Engine
{
public:
    Engine() = default;
    ~Engine() { if (m_running) Shutdown(); }

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

        m_renderer = std::make_unique<Renderer>();
        auto backend = CreateRendererBackend(config.backend);
        if (!m_renderer->Initialize(std::move(backend), glfwWin->GetGLFWWindow()))
        {
            KE_LOG_ERROR("Failed to initialize renderer");
            return false;
        }

        m_assetStore = std::make_unique<AssetStore>();
        m_world = std::make_unique<World>();
        m_world->SetAssetStore(m_assetStore.get());

        m_clock.Reset();
        m_running = true;

        KE_LOG_INFO("Engine initialized successfully");
        return true;
    }

    bool InitializeHeadless(const AppConfig& config)
    {
        m_config = config;
        m_assetStore = std::make_unique<AssetStore>();
        m_world = std::make_unique<World>();
        m_world->SetAssetStore(m_assetStore.get());
        m_running = true;

        KE_LOG_INFO("Engine initialized in headless mode");
        return true;
    }

    void Shutdown()
    {
        if (!m_running && !m_window && !m_renderer) return;
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

        if (m_input) m_input->BeginFrame();
        if (m_window) m_window->PollEvents();

        if (m_window && m_window->ShouldClose())
        {
            m_running = false;
            return delta;
        }

        if (m_world) m_world->Tick(delta);

        UploadAnimatorBoneMatrices();

        if (m_renderer && m_world)
        {
            RenderScene scene = m_world->BuildRenderScene();
            m_renderer->Render(scene);
        }

        if (m_window) m_window->SwapBuffers();

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

    bool LoadScene(const std::string& scenePath)
    {
        if (!m_world || !m_assetStore || !m_renderer) return false;
        auto result = m_world->LoadFromJson(scenePath, m_assetStore.get(), m_renderer.get());
        if (!result.success)
        {
            for (const auto& err : result.errors)
                KE_LOG_ERROR("Scene load error: {}", err);
            return false;
        }
        UploadLoadedMeshesToGPU();
        return true;
    }

    void UploadLoadedMeshesToGPU()
    {
        for (const auto& entity : m_world->GetEntities())
        {
            auto* mr = entity->GetMeshRendererComponent();
            if (!mr || mr->meshHandle == INVALID_HANDLE) continue;

            auto* meshData = m_assetStore->GetMesh(mr->meshHandle);
            if (!meshData) continue;

            std::vector<float> flatVerts;
            flatVerts.reserve(meshData->vertices.size() * 8);
            for (const auto& v : meshData->vertices)
            {
                flatVerts.insert(flatVerts.end(),
                    { v.position.x, v.position.y, v.position.z,
                      v.normal.x, v.normal.y, v.normal.z,
                      v.uv.x, v.uv.y });
            }

            AssetHandle gpuHandle = m_renderer->UploadMesh(flatVerts, meshData->indices);
            mr->meshHandle = gpuHandle;

            if (mr->materialHandle != INVALID_HANDLE)
            {
                auto* matData = m_assetStore->GetMaterial(mr->materialHandle);
                if (matData)
                {
                    AssetHandle albedoTexHandle = INVALID_HANDLE;
                    AssetHandle normalTexHandle = INVALID_HANDLE;
                    bool hasTex = false;
                    bool hasNormalTex = false;
                    if (!matData->albedoTexturePath.empty())
                    {
                        auto texHandle = m_assetStore->LoadTexture(matData->albedoTexturePath);
                        auto* texData = m_assetStore->GetTexture(texHandle);
                        if (texData)
                        {
                            albedoTexHandle = m_renderer->UploadTexture(
                                texData->width, texData->height, texData->channels, texData->pixels.data());
                            hasTex = true;
                        }
                    }
                    if (!matData->normalTexturePath.empty())
                    {
                        auto texHandle = m_assetStore->LoadTexture(matData->normalTexturePath);
                        auto* texData = m_assetStore->GetTexture(texHandle);
                        if (texData)
                        {
                            normalTexHandle = m_renderer->UploadTexture(
                                texData->width, texData->height, texData->channels, texData->pixels.data());
                            hasNormalTex = true;
                        }
                    }
                    mr->materialHandle = m_renderer->RegisterMaterial(
                        matData->albedo, matData->metallic, matData->roughness,
                        albedoTexHandle, hasTex, normalTexHandle, hasNormalTex,
                        matData->emissive);
                }
            }
        }

        for (const auto& entity : m_world->GetEntities())
        {
            auto* tc = entity->GetTerrainComponent();
            if (!tc || tc->terrainHandle == INVALID_HANDLE) continue;

            auto* terrain = m_assetStore->GetTerrain(tc->terrainHandle);
            if (!terrain) continue;

            AssetHandle gpuMesh = GenerateTerrainMesh(terrain);
            tc->terrainHandle = gpuMesh;
        }

        for (const auto& entity : m_world->GetEntities())
        {
            auto* sm = entity->GetSkeletalMeshComponent();
            if (!sm || sm->skeletalMeshHandle == INVALID_HANDLE) continue;

            auto* skelMesh = m_assetStore->GetSkinnedMesh(sm->skeletalMeshHandle);
            if (!skelMesh) continue;

            std::vector<float> flatVerts;
            flatVerts.reserve(skelMesh->vertices.size() * 14);
            for (const auto& v : skelMesh->vertices)
            {
                flatVerts.insert(flatVerts.end(),
                    { v.position.x, v.position.y, v.position.z,
                      v.normal.x, v.normal.y, v.normal.z,
                      v.uv.x, v.uv.y,
                      v.boneWeights.x, v.boneWeights.y, v.boneWeights.z, v.boneWeights.w,
                      static_cast<float>(v.boneIndices.x), static_cast<float>(v.boneIndices.y),
                      static_cast<float>(v.boneIndices.z), static_cast<float>(v.boneIndices.w) });
            }

            AssetHandle gpuHandle = m_renderer->UploadSkinnedMesh(flatVerts, skelMesh->indices);
            sm->skeletalMeshHandle = gpuHandle;

            if (sm->materialHandle != INVALID_HANDLE)
            {
                auto* matData = m_assetStore->GetMaterial(sm->materialHandle);
                if (matData)
                {
                    AssetHandle albedoTexHandle = INVALID_HANDLE;
                    AssetHandle normalTexHandle = INVALID_HANDLE;
                    AssetHandle metallicRoughnessTexHandle = INVALID_HANDLE;
                    bool hasTex = false;
                    bool hasNormalTex = false;
                    bool hasMRTex = false;
                    if (!matData->albedoTexturePath.empty())
                    {
                        auto texHandle = m_assetStore->LoadTexture(matData->albedoTexturePath);
                        auto* texData = m_assetStore->GetTexture(texHandle);
                        if (texData)
                        {
                            albedoTexHandle = m_renderer->UploadTexture(
                                texData->width, texData->height, texData->channels, texData->pixels.data());
                            hasTex = true;
                        }
                    }
                    if (!matData->normalTexturePath.empty())
                    {
                        auto texHandle = m_assetStore->LoadTexture(matData->normalTexturePath);
                        auto* texData = m_assetStore->GetTexture(texHandle);
                        if (texData)
                        {
                            normalTexHandle = m_renderer->UploadTexture(
                                texData->width, texData->height, texData->channels, texData->pixels.data());
                            hasNormalTex = true;
                        }
                    }
                    if (!matData->metallicRoughnessTexturePath.empty())
                    {
                        auto texHandle = m_assetStore->LoadTexture(matData->metallicRoughnessTexturePath);
                        auto* texData = m_assetStore->GetTexture(texHandle);
                        if (texData)
                        {
                            metallicRoughnessTexHandle = m_renderer->UploadTexture(
                                texData->width, texData->height, texData->channels, texData->pixels.data());
                            hasMRTex = true;
                        }
                    }
                    sm->materialHandle = m_renderer->RegisterMaterial(
                        matData->albedo, matData->metallic, matData->roughness,
                        albedoTexHandle, hasTex, normalTexHandle, hasNormalTex,
                        matData->emissive, 1.0f, metallicRoughnessTexHandle, hasMRTex,
                        matData->ao);
                }
            }

            auto* anim = entity->GetAnimatorComponent();
            if (anim && anim->needsInit)
            {
                auto* skelData = m_assetStore->GetSkeleton(anim->skeletonHandle);
                auto* clipData = m_assetStore->GetAnimationClip(anim->currentClipHandle);
                if (skelData && clipData)
                {
                    anim->internalAnimator.SetSkeleton(&skelData->skeleton);
                    anim->internalAnimator.SetClip(&clipData->clip);
                    anim->needsInit = false;
                }
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
    std::unique_ptr<IRendererBackend> CreateRendererBackend(const std::string& backendName)
    {
        if (backendName == "opengl")
        {
            return CreateOpenGLBackend();
        }
        KE_LOG_WARN("Unknown backend '{}', defaulting to OpenGL", backendName);
        return CreateOpenGLBackend();
    }

    AssetHandle GenerateTerrainMesh(const TerrainData* terrain)
    {
        int w = terrain->width;
        int h = terrain->height;
        float cellSize = terrain->cellSize;

        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        for (int z = 0; z < h; ++z)
        {
            for (int x = 0; x < w; ++x)
            {
                float px = static_cast<float>(x) * cellSize;
                float pz = static_cast<float>(z) * cellSize;
                float py = terrain->GetHeight(x, z);

                float hL = terrain->GetHeight(std::max(0, x - 1), z);
                float hR = terrain->GetHeight(std::min(w - 1, x + 1), z);
                float hD = terrain->GetHeight(x, std::max(0, z - 1));
                float hU = terrain->GetHeight(x, std::min(h - 1, z + 1));
                Vec3 normal = glm::normalize(Vec3(hL - hR, 2.0f * cellSize, hD - hU));

                float u = static_cast<float>(x) / static_cast<float>(w - 1);
                float v = static_cast<float>(z) / static_cast<float>(h - 1);

                vertices.insert(vertices.end(), { px, py, pz, normal.x, normal.y, normal.z, u, v });
            }
        }

        for (int z = 0; z < h - 1; ++z)
        {
            for (int x = 0; x < w - 1; ++x)
            {
                uint32_t topLeft = static_cast<uint32_t>(z * w + x);
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = topLeft + static_cast<uint32_t>(w);
                uint32_t bottomRight = bottomLeft + 1;

                indices.insert(indices.end(), { topLeft, bottomLeft, topRight });
                indices.insert(indices.end(), { topRight, bottomLeft, bottomRight });
            }
        }

        return m_renderer->UploadMesh(vertices, indices);
    }

    void UploadAnimatorBoneMatrices()
    {
        if (!m_world || !m_renderer) return;
        for (const auto& entity : m_world->GetEntities())
        {
            auto* anim = entity->GetAnimatorComponent();
            if (!anim || anim->cachedSkinMatrices.empty()) continue;

            if (anim->boneMatricesHandle == INVALID_HANDLE)
            {
                anim->boneMatricesHandle = GenerateBoneHandle();
            }

            m_renderer->UploadBoneMatrices(anim->boneMatricesHandle, anim->cachedSkinMatrices);
        }
    }

    AssetHandle GenerateBoneHandle()
    {
        return ++m_nextBoneHandle;
    }

    AppConfig m_config;
    std::unique_ptr<GlfwWindow> m_window;
    std::unique_ptr<GlfwInput> m_input;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<AssetStore> m_assetStore;
    std::unique_ptr<World> m_world;
    Clock m_clock;
    bool m_running = false;
    AssetHandle m_nextBoneHandle = 0;
};
}
