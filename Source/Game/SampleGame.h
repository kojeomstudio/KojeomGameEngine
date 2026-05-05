#pragma once

#include "App/Engine.h"
#include "Math/Transform.h"
#include "Core/Log.h"
#include "Platform/IInput.h"
#include "Assets/AssetStore.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"

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
        auto* glBackend = static_cast<OpenGLRenderer*>(renderer->GetBackend());

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

        MaterialData cubeMat;
        cubeMat.albedo = Vec3(0.8f, 0.3f, 0.2f);
        cubeMat.metallic = 0.1f;
        cubeMat.roughness = 0.4f;
        AssetHandle cubeMatHandle = assetStore->RegisterMaterial(cubeMat);
        if (glBackend)
        {
            AssetHandle glMat = glBackend->RegisterGLMaterial(cubeMat.albedo, cubeMat.metallic, cubeMat.roughness);
            cubeMatHandle = glMat;
        }

        auto* cubeEntity = world->CreateEntity("Cube");
        cubeEntity->GetTransform()->transform.position = Vec3(0.0f, 1.0f, 0.0f);
        auto* cubeMr = cubeEntity->AddComponent<MeshRendererComponent>();
        if (glBackend)
        {
            cubeMr->meshHandle = glBackend->GetDefaultCubeHandle();
            cubeMr->materialHandle = cubeMatHandle;
        }

        MaterialData blueMat;
        blueMat.albedo = Vec3(0.2f, 0.4f, 0.8f);
        blueMat.metallic = 0.2f;
        blueMat.roughness = 0.3f;
        AssetHandle blueMatHandle = INVALID_HANDLE;
        if (glBackend)
        {
            blueMatHandle = glBackend->RegisterGLMaterial(blueMat.albedo, blueMat.metallic, blueMat.roughness);
        }

        auto* cube2Entity = world->CreateEntity("BlueCube");
        cube2Entity->GetTransform()->transform.position = Vec3(3.0f, 1.0f, -2.0f);
        auto* cube2Mr = cube2Entity->AddComponent<MeshRendererComponent>();
        if (glBackend)
        {
            cube2Mr->meshHandle = glBackend->GetDefaultCubeHandle();
            cube2Mr->materialHandle = blueMatHandle;
        }

        MaterialData floorMat;
        floorMat.albedo = Vec3(0.4f, 0.4f, 0.4f);
        floorMat.metallic = 0.0f;
        floorMat.roughness = 0.8f;
        AssetHandle floorMatHandle = INVALID_HANDLE;
        if (glBackend)
        {
            floorMatHandle = glBackend->RegisterGLMaterial(floorMat.albedo, floorMat.metallic, floorMat.roughness);
        }

        auto* floorEntity = world->CreateEntity("Floor");
        floorEntity->GetTransform()->transform.position = Vec3(0.0f, 0.0f, 0.0f);
        auto* floorMr = floorEntity->AddComponent<MeshRendererComponent>();
        if (glBackend)
        {
            floorMr->meshHandle = glBackend->GetDefaultPlaneHandle();
            floorMr->materialHandle = floorMatHandle;
        }

        CreateTestTerrain(world, assetStore, glBackend);

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
    void CreateTestTerrain(World* world, AssetStore* assetStore, OpenGLRenderer* glBackend)
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

        if (glBackend)
        {
            auto gpuMeshHandle = GenerateTerrainMeshGPU(glBackend, assetStore, terrainHandle, terrainW, terrainH, cellSize);

            MaterialData terrainMat;
            terrainMat.albedo = Vec3(0.3f, 0.6f, 0.2f);
            terrainMat.roughness = 0.9f;
            AssetHandle terrainMatHandle = glBackend->RegisterGLMaterial(terrainMat.albedo, terrainMat.metallic, terrainMat.roughness);

            auto* terrainEntity = world->CreateEntity("Terrain");
            terrainEntity->GetTransform()->transform.position = Vec3(
                -static_cast<float>(terrainW) * cellSize * 0.5f,
                -1.0f,
                -static_cast<float>(terrainH) * cellSize * 0.5f);
            auto* tc = terrainEntity->AddComponent<TerrainComponent>();
            tc->terrainHandle = gpuMeshHandle;
            tc->materialHandle = terrainMatHandle;
        }
    }

    AssetHandle GenerateTerrainMeshGPU(OpenGLRenderer* glBackend, AssetStore* assetStore,
        AssetHandle terrainHandle, int w, int h, float cellSize)
    {
        auto* terrain = assetStore->GetTerrain(terrainHandle);
        if (!terrain) return INVALID_HANDLE;

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

        return glBackend->UploadMesh(vertices, indices);
    }

    Vec3 m_cameraPosition = Vec3(0.0f, 5.0f, 12.0f);
    float m_cameraYaw = -90.0f;
    float m_cameraPitch = -20.0f;
    float m_rotationAngle = 0.0f;
};
}
