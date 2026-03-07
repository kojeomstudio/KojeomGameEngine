#include "../../Engine/Core/Engine.h"
#include <cmath>

class SkySample : public KEngine
{
public:
    SkySample() = default;
    ~SkySample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        if (!renderer)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_cubeMesh = renderer->CreateCubeMesh();

        renderer->SetSkyEnabled(true);
        renderer->SetRenderPath(ERenderPath::Deferred);
        renderer->SetPostProcessEnabled(true);

        if (auto* sky = renderer->GetSkySystem())
        {
            sky->SetSunDirection(XMFLOAT3(0.3f, -0.5f, 0.8f));
            sky->SetSunIntensity(1.5f);
            sky->SetExposure(1.0f);
        }

        if (auto* postProcessor = renderer->GetPostProcessor())
        {
            FPostProcessParams params;
            params.Exposure = 1.2f;
            params.Gamma = 2.2f;
            postProcessor->SetParameters(params);
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.3f, -0.5f, 0.8f);
        dirLight.Color = XMFLOAT4(1.0f, 0.95f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.15f, 0.15f, 0.2f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 2.0f, -5.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Sky Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;
        m_timeOfDay += deltaTime * 0.05f;
        if (m_timeOfDay > 24.0f)
            m_timeOfDay -= 24.0f;

        auto renderer = GetRenderer();
        if (auto* sky = renderer->GetSkySystem())
        {
            float sunAngle = (m_timeOfDay / 24.0f) * XM_2PI - XM_PIDIV2;
            float sunX = cosf(sunAngle);
            float sunY = sinf(sunAngle);
            
            float clampedY = std::max(-1.0f, std::min(1.0f, sunY));
            sky->SetSunDirection(XMFLOAT3(sunX, -clampedY, 0.3f));

            float intensity = std::max(0.1f, sinf(sunAngle + XM_PIDIV2));
            sky->SetSunIntensity(intensity * 2.0f);

            FDirectionalLight dirLight;
            dirLight.Direction = XMFLOAT3(sunX, -clampedY, 0.3f);
            dirLight.Color = XMFLOAT4(intensity, intensity * 0.95f, intensity * 0.9f, 1.0f);
            dirLight.AmbientColor = XMFLOAT4(0.1f * intensity, 0.1f * intensity, 0.15f * intensity, 1.0f);
            renderer->SetDirectionalLight(dirLight);
        }

        m_cameraAngle += deltaTime * 5.0f;
        float radius = 8.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 2.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        renderer->RenderSky();

        XMMATRIX centerCube = XMMatrixRotationY(m_time * 0.3f) *
            XMMatrixRotationX(m_time * 0.2f) *
            XMMatrixTranslation(0.0f, 1.0f, 0.0f);
        renderer->RenderMeshLit(m_cubeMesh, centerCube);

        for (int i = 0; i < 4; ++i)
        {
            float angle = i * XM_PIDIV2 + m_time * 0.2f;
            float x = 3.0f * cosf(angle);
            float z = 3.0f * sinf(angle);

            XMMATRIX pillarWorld = XMMatrixScaling(0.3f, 2.0f, 0.3f) *
                XMMatrixTranslation(x, 1.0f, z);
            renderer->RenderMeshLit(m_cubeMesh, pillarWorld);
        }

        XMMATRIX floorWorld = XMMatrixScaling(10.0f, 0.1f, 10.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    float m_time = 0.0f;
    float m_timeOfDay = 8.0f;
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

    return KEngine::RunApplication<SkySample>(
        hInstance,
        L"Sky/Atmosphere Sample - KojeomEngine",
        1280, 720,
        [](SkySample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
