#include "../../Engine/Core/Engine.h"
#include "../../Engine/Input/InputManager.h"
#include "../../Engine/Physics/PhysicsWorld.h"
#include "../../Engine/Physics/RigidBody.h"
#include <cmath>
#include <random>

class PhysicsSample : public KEngine
{
public:
    PhysicsSample() = default;
    ~PhysicsSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();
        if (!renderer || !graphicsDevice)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_physicsWorld = std::make_unique<KPhysicsWorld>();
        if (FAILED(m_physicsWorld->Initialize()))
        {
            LOG_ERROR("Failed to initialize physics world");
            return E_FAIL;
        }

        m_cubeMesh = renderer->CreateCubeMesh();
        m_sphereMesh = renderer->CreateSphereMesh(16, 16);

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.3f);
        dirLight.Color = XMFLOAT4(1.0f, 1.0f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.3f, 0.3f, 0.35f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        CreateGround();
        
        for (int i = 0; i < 10; ++i)
        {
            SpawnBall();
        }

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 15.0f, -25.0f));
            camera->LookAt(XMFLOAT3(0.0f, 5.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        KInputManager* inputManager = GetInputManager();
        if (inputManager)
        {
            inputManager->RegisterAction("SpawnBall", VK_SPACE, 0);
            inputManager->RegisterAction("MoveForward", 'W', 0);
            inputManager->RegisterAction("MoveBackward", 'S', 0);
            inputManager->RegisterAction("MoveLeft", 'A', 0);
            inputManager->RegisterAction("MoveRight", 'D', 0);
            LOG_INFO("Physics Sample initialized");
        }

        return S_OK;
    }

    void SpawnBall()
    {
        if (!m_physicsWorld || m_balls.size() >= 50)
            return;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDist(-5.0f, 5.0f);
        std::uniform_real_distribution<float> radiusDist(0.3f, 0.8f);
        std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);

        float x = posDist(gen);
        float z = posDist(gen);
        float radius = radiusDist(gen);

        uint32_t bodyId = m_physicsWorld->CreateRigidBody();
        KRigidBody* body = m_physicsWorld->GetRigidBody(bodyId);
        if (body)
        {
            body->SetPosition(XMFLOAT3(x, 15.0f, z));
            body->SetSphereRadius(radius);
            body->SetMass(radius * 2.0f);
            body->SetBodyType(EPhysicsBodyType::Dynamic);
            body->SetColliderType(EColliderType::Sphere);

            FBall ball;
            ball.BodyId = bodyId;
            ball.Radius = radius;
            ball.Color = XMFLOAT4(colorDist(gen), colorDist(gen), colorDist(gen), 1.0f);
            m_balls.push_back(ball);
        }
    }

    void CreateGround()
    {
        if (!m_physicsWorld)
            return;

        uint32_t groundId = m_physicsWorld->CreateRigidBody();
        KRigidBody* ground = m_physicsWorld->GetRigidBody(groundId);
        if (ground)
        {
            ground->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
            ground->SetBoxHalfExtents(XMFLOAT3(15.0f, 0.5f, 15.0f));
            ground->SetBodyType(EPhysicsBodyType::Static);
            ground->SetColliderType(EColliderType::Box);
            m_groundBodies.push_back(groundId);
        }

        for (int i = 0; i < 4; ++i)
        {
            float angle = i * XM_PIDIV2;
            float x = cosf(angle) * 15.0f;
            float z = sinf(angle) * 15.0f;

            uint32_t wallId = m_physicsWorld->CreateRigidBody();
            KRigidBody* wall = m_physicsWorld->GetRigidBody(wallId);
            if (wall)
            {
                wall->SetPosition(XMFLOAT3(x, 2.5f, z));
                wall->SetBoxHalfExtents(XMFLOAT3(0.5f, 2.5f, 15.0f));
                wall->SetBodyType(EPhysicsBodyType::Static);
                wall->SetColliderType(EColliderType::Box);
                m_groundBodies.push_back(wallId);
            }
        }
    }

    void Update(float DeltaTime) override
    {
        KEngine::Update(DeltaTime);

        if (m_physicsWorld)
        {
            m_physicsWorld->Update(DeltaTime);
        }

        KInputManager* inputManager = GetInputManager();
        if (inputManager)
        {
            if (inputManager->IsActionPressed("SpawnBall"))
            {
                SpawnBall();
            }

            auto camera = GetCamera();
            if (camera)
            {
                XMFLOAT3 camPos = camera->GetPosition();
                float speed = 20.0f * DeltaTime;
                
                if (inputManager->IsActionDown("MoveForward"))
                    camPos.z += speed;
                if (inputManager->IsActionDown("MoveBackward"))
                    camPos.z -= speed;
                if (inputManager->IsActionDown("MoveLeft"))
                    camPos.x -= speed;
                if (inputManager->IsActionDown("MoveRight"))
                    camPos.x += speed;
                
                camera->SetPosition(camPos);
            }
        }
    }

private:
    struct FBall
    {
        uint32_t BodyId;
        XMFLOAT4 Color;
        float Radius;
    };

    std::unique_ptr<KPhysicsWorld> m_physicsWorld;
    std::vector<FBall> m_balls;
    std::vector<uint32_t> m_groundBodies;
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return KEngine::RunApplication<PhysicsSample>(
        hInstance, 
        L"KojeomGameEngine - Physics Sample",
        1280, 720,
        [](PhysicsSample* Sample) -> HRESULT {
            return Sample->InitializeSample();
        }
    );
}
