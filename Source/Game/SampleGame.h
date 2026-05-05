#pragma once

#include "App/Engine.h"
#include "Math/Transform.h"
#include "Core/Log.h"
#include "Platform/IInput.h"
#include "Assets/AssetStore.h"

namespace Kojeom
{
class SampleGame
{
public:
    void OnStart(Engine& engine)
    {
        auto* world = engine.GetWorld();
        auto* assetStore = engine.GetAssetStore();
        auto* renderer = engine.GetRenderer();

        LightData light;
        light.direction = Vec3(0.3f, -1.0f, -0.5f);
        light.color = Vec3(1.0f, 0.95f, 0.85f);
        light.intensity = 1.5f;
        light.ambientColor = Vec3(0.12f, 0.12f, 0.15f);
        world->SetDefaultLight(light);

        float aspect = static_cast<float>(engine.GetConfig().windowWidth) /
            static_cast<float>(engine.GetConfig().windowHeight);

        auto* cameraEntity = world->CreateEntity("MainCamera");
        auto* camComp = cameraEntity->AddComponent<CameraComponent>();
        camComp->cameraData.position = Vec3(0.0f, 5.0f, 12.0f);
        camComp->cameraData.forward = glm::normalize(Vec3(0.0f, 0.0f, 0.0f) - Vec3(0.0f, 5.0f, 12.0f));
        camComp->cameraData.fov = 60.0f;
        camComp->cameraData.aspectRatio = aspect;
        camComp->cameraData.nearPlane = 0.1f;
        camComp->cameraData.farPlane = 1000.0f;
        camComp->cameraData.UpdateMatrices();
        camComp->isActive = true;

        AssetHandle cubeMatHandle = renderer->RegisterMaterial(
            Vec3(0.8f, 0.3f, 0.2f), 0.1f, 0.4f);

        auto* cubeEntity = world->CreateEntity("Cube");
        cubeEntity->GetTransform()->transform.position = Vec3(0.0f, 1.0f, 0.0f);
        auto* cubeMr = cubeEntity->AddComponent<MeshRendererComponent>();
        cubeMr->meshHandle = renderer->GetDefaultCubeHandle();
        cubeMr->materialHandle = cubeMatHandle;

        AssetHandle blueMatHandle = renderer->RegisterMaterial(
            Vec3(0.2f, 0.4f, 0.8f), 0.2f, 0.3f);

        auto* cube2Entity = world->CreateEntity("BlueCube");
        cube2Entity->GetTransform()->transform.position = Vec3(3.0f, 1.0f, -2.0f);
        auto* cube2Mr = cube2Entity->AddComponent<MeshRendererComponent>();
        cube2Mr->meshHandle = renderer->GetDefaultCubeHandle();
        cube2Mr->materialHandle = blueMatHandle;

        AssetHandle floorMatHandle = renderer->RegisterMaterial(
            Vec3(0.4f, 0.4f, 0.4f), 0.0f, 0.8f);

        auto* floorEntity = world->CreateEntity("Floor");
        floorEntity->GetTransform()->transform.position = Vec3(0.0f, 0.0f, 0.0f);
        auto* floorMr = floorEntity->AddComponent<MeshRendererComponent>();
        floorMr->meshHandle = renderer->GetDefaultPlaneHandle();
        floorMr->materialHandle = floorMatHandle;

        CreateTestTerrain(world, assetStore, renderer);

        m_cameraPosition = Vec3(0.0f, 5.0f, 12.0f);
        m_cameraYaw = -90.0f;
        m_cameraPitch = -20.0f;

        KE_LOG_INFO("Sample game started with {} entities", world->GetEntityCount());
    }

