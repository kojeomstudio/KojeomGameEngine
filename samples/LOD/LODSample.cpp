#include "../../Engine/Core/Engine.h"
#include "../../Engine/Graphics/LOD/LODGenerator.h"
#include "../../Engine/Graphics/LOD/LODSystem.h"
#include <cmath>
#include <sstream>

class LODSample : public KEngine
{
public:
    LODSample() = default;
    ~LODSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        if (!renderer)
        {
            LOG_ERROR("Renderer not initialized");
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
            camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -15.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        FLODSystemParams LODParams;
        LODParams.MaxLODLevels = 4;
        LODParams.LODTransitionSpeed = 5.0f;
        LODParams.bEnableLODBlending = true;
        LODParams.LODDistances = { 10.0f, 25.0f, 50.0f, 100.0f };
        m_LODSystem.Initialize(LODParams);

        CreateLODSpheres();

        m_floorMesh = renderer->CreateCubeMesh();
        
        LOG_INFO("LOD Sample initialized - Use W/S to move camera forward/backward");
        LOG_INFO("LOD transitions at distances: 10, 25, 50, 100 units");
        return S_OK;
    }

    void CreateLODSpheres()
    {
        auto renderer = GetRenderer();
        auto baseMesh = renderer->CreateSphereMesh(64, 32);
        
        std::vector<FVertex> baseVertices;
        std::vector<uint32> baseIndices;
        
        const uint32 sphereRings = 32;
        const uint32 sphereSlices = 64;
        const float radius = 1.0f;
        
        for (uint32 ring = 0; ring <= sphereRings; ++ring)
        {
            float phi = XM_PI * static_cast<float>(ring) / static_cast<float>(sphereRings);
            for (uint32 slice = 0; slice <= sphereSlices; ++slice)
            {
                float theta = 2.0f * XM_PI * static_cast<float>(slice) / static_cast<float>(sphereSlices);
                
                FVertex v;
                v.Position = XMFLOAT3(
                    radius * sinf(phi) * cosf(theta),
                    radius * cosf(phi),
                    radius * sinf(phi) * sinf(theta)
                );
                v.Normal = v.Position;
                XMVECTOR n = XMLoadFloat3(&v.Normal);
                n = XMVector3Normalize(n);
                XMStoreFloat3(&v.Normal, n);
                v.Color = XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f);
                v.TexCoord = XMFLOAT2(
                    static_cast<float>(slice) / static_cast<float>(sphereSlices),
                    static_cast<float>(ring) / static_cast<float>(sphereRings)
                );
                
                baseVertices.push_back(v);
            }
        }
        
        for (uint32 ring = 0; ring < sphereRings; ++ring)
        {
            for (uint32 slice = 0; slice < sphereSlices; ++slice)
            {
                uint32 current = ring * (sphereSlices + 1) + slice;
                uint32 next = current + sphereSlices + 1;
                
                baseIndices.push_back(current);
                baseIndices.push_back(next);
                baseIndices.push_back(current + 1);
                
                baseIndices.push_back(current + 1);
                baseIndices.push_back(next);
                baseIndices.push_back(next + 1);
            }
        }

        FLODGenerationParams genParams;
        genParams.ReductionFactor = 0.5f;

        std::vector<FMeshLOD> lodChain = KLODGenerator::GenerateLODChain(
            baseVertices, baseIndices, 4, 0.5f);

        m_lodMeshes.resize(lodChain.size());
        for (size_t i = 0; i < lodChain.size(); ++i)
        {
            XMFLOAT4 lodColors[4] = {
                XMFLOAT4(0.2f, 0.8f, 0.2f, 1.0f),
                XMFLOAT4(0.8f, 0.8f, 0.2f, 1.0f),
                XMFLOAT4(0.8f, 0.4f, 0.2f, 1.0f),
                XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f)
            };

            for (auto& v : lodChain[i].Vertices)
            {
                v.Color = lodColors[std::min(i, static_cast<size_t>(3))];
            }

