#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/Particle/ParticleEmitter.h"
#include <cmath>

class ParticlesSample : public KEngine
{
public:
    ParticlesSample() = default;
    ~ParticlesSample() = default;

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

        m_fireEmitter = std::make_unique<KParticleEmitter>();
        FParticleEmitterParams fireParams;
        fireParams.Position = XMFLOAT3(-3.0f, 0.0f, 0.0f);
        fireParams.MaxParticles = 500;
        fireParams.EmitRate = 50;
        fireParams.LifetimeMin = 0.5f;
        fireParams.LifetimeMax = 1.5f;
        fireParams.SpeedMin = 1.0f;
        fireParams.SpeedMax = 3.0f;
        fireParams.SizeMin = 0.1f;
        fireParams.SizeMax = 0.3f;
        fireParams.ColorStart = XMFLOAT4(1.0f, 0.8f, 0.2f, 1.0f);
        fireParams.ColorEnd = XMFLOAT4(1.0f, 0.2f, 0.0f, 0.0f);
        fireParams.Gravity = 2.0f;
        fireParams.SpreadAngle = 15.0f;
        m_fireEmitter->SetParameters(fireParams);
        m_fireEmitter->SetEmitterShape(EEmitterShape::Cone);
        if (FAILED(m_fireEmitter->Initialize(graphicsDevice->GetDevice())))
        {
            LOG_ERROR("Failed to initialize fire emitter");
            return E_FAIL;
        }

        m_smokeEmitter = std::make_unique<KParticleEmitter>();
        FParticleEmitterParams smokeParams;
        smokeParams.Position = XMFLOAT3(-3.0f, 1.5f, 0.0f);
        smokeParams.MaxParticles = 300;
        smokeParams.EmitRate = 20;
        smokeParams.LifetimeMin = 2.0f;
        smokeParams.LifetimeMax = 4.0f;
        smokeParams.SpeedMin = 0.3f;
        smokeParams.SpeedMax = 0.8f;
        smokeParams.SizeMin = 0.2f;
        smokeParams.SizeMax = 1.0f;
        smokeParams.ColorStart = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.7f);
        smokeParams.ColorEnd = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
        smokeParams.Gravity = 0.5f;
        m_smokeEmitter->SetParameters(smokeParams);
        if (FAILED(m_smokeEmitter->Initialize(graphicsDevice->GetDevice())))
        {
            LOG_ERROR("Failed to initialize smoke emitter");
            return E_FAIL;
        }

        m_sparkEmitter = std::make_unique<KParticleEmitter>();
        FParticleEmitterParams sparkParams;
        sparkParams.Position = XMFLOAT3(3.0f, 1.0f, 0.0f);
        sparkParams.MaxParticles = 200;
        sparkParams.EmitRate = 30;
        sparkParams.LifetimeMin = 0.3f;
        sparkParams.LifetimeMax = 0.8f;
        sparkParams.SpeedMin = 3.0f;
        sparkParams.SpeedMax = 8.0f;
        sparkParams.SizeMin = 0.05f;
        sparkParams.SizeMax = 0.15f;
        sparkParams.ColorStart = XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f);
        sparkParams.ColorEnd = XMFLOAT4(1.0f, 0.5f, 0.0f, 0.0f);
        sparkParams.Gravity = -5.0f;
        sparkParams.Radius = 0.5f;
        m_sparkEmitter->SetParameters(sparkParams);
        m_sparkEmitter->SetEmitterShape(EEmitterShape::Sphere);
        if (FAILED(m_sparkEmitter->Initialize(graphicsDevice->GetDevice())))
        {
            LOG_ERROR("Failed to initialize spark emitter");
            return E_FAIL;
        }

        m_snowEmitter = std::make_unique<KParticleEmitter>();
        FParticleEmitterParams snowParams;
        snowParams.Position = XMFLOAT3(0.0f, 10.0f, 0.0f);
        snowParams.MaxParticles = 1000;
        snowParams.EmitRate = 100;
        snowParams.LifetimeMin = 5.0f;
        snowParams.LifetimeMax = 10.0f;
        snowParams.SpeedMin = 0.2f;
        snowParams.SpeedMax = 0.5f;
        snowParams.SizeMin = 0.05f;
        snowParams.SizeMax = 0.1f;
        snowParams.ColorStart = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.8f);
        snowParams.ColorEnd = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
        snowParams.Gravity = -0.5f;
        snowParams.Radius = 10.0f;
        m_snowEmitter->SetParameters(snowParams);
        m_snowEmitter->SetEmitterShape(EEmitterShape::Box);
        if (FAILED(m_snowEmitter->Initialize(graphicsDevice->GetDevice())))
        {
            LOG_ERROR("Failed to initialize snow emitter");
            return E_FAIL;
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.3f);
        dirLight.Color = XMFLOAT4(0.8f, 0.8f, 1.0f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.2f, 0.2f, 0.3f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        renderer->SetPostProcessEnabled(true);
        if (auto* postProcessor = renderer->GetPostProcessor())
        {
            postProcessor->SetBloomEnabled(true);
            FPostProcessParams params = postProcessor->GetParameters();
            params.BloomThreshold = 0.6f;
            params.BloomIntensity = 0.8f;
            postProcessor->SetParameters(params);
        }

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -15.0f));
            camera->LookAt(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Particles Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;

        m_fireEmitter->Update(deltaTime);
        m_smokeEmitter->Update(deltaTime);
        m_sparkEmitter->Update(deltaTime);
        m_snowEmitter->Update(deltaTime);

        m_cameraAngle += deltaTime * 10.0f;
        float radius = 15.0f;
        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 6.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto camera = GetCamera();
        auto graphicsDevice = GetGraphicsDevice();

        XMMATRIX fireBaseWorld = XMMatrixScaling(1.0f, 0.5f, 1.0f) *
            XMMatrixTranslation(-3.0f, 0.25f, 0.0f);
        renderer->RenderMeshLit(m_cubeMesh, fireBaseWorld);

        XMMATRIX sparkBaseWorld = XMMatrixScaling(0.8f, 0.4f, 0.8f) *
            XMMatrixTranslation(3.0f, 0.2f, 0.0f);
        renderer->RenderMeshLit(m_cubeMesh, sparkBaseWorld);

        XMMATRIX floorWorld = XMMatrixScaling(20.0f, 0.1f, 20.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        XMMATRIX view = camera->GetViewMatrix();
        XMMATRIX proj = camera->GetProjectionMatrix();
        
        m_fireEmitter->Render(graphicsDevice->GetContext(), view, proj);
        m_smokeEmitter->Render(graphicsDevice->GetContext(), view, proj);
        m_sparkEmitter->Render(graphicsDevice->GetContext(), view, proj);
        m_snowEmitter->Render(graphicsDevice->GetContext(), view, proj);
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::unique_ptr<KParticleEmitter> m_fireEmitter;
    std::unique_ptr<KParticleEmitter> m_smokeEmitter;
    std::unique_ptr<KParticleEmitter> m_sparkEmitter;
    std::unique_ptr<KParticleEmitter> m_snowEmitter;
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

    return KEngine::RunApplication<ParticlesSample>(
        hInstance,
        L"Particles Sample - KojeomEngine",
        1280, 720,
        [](ParticlesSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
