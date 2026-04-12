#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/Material.h"
#include <cmath>

class PBRSample : public KEngine
{
public:
    PBRSample() = default;
    ~PBRSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();
        if (!renderer || !graphicsDevice)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_sphereMesh = renderer->CreateSphereMesh(64, 32);
        m_cubeMesh = renderer->CreateCubeMesh();

        if (!m_sphereMesh || !m_cubeMesh)
        {
            LOG_ERROR("Failed to create meshes");
            return E_FAIL;
        }

        renderer->SetRenderPath(ERenderPath::Deferred);
        renderer->SetPostProcessEnabled(true);
        renderer->SetSSAOEnabled(true);

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.3f);
        dirLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        FPointLight pointLight;
        pointLight.Position = XMFLOAT3(5.0f, 5.0f, 5.0f);
        pointLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        pointLight.Intensity = 100.0f;
        pointLight.Radius = 30.0f;
        renderer->AddPointLight(pointLight);

        InitializeMaterials(graphicsDevice);
        InitializeVariationMaterials(graphicsDevice);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 3.0f, -10.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("PBR Sample initialized");
        return S_OK;
    }

    void InitializeMaterials(KGraphicsDevice* device)
    {
        m_goldMaterial = std::make_unique<KMaterial>();
        m_goldMaterial->Initialize(device);
        m_goldMaterial->SetParams(FPBRMaterialParams::Gold());

        m_silverMaterial = std::make_unique<KMaterial>();
        m_silverMaterial->Initialize(device);
        m_silverMaterial->SetParams(FPBRMaterialParams::Silver());

        m_copperMaterial = std::make_unique<KMaterial>();
        m_copperMaterial->Initialize(device);
        m_copperMaterial->SetParams(FPBRMaterialParams::Copper());

        m_plasticMaterial = std::make_unique<KMaterial>();
        m_plasticMaterial->Initialize(device);
        m_plasticMaterial->SetParams(FPBRMaterialParams::Plastic());
        m_plasticMaterial->SetAlbedoColor(XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f));

        m_rubberMaterial = std::make_unique<KMaterial>();
        m_rubberMaterial->Initialize(device);
        m_rubberMaterial->SetParams(FPBRMaterialParams::Rubber());
        m_rubberMaterial->SetAlbedoColor(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f));
    }

    void InitializeVariationMaterials(KGraphicsDevice* device)
    {
        for (int row = 0; row < 5; ++row)
        {
            for (int col = 0; col < 5; ++col)
            {
                auto material = std::make_unique<KMaterial>();
                material->Initialize(device);
                FPBRMaterialParams params;
                params.AlbedoColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
                params.Metallic = (float)col / 4.0f;
                params.Roughness = (float)row / 4.0f;
                params.AO = 1.0f;
                material->SetParams(params);
                m_variationMaterials.push_back(std::move(material));
            }
        }
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;
        m_cameraAngle += deltaTime * 20.0f;

        float radius = 12.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 5.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();

        float spacing = 3.0f;

        XMMATRIX goldWorld = XMMatrixTranslation(-spacing * 1.5f, 1.0f, 0.0f);
        m_goldMaterial->UpdateConstantBuffer(graphicsDevice->GetContext());
        renderer->RenderMeshPBR(m_sphereMesh, goldWorld, m_goldMaterial.get());

        XMMATRIX silverWorld = XMMatrixTranslation(-spacing * 0.5f, 1.0f, 0.0f);
        m_silverMaterial->UpdateConstantBuffer(graphicsDevice->GetContext());
        renderer->RenderMeshPBR(m_sphereMesh, silverWorld, m_silverMaterial.get());

        XMMATRIX copperWorld = XMMatrixTranslation(spacing * 0.5f, 1.0f, 0.0f);
        m_copperMaterial->UpdateConstantBuffer(graphicsDevice->GetContext());
        renderer->RenderMeshPBR(m_sphereMesh, copperWorld, m_copperMaterial.get());

        XMMATRIX plasticWorld = XMMatrixTranslation(spacing * 1.5f, 1.0f, 0.0f);
        m_plasticMaterial->UpdateConstantBuffer(graphicsDevice->GetContext());
        renderer->RenderMeshPBR(m_sphereMesh, plasticWorld, m_plasticMaterial.get());

        XMMATRIX rubberWorld = XMMatrixTranslation(0.0f, 1.0f, spacing * 1.5f);
        m_rubberMaterial->UpdateConstantBuffer(graphicsDevice->GetContext());
        renderer->RenderMeshPBR(m_sphereMesh, rubberWorld, m_rubberMaterial.get());

        int materialIndex = 0;
        for (int row = 0; row < 5; ++row)
        {
            for (int col = 0; col < 5; ++col)
            {
                KMaterial* material = m_variationMaterials[materialIndex].get();
                material->UpdateConstantBuffer(graphicsDevice->GetContext());

                float x = -4.0f + col * 2.0f;
                float z = -8.0f + row * 2.0f;
                XMMATRIX world = XMMatrixScaling(0.4f, 0.4f, 0.4f) * 
                    XMMatrixTranslation(x, 0.4f, z);
                renderer->RenderMeshPBR(m_sphereMesh, world, material);
                ++materialIndex;
            }
        }

        XMMATRIX floorWorld = XMMatrixScaling(20.0f, 0.1f, 20.0f);
        renderer->RenderMeshLit(m_cubeMesh, floorWorld);
    }

private:
    std::shared_ptr<KMesh> m_sphereMesh;
    std::shared_ptr<KMesh> m_cubeMesh;
    std::unique_ptr<KMaterial> m_goldMaterial;
    std::unique_ptr<KMaterial> m_silverMaterial;
    std::unique_ptr<KMaterial> m_copperMaterial;
    std::unique_ptr<KMaterial> m_plasticMaterial;
    std::unique_ptr<KMaterial> m_rubberMaterial;
    std::vector<std::unique_ptr<KMaterial>> m_variationMaterials;
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

    return KEngine::RunApplication<PBRSample>(
        hInstance,
        L"PBR Sample - KojeomEngine",
        1280, 720,
        [](PBRSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
