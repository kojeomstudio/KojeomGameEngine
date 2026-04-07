#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/Debug/DebugRenderer.h"
#include <cmath>

class KDebugRenderingSample : public KEngine
{
public:
    KDebugRenderingSample() : m_SampleTotalTime(0.0f), m_CurrentDemo(0), m_bShowGrid(true), m_bShowAxis(true) {}
    ~KDebugRenderingSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        if (!renderer)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        HRESULT hr = m_DebugRenderer.Initialize(GetGraphicsDevice());
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to initialize debug renderer");
            return hr;
        }

        FDirectionalLight light;
        light.Direction = XMFLOAT3(0.5f, -1.0f, 0.5f);
        light.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        light.AmbientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
        renderer->SetDirectionalLight(light);

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -15.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Debug Rendering Sample initialized");
        LOG_INFO("Controls: SPACE - Cycle demos, G - Toggle grid, X - Toggle axis");
        LOG_INFO("WASD - Move camera, Right Mouse - Rotate camera");
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);
        m_SampleTotalTime += deltaTime;

        auto input = GetInputManager();
        if (!input) return;

        if (input->IsKeyDown(EKeyCode::Space))
        {
            m_CurrentDemo = (m_CurrentDemo + 1) % 5;
        }

        if (input->IsKeyDown(EKeyCode::G))
        {
            m_bShowGrid = !m_bShowGrid;
        }

        if (input->IsKeyDown(EKeyCode::X))
        {
            m_bShowAxis = !m_bShowAxis;
        }
    }

    void Render() override
    {
        KEngine::Render();

        auto renderer = GetRenderer();
        auto context = GetGraphicsDevice()->GetContext();
        auto camera = GetCamera();

        m_DebugRenderer.BeginFrame();

        if (m_bShowGrid)
        {
            m_DebugRenderer.DrawGrid(XMFLOAT3(0, 0, 0), 40.0f, 2.0f, XMFLOAT4(0.3f, 0.3f, 0.3f, 0.5f), 10);
        }

        if (m_bShowAxis)
        {
            m_DebugRenderer.DrawAxis(XMFLOAT3(0, 0.01f, 0), 2.0f);
        }

        switch (m_CurrentDemo)
        {
        case 0:
            RenderBasicShapes();
            break;
        case 1:
            RenderAnimatedShapes();
            break;
        case 2:
            RenderRotatingBoxes();
            break;
        case 3:
            RenderArrowsAndRays();
            break;
        case 4:
            RenderCapsulesAndCones();
            break;
        }

        m_DebugRenderer.Render(context, camera);
        m_DebugRenderer.EndFrame(0.016f);
    }

