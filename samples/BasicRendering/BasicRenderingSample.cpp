#include "../../Engine/Core/Engine.h"
#include <cmath>

class BasicRenderingSample : public KEngine
{
public:
    BasicRenderingSample() = default;
    ~BasicRenderingSample() = default;

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
        m_triangleMesh = renderer->CreateTriangleMesh();

        if (!m_cubeMesh || !m_sphereMesh || !m_triangleMesh)
        {
            LOG_ERROR("Failed to create meshes");
            return E_FAIL;
        }

        FDirectionalLight light;
        light.Direction = XMFLOAT3(0.5f, -1.0f, 0.5f);
        light.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        light.AmbientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
        renderer->SetDirectionalLight(light);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 3.0f, -8.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Basic Rendering Sample initialized");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_rotationAngle += deltaTime * 45.0f;
        if (m_rotationAngle > 360.0f)
            m_rotationAngle -= 360.0f;

        float radius = 8.0f;
        m_cameraAngle += deltaTime * 20.0f;
        if (m_cameraAngle > 360.0f)
            m_cameraAngle -= 360.0f;

        float camX = radius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = radius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 4.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        XMMATRIX cubeWorld = XMMatrixRotationY(XMConvertToRadians(m_rotationAngle)) *
            XMMatrixRotationX(XMConvertToRadians(m_rotationAngle * 0.5f));
        renderer->RenderMeshLit(m_cubeMesh, cubeWorld);

        XMMATRIX sphereWorld = XMMatrixScaling(0.7f, 0.7f, 0.7f) *
            XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
        renderer->RenderMeshLit(m_sphereMesh, sphereWorld);

        XMMATRIX sphere2World = XMMatrixScaling(0.7f, 0.7f, 0.7f) *
            XMMatrixTranslation(3.0f, 0.0f, 0.0f);
        renderer->RenderMeshLit(m_sphereMesh, sphere2World);

        XMMATRIX floorWorld = XMMatrixScaling(10.0f, 0.1f, 10.0f) *
            XMMatrixTranslation(0.0f, -1.5f, 0.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::shared_ptr<KMesh> m_triangleMesh;
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

    return KEngine::RunApplication<BasicRenderingSample>(
        hInstance,
        L"Basic Rendering Sample - KojeomEngine",
        1280, 720,
        [](BasicRenderingSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
