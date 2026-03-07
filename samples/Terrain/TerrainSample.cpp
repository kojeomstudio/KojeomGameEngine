#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/Terrain/Terrain.h"
#include <cmath>

class TerrainSample : public KEngine
{
public:
    TerrainSample() = default;
    ~TerrainSample() = default;

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

        m_heightMap = std::make_shared<KHeightMap>();
        m_heightMap->GeneratePerlinNoise(128, 128, 0.02f, 6, 0.5f);
        m_heightMap->Smooth(2);

        m_terrain = std::make_unique<KTerrain>();
        FTerrainConfig config;
        config.Resolution = 128;
        config.Scale = 2.0f;
        config.HeightScale = 30.0f;
        config.LODCount = 4;
        config.TextureScale = 0.1f;
        
        if (FAILED(m_terrain->InitializeWithHeightMap(graphicsDevice, config, m_heightMap)))
        {
            LOG_ERROR("Failed to initialize terrain");
            return E_FAIL;
        }

        renderer->SetRenderPath(ERenderPath::Deferred);
        renderer->SetPostProcessEnabled(true);
        renderer->SetSSAOEnabled(true);
        renderer->SetVolumetricFogEnabled(true);

        if (auto* fog = renderer->GetVolumetricFog())
        {
            FVolumetricFogParams params = fog->GetFogParams();
            params.Density = 0.015f;
            params.HeightFalloff = 0.05f;
            params.HeightBase = 5.0f;
            fog->SetFogParams(params);
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.4f, -0.8f, 0.4f);
        dirLight.Color = XMFLOAT4(1.0f, 0.95f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.2f, 0.25f, 0.3f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        renderer->SetShadowEnabled(true);
        renderer->SetShadowSceneBounds(XMFLOAT3(0.0f, 15.0f, 0.0f), 150.0f);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 50.0f, -100.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Terrain Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;
        m_cameraAngle += deltaTime * 10.0f;

        float radius = 80.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));
        
        float height = m_terrain->GetHeightAtWorldPosition(camX, camZ) + 20.0f;

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, height, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 10.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        m_terrain->Render(renderer);

        for (int i = 0; i < 5; ++i)
        {
            float angle = i * XM_2PI / 5.0f;
            float radius = 50.0f;
            float x = radius * cosf(angle + m_time * 0.1f);
            float z = radius * sinf(angle + m_time * 0.1f);
            float y = m_terrain->GetHeightAtWorldPosition(x, z) + 2.0f;

            XMMATRIX treeWorld = XMMatrixScaling(2.0f, 4.0f, 2.0f) *
                XMMatrixTranslation(x, y, z);
            renderer->RenderMeshLit(m_cubeMesh, treeWorld);
        }

        for (int i = 0; i < 8; ++i)
        {
            float angle = i * XM_2PI / 8.0f + m_time * 0.05f;
            float radius = 30.0f;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float y = m_terrain->GetHeightAtWorldPosition(x, z) + 1.0f;

            XMMATRIX rockWorld = XMMatrixScaling(0.8f, 0.8f, 0.8f) *
                XMMatrixRotationY(m_time * 0.5f) *
                XMMatrixTranslation(x, y, z);
            renderer->RenderMeshLit(m_sphereMesh, rockWorld);
        }
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::unique_ptr<KTerrain> m_terrain;
    std::shared_ptr<KHeightMap> m_heightMap;
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

    return KEngine::RunApplication<TerrainSample>(
        hInstance,
        L"Terrain Sample - KojeomEngine",
        1280, 720,
        [](TerrainSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
