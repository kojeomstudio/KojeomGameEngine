#pragma once

#include "App/Engine.h"
#include "Math/Transform.h"
#include "Core/Log.h"
#include "Platform/IInput.h"

namespace Kojeom
{
class SampleGame
{
public:
    void OnStart(Engine& engine)
    {
        auto* world = engine.GetWorld();
        auto* assetStore = engine.GetAssetStore();

        auto* cameraEntity = world->CreateEntity("Camera");
        auto* camComp = cameraEntity->AddComponent<CameraComponent>();
        camComp->cameraData.position = Vec3(0.0f, 3.0f, 8.0f);
        camComp->cameraData.forward = glm::normalize(Vec3(0.0f) - Vec3(0.0f, 3.0f, 8.0f));
        camComp->cameraData.fov = 60.0f;
        camComp->cameraData.aspectRatio =
            static_cast<float>(engine.GetConfig().windowWidth) /
            static_cast<float>(engine.GetConfig().windowHeight);
        camComp->cameraData.UpdateMatrices();
        camComp->isActive = true;

        auto* cubeEntity = world->CreateEntity("Cube");
        auto* transform = cubeEntity->GetTransform();
        transform->transform.position = Vec3(0.0f, 0.0f, 0.0f);
        auto* mr = cubeEntity->AddComponent<MeshRendererComponent>();

        auto* renderer = engine.GetRenderer();
        auto* glBackend = static_cast<OpenGLRenderer*>(renderer->GetBackend());
        if (glBackend)
        {
            auto* assetMesh = assetStore->GetMesh(INVALID_HANDLE);
            mr->meshHandle = INVALID_HANDLE;
        }

        auto* lightEntity = world->CreateEntity("Light");
        auto* floorEntity = world->CreateEntity("Floor");
        floorEntity->GetTransform()->transform.position = Vec3(0.0f, -1.0f, 0.0f);
        floorEntity->GetTransform()->transform.scale = Vec3(5.0f, 0.1f, 5.0f);
        floorEntity->AddComponent<MeshRendererComponent>();

        m_cameraPosition = Vec3(0.0f, 3.0f, 8.0f);
        m_cameraYaw = -90.0f;
        m_cameraPitch = -20.0f;

        KE_LOG_INFO("Sample game started with {} entities", world->GetEntityCount());
    }

    void OnUpdate(Engine& engine, float deltaSeconds)
    {
        const auto& input = engine.GetInput()->GetState();
        auto* world = engine.GetWorld();
        auto* cameraEntity = world->GetEntity(1);
        if (!cameraEntity) return;

        auto* cam = cameraEntity->GetCameraComponent();
        if (!cam) return;

        float speed = 3.0f * deltaSeconds;
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

        auto* cubeEntity = world->GetEntity(2);
        if (cubeEntity)
        {
            auto* transform = cubeEntity->GetTransform();
            m_rotationAngle += 45.0f * deltaSeconds;
            transform->transform.rotation =
                glm::angleAxis(glm::radians(m_rotationAngle), Vec3(0.0f, 1.0f, 0.0f));
        }
    }

    void OnStop()
    {
        KE_LOG_INFO("Sample game stopped");
    }

private:
    Vec3 m_cameraPosition = Vec3(0.0f, 3.0f, 8.0f);
    float m_cameraYaw = -90.0f;
    float m_cameraPitch = -20.0f;
    float m_rotationAngle = 0.0f;
};
}