            m_lodMeshes[i] = std::make_shared<KMesh>();
            m_lodMeshes[i]->Initialize(
                GetGraphicsDevice()->GetDevice(),
                lodChain[i].Vertices.data(),
                static_cast<UINT32>(lodChain[i].Vertices.size()),
                lodChain[i].Indices.data(),
                static_cast<UINT32>(lodChain[i].Indices.size())
            );
            
            std::ostringstream oss;
            oss << "LOD" << i << ": " << lodChain[i].Vertices.size() << " vertices";
            LOG_INFO(oss.str().c_str());
        }

        for (int i = 0; i < 5; ++i)
        {
            for (int j = 0; j < 5; ++j)
            {
                FSphereObject obj;
                obj.Position = XMFLOAT3(
                    (i - 2) * 15.0f,
                    1.0f,
                    (j - 2) * 15.0f
                );
                obj.ObjectID = i * 5 + j;
                m_LODSystem.RegisterObject(obj.ObjectID);
                m_spheres.push_back(obj);
            }
        }
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_cameraDistance += m_cameraMoveSpeed * deltaTime;
        
        if (m_cameraDistance > m_maxCameraDistance)
        {
            m_cameraDistance = m_minCameraDistance;
        }

        auto camera = GetCamera();
        if (camera)
        {
            float camX = m_cameraDistance * sinf(XMConvertToRadians(m_cameraAngle));
            float camZ = m_cameraDistance * cosf(XMConvertToRadians(m_cameraAngle));
            
            camera->SetPosition(XMFLOAT3(camX, 8.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        m_cameraAngle += deltaTime * 10.0f;
        if (m_cameraAngle > 360.0f)
            m_cameraAngle -= 360.0f;

        m_LODSystem.Update(deltaTime);

        XMFLOAT3 cameraPos;
        if (camera)
        {
            cameraPos = camera->GetPosition();
        }

        for (auto& sphere : m_spheres)
        {
            uint32 newLOD = m_LODSystem.SelectLODByDistance(sphere.Position, cameraPos);
            m_LODSystem.UpdateBlendState(sphere.ObjectID, newLOD, deltaTime);
            sphere.CurrentLOD = m_LODSystem.GetBlendState(sphere.ObjectID).CurrentLOD;
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();

        XMMATRIX floorWorld = XMMatrixScaling(80.0f, 0.1f, 80.0f) *
            XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_floorMesh, floorWorld, checkerTexture);

        for (const auto& sphere : m_spheres)
        {
            uint32 lodIndex = std::min(sphere.CurrentLOD, static_cast<uint32>(m_lodMeshes.size() - 1));
            
            if (lodIndex < m_lodMeshes.size() && m_lodMeshes[lodIndex])
            {
                XMMATRIX world = XMMatrixTranslation(
                    sphere.Position.x,
                    sphere.Position.y,
                    sphere.Position.z
                );

                renderer->RenderMeshLit(m_lodMeshes[lodIndex], world);
            }
        }
    }

private:
    struct FSphereObject
    {
        XMFLOAT3 Position;
        uint32 ObjectID;
        uint32 CurrentLOD = 0;
    };

    KLODSystem m_LODSystem;
    std::vector<std::shared_ptr<KMesh>> m_lodMeshes;
    std::shared_ptr<KMesh> m_floorMesh;
    std::vector<FSphereObject> m_spheres;
    
    float m_cameraAngle = 0.0f;
    float m_cameraDistance = 15.0f;
    float m_cameraMoveSpeed = 5.0f;
    float m_minCameraDistance = 5.0f;
    float m_maxCameraDistance = 70.0f;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    return KEngine::RunApplication<LODSample>(
        hInstance, 
        L"LOD Sample - Level of Detail System",
        1280, 720,
        [](LODSample* Sample) -> HRESULT {
            return Sample->InitializeSample();
        }
    );
}
