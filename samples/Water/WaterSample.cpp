#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/Water/Water.h"
#include <cmath>

class WaterSample : public KEngine
{
public:
    WaterSample() = default;
    ~WaterSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();
        if (!renderer || !graphicsDevice)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_cubeMesh = renderer->CreateCubeMesh();
        m_sphereMesh = renderer->CreateSphereMesh(32, 16);

        m_water = std::make_unique<KWater>();
        FWaterConfig config;
        config.Resolution = 256;
        config.Width = 100.0f;
        config.Depth = 100.0f;
        config.TessellationFactor = 1.0f;
        
        if (FAILED(m_water->Initialize(graphicsDevice, config)))
        {
            LOG_ERROR("Failed to initialize water");
            return E_FAIL;
        }

        FWaterParameters waterParams = m_water->GetParameters();
        waterParams.DeepColor = XMFLOAT4(0.0f, 0.1f, 0.3f, 0.8f);
        waterParams.ShallowColor = XMFLOAT4(0.0f, 0.4f, 0.6f, 0.8f);
        waterParams.Transparency = 0.7f;
        waterParams.FresnelPower = 3.0f;
        m_water->SetParameters(waterParams);

        renderer->SetRenderPath(ERenderPath::Deferred);
        renderer->SetPostProcessEnabled(true);
        renderer->SetSkyEnabled(true);

        if (auto* sky = renderer->GetSkySystem())
        {
            sky->SetSunDirection(XMFLOAT3(0.4f, -0.6f, 0.4f));
            sky->SetSunIntensity(1.2f);
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.4f, -0.6f, 0.4f);
        dirLight.Color = XMFLOAT4(1.0f, 0.95f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.3f, 0.35f, 0.4f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 15.0f, -30.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Water Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;
        m_cameraAngle += deltaTime * 8.0f;

        float radius = 35.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 12.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        m_water->Update(deltaTime);
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        renderer->RenderSky();

        XMMATRIX floorWorld = XMMatrixScaling(60.0f, 0.5f, 60.0f) *
            XMMatrixTranslation(0.0f, -3.0f, 0.0f);
        auto sandTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, sandTexture);

        for (int i = 0; i < 5; ++i)
        {
            float angle = i * XM_2PI / 5.0f + 0.3f;
            float radius = 20.0f;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);

            XMMATRIX islandWorld = XMMatrixScaling(3.0f, 1.5f, 3.0f) *
                XMMatrixTranslation(x, 0.5f, z);
            renderer->RenderMeshLit(m_sphereMesh, islandWorld);
        }

        for (int i = 0; i < 3; ++i)
        {
            float x = -15.0f + i * 15.0f;
            float z = 10.0f * sinf(m_time * 0.5f + i);

            XMMATRIX boatWorld = XMMatrixScaling(1.5f, 0.5f, 0.8f) *
                XMMatrixRotationY(m_time * 0.2f) *
                XMMatrixTranslation(x, 0.5f + sinf(m_time * 2.0f + i) * 0.2f, z);
            renderer->RenderMeshLit(m_cubeMesh, boatWorld);
        }

        m_water->Render(renderer);
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::unique_ptr<KWater> m_water;
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

    return KEngine::RunApplication<WaterSample>(
        hInstance,
        L"Water Sample - KojeomEngine",
        1280, 720,
        [](WaterSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
