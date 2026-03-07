#include "../../Engine/Core/Engine.h"
#include <cmath>

class PostProcessingSample : public KEngine
{
public:
    PostProcessingSample() = default;
    ~PostProcessingSample() = default;

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

        renderer->SetRenderPath(ERenderPath::Deferred);
        renderer->SetPostProcessEnabled(true);
        renderer->SetSSAOEnabled(true);
        renderer->SetSSREnabled(true);
        renderer->SetTAAEnabled(true);
        renderer->SetMotionBlurEnabled(true);
        renderer->SetLensEffectsEnabled(true);
        renderer->SetVolumetricFogEnabled(true);

        if (auto* postProcessor = renderer->GetPostProcessor())
        {
            FPostProcessParams params = postProcessor->GetParameters();
            params.bBloomEnabled = true;
            params.BloomThreshold = 0.8f;
            params.BloomIntensity = 0.5f;
            postProcessor->SetParameters(params);
        }

        if (auto* ssao = renderer->GetSSAO())
        {
            FSSAOParams params = ssao->GetParameters();
            params.Radius = 0.5f;
            params.Bias = 0.025f;
            params.Power = 2.0f;
            ssao->SetParameters(params);
        }

        if (auto* fog = renderer->GetVolumetricFog())
        {
            FVolumetricFogParams params = fog->GetFogParams();
            params.Density = 0.02f;
            params.HeightFalloff = 0.1f;
            params.Scattering = 0.5f;
            fog->SetFogParams(params);
        }

        if (auto* lens = renderer->GetLensEffects())
        {
            FLensEffectsParams params = lens->GetParameters();
            params.ChromaticAberrationStrength = 0.003f;
            params.VignetteIntensity = 0.3f;
            params.FilmGrainIntensity = 0.05f;
            lens->SetParameters(params);
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.5f);
        dirLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        FPointLight brightLight;
        brightLight.Position = XMFLOAT3(0.0f, 3.0f, 0.0f);
        brightLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        brightLight.Intensity = 50.0f;
        brightLight.Radius = 15.0f;
        renderer->AddPointLight(brightLight);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 4.0f, -12.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("PostProcessing Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;
        m_rotationAngle += deltaTime * 30.0f;

        m_cameraAngle += deltaTime * 15.0f;
        float radius = 15.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 6.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        for (int i = 0; i < 5; ++i)
        {
            float angle = i * XM_2PI / 5.0f + m_rotationAngle * 0.01f;
            float radius = 4.0f;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float y = 1.0f + sinf(m_time * 2.0f + i) * 0.5f;

            XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) *
                XMMatrixRotationY(m_rotationAngle * (i % 2 == 0 ? 1.0f : -1.0f)) *
                XMMatrixTranslation(x, y, z);
            renderer->RenderMeshLit(m_cubeMesh, world);
        }

        XMMATRIX centerSphere = XMMatrixScaling(1.5f, 1.5f, 1.5f) *
            XMMatrixRotationY(m_time) *
            XMMatrixTranslation(0.0f, 2.0f, 0.0f);
        renderer->RenderMeshLit(m_sphereMesh, centerSphere);

        XMMATRIX floorWorld = XMMatrixScaling(25.0f, 0.1f, 25.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        for (int i = 0; i < 8; ++i)
        {
            float angle = i * XM_2PI / 8.0f;
            float x = 8.0f * cosf(angle);
            float z = 8.0f * sinf(angle);
            
            XMMATRIX pillarWorld = XMMatrixScaling(0.3f, 3.0f, 0.3f) *
                XMMatrixTranslation(x, 1.5f, z);
            renderer->RenderMeshLit(m_cubeMesh, pillarWorld);
        }
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    float m_time = 0.0f;
    float m_rotationAngle = 0.0f;
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

    return KEngine::RunApplication<PostProcessingSample>(
        hInstance,
        L"PostProcessing Sample - KojeomEngine",
        1280, 720,
        [](PostProcessingSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