    void OnUpdate(Engine& engine, float deltaSeconds)
    {
        const auto& input = engine.GetInput()->GetState();
        auto* world = engine.GetWorld();

        Entity* cameraEntity = nullptr;
        for (size_t i = 0; i < world->GetEntityCount(); ++i)
        {
            auto& ent = world->GetEntities()[i];
            if (ent->GetCameraComponent() && ent->GetCameraComponent()->isActive)
            {
                cameraEntity = ent.get();
                break;
            }
        }
        if (!cameraEntity) return;

        auto* cam = cameraEntity->GetCameraComponent();
        if (!cam) return;

        float speed = 5.0f * deltaSeconds;
        Vec3 forward = cam->cameraData.forward;
        Vec3 right = cam->cameraData.right;
        Vec3 up = Vec3(0.0f, 1.0f, 0.0f);

        if (input.IsKeyDown(KeyCode::W))
            m_cameraPosition += forward * speed;
        if (input.IsKeyDown(KeyCode::S))
            m_cameraPosition -= forward * speed;
        if (input.IsKeyDown(KeyCode::A))
            m_cameraPosition -= right * speed;
        if (input.IsKeyDown(KeyCode::D))
            m_cameraPosition += right * speed;
        if (input.IsKeyDown(KeyCode::Space))
            m_cameraPosition += up * speed;
        if (input.IsKeyDown(KeyCode::LeftCtrl))
            m_cameraPosition -= up * speed;

        float sensitivity = 0.1f;
        if (input.IsMouseButtonDown(MouseButton::Right))
        {
            m_cameraYaw += static_cast<float>(input.mouseDeltaX) * sensitivity;
            m_cameraPitch -= static_cast<float>(input.mouseDeltaY) * sensitivity;
            m_cameraPitch = glm::clamp(m_cameraPitch, -89.0f, 89.0f);
        }

        cam->cameraData.position = m_cameraPosition;
        cam->cameraData.forward = glm::normalize(Vec3(
            cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch)),
            sin(glm::radians(m_cameraPitch)),
            sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch))
        ));
        cam->cameraData.UpdateMatrices();

        for (size_t i = 0; i < world->GetEntityCount(); ++i)
        {
            auto& ent = world->GetEntities()[i];
            if (ent->GetName() == "Cube")
            {
                auto* transform = ent->GetTransform();
                m_rotationAngle += 45.0f * deltaSeconds;
                transform->transform.rotation =
                    glm::angleAxis(glm::radians(m_rotationAngle), Vec3(0.0f, 1.0f, 0.0f));
            }
        }
    }

    void OnStop()
    {
        KE_LOG_INFO("Sample game stopped");
    }

private:
    void CreateTestTerrain(World* world, AssetStore* assetStore, Renderer* renderer)
    {
        int terrainW = 32;
        int terrainH = 32;
        float cellSize = 2.0f;
        float maxH = 3.0f;

        std::vector<float> heights(static_cast<size_t>(terrainW) * terrainH);
        for (int z = 0; z < terrainH; ++z)
        {
            for (int x = 0; x < terrainW; ++x)
            {
                float fx = static_cast<float>(x) / terrainW;
                float fz = static_cast<float>(z) / terrainH;
                heights[z * terrainW + x] =
                    (glm::sin(fx * 6.28f) * 0.5f + 0.5f) * maxH * 0.3f +
                    (glm::cos(fz * 4.0f) * 0.5f + 0.5f) * maxH * 0.2f;
            }
        }

        AssetHandle terrainHandle = assetStore->CreateTerrain("TestTerrain", terrainW, terrainH,
            cellSize, maxH, heights);

        auto* terrainEntity = world->CreateEntity("Terrain");
        terrainEntity->GetTransform()->transform.position = Vec3(
            -static_cast<float>(terrainW) * cellSize * 0.5f,
            -1.0f,
            -static_cast<float>(terrainH) * cellSize * 0.5f);
        auto* tc = terrainEntity->AddComponent<TerrainComponent>();
        tc->terrainHandle = terrainHandle;
        tc->cellSize = cellSize;

        AssetHandle terrainMatHandle = renderer->RegisterMaterial(
            Vec3(0.3f, 0.6f, 0.2f), 0.0f, 0.9f);
        tc->materialHandle = terrainMatHandle;
    }

    Vec3 m_cameraPosition = Vec3(0.0f, 5.0f, 12.0f);
    float m_cameraYaw = -90.0f;
    float m_cameraPitch = -20.0f;
    float m_rotationAngle = 0.0f;
};
}
