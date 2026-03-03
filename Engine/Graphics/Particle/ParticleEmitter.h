#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>
#include <vector>
#include <random>

struct FParticle
{
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Velocity = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float Size = 1.0f;
    float Lifetime = 1.0f;
    float Age = 0.0f;
    float Rotation = 0.0f;
    float RotationSpeed = 0.0f;
    UINT32 TextureIndex = 0;
    float Padding[3];
};

struct FParticleEmitterParams
{
    XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Direction = { 0.0f, 1.0f, 0.0f };
    
    UINT32 MaxParticles = 1000;
    UINT32 EmitRate = 10;
    float LifetimeMin = 1.0f;
    float LifetimeMax = 3.0f;
    
    float SpeedMin = 1.0f;
    float SpeedMax = 5.0f;
    float SizeMin = 0.1f;
    float SizeMax = 0.5f;
    
    XMFLOAT4 ColorStart = { 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT4 ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
    
    float Gravity = -9.81f;
    float SpreadAngle = 15.0f;
    float Radius = 0.0f;
    
    bool bEmitFromShell = false;
    bool bUseTextureAtlas = false;
    UINT32 AtlasColumns = 1;
    UINT32 AtlasRows = 1;
    float AnimationSpeed = 1.0f;
    
    bool bEnabled = true;
    bool bLocalSpace = false;
    bool bBillboard = true;
    float Padding;
};

enum class EEmitterShape
{
    Point,
    Sphere,
    Cone,
    Box
};

class KParticleEmitter
{
public:
    KParticleEmitter() = default;
    ~KParticleEmitter() = default;

    KParticleEmitter(const KParticleEmitter&) = delete;
    KParticleEmitter& operator=(const KParticleEmitter&) = delete;

    HRESULT Initialize(ID3D11Device* Device);
    void Cleanup();

    void Update(float DeltaTime);
    void Render(ID3D11DeviceContext* Context, const XMMATRIX& View, const XMMATRIX& Projection);

    void SetParameters(const FParticleEmitterParams& Params) { Parameters = Params; }
    const FParticleEmitterParams& GetParameters() const { return Parameters; }

    void SetEmitterShape(EEmitterShape Shape) { EmitterShape = Shape; }
    EEmitterShape GetEmitterShape() const { return EmitterShape; }

    void SetTexture(ID3D11ShaderResourceView* TextureSRV) { ParticleTextureSRV = TextureSRV; }
    void SetTextureAtlas(bool bUse, UINT32 Columns, UINT32 Rows)
    {
        Parameters.bUseTextureAtlas = bUse;
        Parameters.AtlasColumns = Columns;
        Parameters.AtlasRows = Rows;
    }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    bool IsEnabled() const { return Parameters.bEnabled; }

    void Burst(UINT32 Count);
    void Reset();

    UINT32 GetActiveParticleCount() const { return ActiveParticleCount; }
    UINT32 GetMaxParticles() const { return Parameters.MaxParticles; }

    bool IsInitialized() const { return bInitialized; }

private:
    void EmitParticle();
    XMFLOAT3 GetEmissionPosition();
    XMFLOAT3 GetEmissionDirection();
    void UpdateParticle(FParticle& Particle, float DeltaTime);
    void CreateParticleBuffers(ID3D11Device* Device);
    void CreateShaders(ID3D11Device* Device);
    void CreateSamplerStates(ID3D11Device* Device);
    void UpdateParticleBuffer(ID3D11DeviceContext* Context);
    void RenderBillboards(ID3D11DeviceContext* Context, const XMMATRIX& View, const XMMATRIX& Projection);

private:
    ID3D11Device* Device = nullptr;
    
    std::vector<FParticle> Particles;
    std::vector<float> EmitTimers;
    UINT32 ActiveParticleCount = 0;
    UINT32 NextParticleIndex = 0;

    FParticleEmitterParams Parameters;
    EEmitterShape EmitterShape = EEmitterShape::Point;

    ComPtr<ID3D11Buffer> ParticleVertexBuffer;
    ComPtr<ID3D11Buffer> ParticleConstantBuffer;
    ComPtr<ID3D11Buffer> EmitterConstantBuffer;

    std::shared_ptr<KShaderProgram> ParticleShader;

    ComPtr<ID3D11SamplerState> LinearSamplerState;
    ComPtr<ID3D11BlendState> AdditiveBlendState;
    ComPtr<ID3D11DepthStencilState> ParticleDepthState;
    ComPtr<ID3D11RasterizerState> ParticleRasterizerState;

    ID3D11ShaderResourceView* ParticleTextureSRV = nullptr;

    std::mt19937 RandomEngine;
    std::uniform_real_distribution<float> RandomDistribution;

    float RandomFloat() { return RandomDistribution(RandomEngine); }
    float RandomFloatConst() const { return const_cast<KParticleEmitter*>(this)->RandomDistribution(const_cast<KParticleEmitter*>(this)->RandomEngine); }

    bool bInitialized = false;

    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
