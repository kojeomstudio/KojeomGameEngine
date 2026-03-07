#include "../../Engine/Core/Engine.h"
#include <cmath>

class LightingSample : public KEngine
{
public:
    LightingSample() = default;
    ~LightingSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        if (!renderer)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_cubeMesh = renderer->CreateCubeMesh();
        m_sphereMesh = renderer->CreateSphereMesh(32, 16);

        if (!m_cubeMesh || !m_sphereMesh)
        {
            LOG_ERROR("Failed to create meshes");
            return E_FAIL;
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.3f, -1.0f, 0.5f);
        dirLight.Color = XMFLOAT4(1.0f, 0.95f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.1f, 0.1f, 0.15f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        FPointLight pointLight1;
        pointLight1.Position = XMFLOAT3(-3.0f, 2.0f, 0.0f);
        pointLight1.Color = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
        pointLight1.Intensity = 5.0f;
        pointLight1.Radius = 10.0f;
        renderer->AddPointLight(pointLight1);

        FPointLight pointLight2;
        pointLight2.Position = XMFLOAT3(3.0f, 2.0f, 0.0f);
        pointLight2.Color = XMFLOAT4(0.2f, 0.2f, 1.0f, 1.0f);
        pointLight2.Intensity = 5.0f;
        pointLight2.Radius = 10.0f;
        renderer->AddPointLight(pointLight2);

        FPointLight pointLight3;
        pointLight3.Position = XMFLOAT3(0.0f, 2.0f, -3.0f);
        pointLight3.Color = XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f);
        pointLight3.Intensity = 5.0f;
        pointLight3.Radius = 10.0f;
        renderer->AddPointLight(pointLight3);

        FSpotLight spotLight;
        spotLight.Position = XMFLOAT3(0.0f, 5.0f, 0.0f);
        spotLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
        spotLight.Color = XMFLOAT4(1.0f, 1.0f, 0.8f, 1.0f);
        spotLight.Intensity = 10.0f;
        spotLight.Radius = 15.0f;
        spotLight.InnerCone = XMConvertToRadians(15.0f);
        spotLight.OuterCone = XMConvertToRadians(30.0f);
        renderer->AddSpotLight(spotLight);

        renderer->SetShadowEnabled(true);
        renderer->SetShadowSceneBounds(XMFLOAT3(0.0f, 0.0f, 0.0f), 20.0f);
        renderer->SetDebugDrawEnabled(true);
        renderer->DebugDrawPointLightVolumes(true);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -12.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Lighting Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;

        auto renderer = GetRenderer();
        if (renderer && renderer->GetNumPointLights() > 0)
        {
            float radius = 4.0f;
            FPointLight light = renderer->GetPointLight(0);
            light.Position.x = radius * cosf(m_time);
            light.Position.z = radius * sinf(m_time);
            renderer->SetPointLight(0, light);

            if (renderer->GetNumPointLights() > 1)
            {
                light = renderer->GetPointLight(1);
                light.Position.x = radius * cosf(m_time + XM_2PI / 3.0f);
                light.Position.z = radius * sinf(m_time + XM_2PI / 3.0f);
                renderer->SetPointLight(1, light);
            }

            if (renderer->GetNumPointLights() > 2)
            {
                light = renderer->GetPointLight(2);
                light.Position.x = radius * cosf(m_time + 2.0f * XM_2PI / 3.0f);
                light.Position.z = radius * sinf(m_time + 2.0f * XM_2PI / 3.0f);
                renderer->SetPointLight(2, light);
            }
        }

        m_cameraAngle += deltaTime * 15.0f;
        float camRadius = 15.0f;
        float camX = camRadius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = camRadius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 8.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        XMMATRIX centerCube = XMMatrixRotationY(m_time * 0.5f) *
            XMMatrixTranslation(0.0f, 0.5f, 0.0f);
        renderer->RenderMeshLit(m_cubeMesh, centerCube);

        for (int i = 0; i < 4; ++i)
        {
            float angle = i * XM_PIDIV2 + m_time * 0.3f;
            float x = 4.0f * cosf(angle);
            float z = 4.0f * sinf(angle);
            XMMATRIX pillarWorld = XMMatrixScaling(0.5f, 2.0f, 0.5f) *
                XMMatrixTranslation(x, 1.0f, z);
            renderer->RenderMeshLit(m_cubeMesh, pillarWorld);
        }

        XMMATRIX floorWorld = XMMatrixScaling(15.0f, 0.1f, 15.0f) *
            XMMatrixTranslation(0.0f, -0.05f, 0.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        renderer->DebugDrawLightVolumes();
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    float m_time = 0.0f;
    float m_cameraAngle = 0.0f;
};

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    return KEngine::RunApplication<LightingSample>(
        hInstance,
        L"Lighting Sample - KojeomEngine",
        1280, 720,
        [](LightingSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
