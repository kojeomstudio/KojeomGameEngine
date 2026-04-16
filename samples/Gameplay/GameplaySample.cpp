#include "../../Engine/Core/Engine.h"
#include "../../Engine/Input/InputManager.h"
#include "../../Engine/Audio/AudioManager.h"
#include "../../Engine/Graphics/Particle/ParticleEmitter.h"
#include <cmath>

class GameplaySample : public KEngine
{
public:
    GameplaySample() = default;
    ~GameplaySample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();
        if (!renderer || !graphicsDevice)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        m_playerPosition = XMFLOAT3(0.0f, 1.0f, 0.0f);
        m_playerRotation = 0.0f;

        m_cubeMesh = renderer->CreateCubeMesh();
        m_sphereMesh = renderer->CreateSphereMesh(16, 16);

        for (int i = 0; i < 5; ++i)
        {
            float x = static_cast<float>(i - 2) * 3.0f;
            float z = 10.0f;
            m_collectibles.push_back(XMFLOAT3(x, 1.0f, z));
            m_collected.push_back(false);
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.3f);
        dirLight.Color = XMFLOAT4(1.0f, 1.0f, 0.9f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.3f, 0.3f, 0.35f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        renderer->SetPostProcessEnabled(true);
        if (auto* postProcessor = renderer->GetPostProcessor())
        {
            postProcessor->SetBloomEnabled(true);
            FPostProcessParams params = postProcessor->GetParameters();
            params.BloomThreshold = 0.7f;
            params.BloomIntensity = 0.5f;
            postProcessor->SetParameters(params);
        }

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 8.0f, -12.0f));
            camera->LookAt(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        KInputManager* inputManager = KInputManager::GetInstance();
        if (inputManager)
        {
            inputManager->RegisterAction("MoveForward", 'W', 0);
            inputManager->RegisterAction("MoveBackward", 'S', 0);
            inputManager->RegisterAction("MoveLeft", 'A', 0);
            inputManager->RegisterAction("MoveRight", 'D', 0);
            inputManager->RegisterAction("Jump", VK_SPACE, 0);
            inputManager->RegisterAction("Sprint", VK_SHIFT, 0);
            LOG_INFO("Input actions registered");
        }

        KAudioManager* audioManager = KAudioManager::GetInstance();
        if (audioManager)
        {
            FAudioConfig audioConfig;
            audioConfig.MasterVolume = 0.8f;
            audioConfig.SoundVolume = 1.0f;
            audioConfig.MusicVolume = 0.5f;
            
            if (FAILED(audioManager->Initialize(audioConfig)))
            {
                LOG_WARNING("Audio system initialization failed - continuing without audio");
            }
            else
            {
                LOG_INFO("Audio system initialized");
            }
        }

        m_particleEmitter = std::make_unique<KParticleEmitter>();
        FParticleEmitterParams particleParams;
        particleParams.Position = XMFLOAT3(0.0f, 0.5f, 0.0f);
        particleParams.MaxParticles = 200;
        particleParams.EmitRate = 30;
        particleParams.LifetimeMin = 0.3f;
        particleParams.LifetimeMax = 0.8f;
        particleParams.SpeedMin = 1.0f;
        particleParams.SpeedMax = 3.0f;
        particleParams.SizeMin = 0.1f;
        particleParams.SizeMax = 0.2f;
        particleParams.ColorStart = XMFLOAT4(1.0f, 0.9f, 0.2f, 1.0f);
        particleParams.ColorEnd = XMFLOAT4(1.0f, 0.3f, 0.0f, 0.0f);
        particleParams.Gravity = 2.0f;
        m_particleEmitter->SetParameters(particleParams);
        m_particleEmitter->SetEmitterShape(EEmitterShape::Cone);
        if (FAILED(m_particleEmitter->Initialize(graphicsDevice->GetDevice())))
        {
            LOG_WARNING("Particle emitter initialization failed");
        }

        LOG_INFO("Gameplay Sample initialized");
        LOG_INFO("Controls: WASD - Move, Mouse - Look, Space - Jump, Shift - Sprint");
        LOG_INFO("Collect all yellow spheres!");
        
        return S_OK;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;

        UpdatePlayer(deltaTime);
        UpdateCollectibles(deltaTime);
        CheckCollisions();

        FParticleEmitterParams params = m_particleEmitter->GetParameters();
        params.Position = m_playerPosition;
        m_particleEmitter->SetParameters(params);
        m_particleEmitter->Update(deltaTime);

        if (m_cameraFollow)
        {
            UpdateCameraFollow();
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto camera = GetCamera();
        auto graphicsDevice = GetGraphicsDevice();

        XMMATRIX playerWorld = XMMatrixRotationY(m_playerRotation) *
            XMMatrixTranslation(m_playerPosition.x, m_playerPosition.y, m_playerPosition.z);
        renderer->RenderMeshLit(m_cubeMesh, playerWorld);

        for (size_t i = 0; i < m_collectibles.size(); ++i)
        {
            if (!m_collected[i])
            {
                float bobOffset = sinf(m_time * 3.0f + static_cast<float>(i)) * 0.2f;
                XMFLOAT3 pos = m_collectibles[i];
                pos.y += bobOffset;
                
                XMMATRIX collectibleWorld = XMMatrixScaling(0.5f, 0.5f, 0.5f) *
                    XMMatrixRotationY(m_time * 2.0f) *
                    XMMatrixTranslation(pos.x, pos.y, pos.z);
                renderer->RenderMeshLit(m_sphereMesh, collectibleWorld);
            }
        }

        XMMATRIX floorWorld = XMMatrixScaling(30.0f, 0.1f, 30.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        for (int i = -2; i <= 2; ++i)
        {
            XMMATRIX pillarWorld = XMMatrixScaling(0.5f, 3.0f, 0.5f) *
                XMMatrixTranslation(static_cast<float>(i) * 5.0f, 1.5f, 15.0f);
            renderer->RenderMeshLit(m_cubeMesh, pillarWorld);

            XMMATRIX pillar2World = XMMatrixScaling(0.5f, 3.0f, 0.5f) *
                XMMatrixTranslation(static_cast<float>(i) * 5.0f, 1.5f, -15.0f);
            renderer->RenderMeshLit(m_cubeMesh, pillar2World);
        }

        XMMATRIX view = camera->GetViewMatrix();
        XMMATRIX proj = camera->GetProjectionMatrix();
        m_particleEmitter->Render(graphicsDevice->GetContext(), view, proj);
    }

private:
    void UpdatePlayer(float deltaTime)
    {
        KInputManager* input = KInputManager::GetInstance();
        if (!input) return;

        float moveSpeed = m_moveSpeed;
        if (input->IsActionDown("Sprint"))
        {
            moveSpeed *= 2.0f;
        }

        float forward = 0.0f;
        float right = 0.0f;

        if (input->IsActionDown("MoveForward")) forward += 1.0f;
        if (input->IsActionDown("MoveBackward")) forward -= 1.0f;
        if (input->IsActionDown("MoveRight")) right += 1.0f;
        if (input->IsActionDown("MoveLeft")) right -= 1.0f;

        if (forward != 0.0f || right != 0.0f)
        {
            float length = sqrtf(forward * forward + right * right);
            forward /= length;
            right /= length;

            float sinR = sinf(m_playerRotation);
            float cosR = cosf(m_playerRotation);

            float moveX = (forward * sinR + right * cosR) * moveSpeed * deltaTime;
            float moveZ = (forward * cosR - right * sinR) * moveSpeed * deltaTime;

            m_playerPosition.x += moveX;
            m_playerPosition.z += moveZ;

            m_isMoving = true;
            m_particleEmitter->SetEnabled(true);
        }
        else
        {
            m_isMoving = false;
            m_particleEmitter->SetEnabled(false);
        }

        if (input->IsActionPressed("Jump") && !m_isJumping)
        {
            m_isJumping = true;
            m_jumpVelocity = m_jumpForce;
            
            KAudioManager* audio = KAudioManager::GetInstance();
            if (audio && audio->GetLoadedSoundCount() > 0)
            {
            }
        }

        if (m_isJumping)
        {
            m_jumpVelocity -= m_gravity * deltaTime;
            m_playerPosition.y += m_jumpVelocity * deltaTime;

            if (m_playerPosition.y <= 1.0f)
            {
                m_playerPosition.y = 1.0f;
                m_isJumping = false;
                m_jumpVelocity = 0.0f;
            }
        }

        int32_t mouseX = 0, mouseY = 0;
        input->GetMouseDelta(mouseX, mouseY);
        
        if (input->IsMouseButtonDown(EMouseButton::Right))
        {
            m_playerRotation += static_cast<float>(mouseX) * m_rotationSpeed * deltaTime;
            m_cameraPitch += static_cast<float>(mouseY) * m_rotationSpeed * deltaTime;
            m_cameraPitch = Clamp(m_cameraPitch, -XM_PIDIV4, XM_PIDIV4);
        }
    }

    void UpdateCollectibles(float deltaTime)
    {
    }

    void CheckCollisions()
    {
        for (size_t i = 0; i < m_collectibles.size(); ++i)
        {
            if (m_collected[i]) continue;

            XMFLOAT3& collectible = m_collectibles[i];
            float dx = m_playerPosition.x - collectible.x;
            float dy = m_playerPosition.y - collectible.y;
            float dz = m_playerPosition.z - collectible.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < 1.0f)
            {
                m_collected[i] = true;
                m_score++;

                KAudioManager* audio = KAudioManager::GetInstance();
                if (audio && audio->GetLoadedSoundCount() > 0)
                {
                }

                char msg[64];
                sprintf_s(msg, "Collected! Score: %d / %zu", m_score, m_collectibles.size());
                LOG_INFO(msg);

                if (m_score == static_cast<int>(m_collectibles.size()))
                {
                    LOG_INFO("Congratulations! All collectibles gathered!");
                }
            }
        }
    }

    void UpdateCameraFollow()
    {
        auto camera = GetCamera();
        if (!camera) return;

        float distance = 8.0f;
        float height = 5.0f;

        float sinR = sinf(m_playerRotation);
        float cosR = cosf(m_playerRotation);

        float camX = m_playerPosition.x - sinR * distance;
        float camZ = m_playerPosition.z - cosR * distance;
        float camY = m_playerPosition.y + height;

        camera->SetPosition(XMFLOAT3(camX, camY, camZ));
        camera->LookAt(m_playerPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
    }

    float Clamp(float value, float minVal, float maxVal)
    {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::unique_ptr<KParticleEmitter> m_particleEmitter;
    
    XMFLOAT3 m_playerPosition;
    float m_playerRotation = 0.0f;
    float m_moveSpeed = 5.0f;
    float m_rotationSpeed = 0.005f;
    bool m_isMoving = false;

    bool m_isJumping = false;
    float m_jumpVelocity = 0.0f;
    float m_jumpForce = 8.0f;
    float m_gravity = 20.0f;

    float m_cameraPitch = 0.0f;
    bool m_cameraFollow = true;

    std::vector<XMFLOAT3> m_collectibles;
    std::vector<bool> m_collected;
    int m_score = 0;

    float m_time = 0.0f;
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

    return KEngine::RunApplication<GameplaySample>(
        hInstance,
        L"Gameplay Sample - KojeomEngine",
        1280, 720,
        [](GameplaySample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