private:
    void RenderBasicShapes()
    {
        m_DebugRenderer.DrawLine(XMFLOAT3(-5, 1, 0), XMFLOAT3(5, 1, 0), XMFLOAT4(1, 0, 0, 1));
        m_DebugRenderer.DrawSphere(XMFLOAT3(0, 2, 5), 1.0f, XMFLOAT4(0, 1, 0, 1));
        m_DebugRenderer.DrawBox(XMFLOAT3(-3, 1, 5), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(0, 0, 1, 1));
        m_DebugRenderer.DrawArrow(XMFLOAT3(3, 0, 5), XMFLOAT3(3, 3, 5), XMFLOAT4(1, 1, 0, 1), 0.3f);
        m_DebugRenderer.DrawCapsule(XMFLOAT3(6, 1, 5), 0.5f, 1.0f, XMFLOAT4(1, 0, 1, 1));
    }

    void RenderAnimatedShapes()
    {
        float radius = 3.0f + sinf(m_SampleTotalTime) * 1.0f;
        m_DebugRenderer.DrawSphere(XMFLOAT3(0, 3, 0), radius, XMFLOAT4(1, 0.5f, 0, 1), 24);

        for (int i = 0; i < 8; ++i)
        {
            float angle = i * XM_PIDIV4 + m_SampleTotalTime;
            float x = cosf(angle) * 5.0f;
            float z = sinf(angle) * 5.0f;
            m_DebugRenderer.DrawBox(XMFLOAT3(x, 1, z), XMFLOAT3(0.3f, 0.3f, 0.3f), XMFLOAT4(0, 1, 0.5f, 1));
        }
    }

    void RenderRotatingBoxes()
    {
        XMMATRIX rotation = XMMatrixRotationY(m_SampleTotalTime);
        m_DebugRenderer.DrawBox(XMFLOAT3(0, 2, 0), XMFLOAT3(1, 1, 1), rotation, XMFLOAT4(1, 0.5f, 0, 1));

        XMMATRIX rot2 = XMMatrixRotationAxis(XMVectorSet(1, 0, 0, 0), m_SampleTotalTime * 0.5f);
        m_DebugRenderer.DrawBox(XMFLOAT3(4, 2, 0), XMFLOAT3(0.5f, 0.5f, 0.5f), rot2, XMFLOAT4(0, 0.5f, 1, 1));

        XMMATRIX rot3 = XMMatrixRotationAxis(XMVectorSet(0, 1, 1, 0), m_SampleTotalTime * 0.7f);
        m_DebugRenderer.DrawBox(XMFLOAT3(-4, 2, 0), XMFLOAT3(0.8f, 0.5f, 0.3f), rot3, XMFLOAT4(1, 0, 0.5f, 1));
    }

    void RenderArrowsAndRays()
    {
        for (int i = 0; i < 10; ++i)
        {
            float x = (i - 5) * 2.0f;
            float y = 1.0f + sinf(m_SampleTotalTime * 2.0f + i * 0.5f) * 0.5f;
            float z = 0;

            m_DebugRenderer.DrawArrow(
                XMFLOAT3(x, 0, z),
                XMFLOAT3(x, y, z),
                XMFLOAT4(1, 0.5f + i * 0.05f, 0, 1),
                0.2f
            );
        }

        m_DebugRenderer.DrawRay(XMFLOAT3(0, 5, -5), XMFLOAT3(0, -1, 0.3f), 8.0f, XMFLOAT4(1, 1, 0, 1));
        m_DebugRenderer.DrawRay(XMFLOAT3(-5, 5, 0), XMFLOAT3(0.5f, -1, 0), 8.0f, XMFLOAT4(0, 1, 1, 1));
    }

    void RenderCapsulesAndCones()
    {
        float baseY = 0.5f;
        m_DebugRenderer.DrawCapsule(XMFLOAT3(-6, baseY + 1, 0), 0.3f, 1.0f, XMFLOAT4(1, 0, 0, 1));
        m_DebugRenderer.DrawCapsule(XMFLOAT3(-3, baseY + 1.5f, 0), 0.5f, 1.5f, XMFLOAT4(0, 1, 0, 1));
        m_DebugRenderer.DrawCapsule(XMFLOAT3(0, baseY + 2, 0), 0.7f, 2.0f, XMFLOAT4(0, 0, 1, 1));
        m_DebugRenderer.DrawCapsule(XMFLOAT3(3, baseY + 1, 0), 0.3f, 1.0f, XMFLOAT4(1, 1, 0, 1));
        m_DebugRenderer.DrawCapsule(XMFLOAT3(6, baseY + 0.5f, 0), 0.2f, 0.5f, XMFLOAT4(1, 0, 1, 1));

        m_DebugRenderer.DrawCone(XMFLOAT3(0, 6, 8), XMFLOAT3(0, -1, -0.5f), 3.0f, 1.5f, XMFLOAT4(1, 0.5f, 0, 1), 16);
        m_DebugRenderer.DrawCone(XMFLOAT3(-5, 4, 8), XMFLOAT3(0.3f, -1, 0), 2.0f, 1.0f, XMFLOAT4(0, 0.5f, 1, 1), 12);
        m_DebugRenderer.DrawCone(XMFLOAT3(5, 4, 8), XMFLOAT3(-0.3f, -1, 0), 2.0f, 1.0f, XMFLOAT4(0.5f, 1, 0, 1), 12);

        m_DebugRenderer.DrawCylinder(XMFLOAT3(-8, 0, -5), XMFLOAT3(-8, 4, -5), 0.5f, XMFLOAT4(0.5f, 0.5f, 1, 1), 16);
        m_DebugRenderer.DrawCylinder(XMFLOAT3(0, 0, -5), XMFLOAT3(0, 5, -5), 0.3f, XMFLOAT4(1, 0.5f, 0.5f, 1), 12);
        m_DebugRenderer.DrawCylinder(XMFLOAT3(8, 0, -5), XMFLOAT3(8, 3, -5), 0.7f, XMFLOAT4(0.5f, 1, 0.5f, 1), 20);
    }

private:
    KDebugRenderer m_DebugRenderer;
    float m_SampleTotalTime;
    int32 m_CurrentDemo;
    bool m_bShowGrid;
    bool m_bShowAxis;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    return KEngine::RunApplication<KDebugRenderingSample>(
        hInstance, 
        L"Debug Rendering Sample",
        1280, 
        720,
        [](KDebugRenderingSample* App) { return App->InitializeSample(); }
    );
}
