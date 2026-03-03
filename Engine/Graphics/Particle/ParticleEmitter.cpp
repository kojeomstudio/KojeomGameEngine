#include "ParticleEmitter.h"
#include <DirectXMath.h>
#include <chrono>

using namespace DirectX;

struct FParticleVertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;
    XMFLOAT2 Size;
    XMFLOAT2 TexCoord;
    float Rotation;
    UINT32 TextureIndex;
};

struct FParticleConstantBuffer
{
    XMMATRIX ViewProjection;
    XMFLOAT3 CameraRight;
    float Padding0;
    XMFLOAT3 CameraUp;
    float Padding1;
};

struct FEmitterConstantBuffer
{
    XMFLOAT4 ColorStart;
    XMFLOAT4 ColorEnd;
    float Gravity;
    float Time;
    float AtlasUVScaleX;
    float AtlasUVScaleY;
    UINT32 UseTextureAtlas;
    UINT32 AtlasColumns;
    UINT32 AtlasRows;
    float Padding;
};

HRESULT KParticleEmitter::Initialize(ID3D11Device* InDevice)
{
    if (bInitialized)
    {
        return S_OK;
    }

    if (!InDevice)
    {
        LOG_ERROR("Invalid device pointer");
        return E_INVALIDARG;
    }

    Device = InDevice;

    Particles.resize(Parameters.MaxParticles);
    EmitTimers.resize(Parameters.MaxParticles, 0.0f);

    RandomEngine.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

    CreateParticleBuffers(Device);
    CreateShaders(Device);
    CreateSamplerStates(Device);

    bInitialized = true;
    LOG_INFO("Particle emitter initialized");
    return S_OK;
}

void KParticleEmitter::Cleanup()
{
    Particles.clear();
    EmitTimers.clear();

    ParticleVertexBuffer.Reset();
    ParticleConstantBuffer.Reset();
    EmitterConstantBuffer.Reset();

    ParticleShader.reset();

    LinearSamplerState.Reset();
    AdditiveBlendState.Reset();
    ParticleDepthState.Reset();
    ParticleRasterizerState.Reset();

    bInitialized = false;
}

void KParticleEmitter::CreateParticleBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    
    bufferDesc.ByteWidth = sizeof(FParticleVertex) * Parameters.MaxParticles * 6;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &ParticleVertexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create particle vertex buffer");
    }

    bufferDesc.ByteWidth = sizeof(FParticleConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &ParticleConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create particle constant buffer");
    }

    bufferDesc.ByteWidth = sizeof(FEmitterConstantBuffer);
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &EmitterConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create emitter constant buffer");
    }
}

void KParticleEmitter::CreateShaders(ID3D11Device* InDevice)
{
    ParticleShader = std::make_shared<KShaderProgram>();

    std::string vsSource = R"(
struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR0;
    float2 Size : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float Rotation : TEXCOORD2;
    uint TextureIndex : TEXCOORD3;
};

cbuffer ParticleConstantBuffer : register(b0)
{
    matrix ViewProjection;
    float3 CameraRight;
    float Padding0;
    float3 CameraUp;
    float Padding1;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 TexCoord : TEXCOORD0;
    uint TextureIndex : TEXCOORD1;
};

float2 RotateUV(float2 uv, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float2 offset = RotateUV(input.Size * (input.TexCoord - 0.5), input.Rotation);
    
    float3 worldPos = input.Position 
        + CameraRight * offset.x 
        + CameraUp * offset.y;
    
    output.Position = mul(ViewProjection, float4(worldPos, 1.0));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.TextureIndex = input.TextureIndex;
    
    return output;
}
)";

    std::string psSource = R"(
Texture2D ParticleTexture : register(t0);

SamplerState LinearSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 TexCoord : TEXCOORD0;
    uint TextureIndex : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 texColor = ParticleTexture.Sample(LinearSampler, input.TexCoord);
    float4 result = texColor * input.Color;
    return result;
}
)";

    HRESULT hr = ParticleShader->CompileFromSource(InDevice, vsSource, psSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile particle shader");
    }
}

void KParticleEmitter::CreateSamplerStates(ID3D11Device* InDevice)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    InDevice->CreateSamplerState(&samplerDesc, &LinearSamplerState);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    InDevice->CreateBlendState(&blendDesc, &AdditiveBlendState);

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

    InDevice->CreateDepthStencilState(&depthDesc, &ParticleDepthState);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;

    InDevice->CreateRasterizerState(&rasterDesc, &ParticleRasterizerState);
}

void KParticleEmitter::Update(float DeltaTime)
{
    if (!Parameters.bEnabled)
    {
        return;
    }

    ActiveParticleCount = 0;

    for (UINT32 i = 0; i < Parameters.MaxParticles; ++i)
    {
        FParticle& particle = Particles[i];

        if (particle.Age < particle.Lifetime)
        {
            UpdateParticle(particle, DeltaTime);
            ActiveParticleCount++;
        }
        else
        {
            EmitTimers[i] -= DeltaTime;
            if (EmitTimers[i] <= 0.0f)
            {
                EmitParticle();
                EmitTimers[i] = 1.0f / static_cast<float>(Parameters.EmitRate);
            }
        }
    }
}

void KParticleEmitter::UpdateParticle(FParticle& Particle, float DeltaTime)
{
    Particle.Age += DeltaTime;

    float lifeRatio = Particle.Age / Particle.Lifetime;

    Particle.Velocity.y += Parameters.Gravity * DeltaTime;

    Particle.Position.x += Particle.Velocity.x * DeltaTime;
    Particle.Position.y += Particle.Velocity.y * DeltaTime;
    Particle.Position.z += Particle.Velocity.z * DeltaTime;

    Particle.Color.x = Parameters.ColorStart.x + (Parameters.ColorEnd.x - Parameters.ColorStart.x) * lifeRatio;
    Particle.Color.y = Parameters.ColorStart.y + (Parameters.ColorEnd.y - Parameters.ColorStart.y) * lifeRatio;
    Particle.Color.z = Parameters.ColorStart.z + (Parameters.ColorEnd.z - Parameters.ColorStart.z) * lifeRatio;
    Particle.Color.w = Parameters.ColorStart.w + (Parameters.ColorEnd.w - Parameters.ColorStart.w) * lifeRatio;

    Particle.Rotation += Particle.RotationSpeed * DeltaTime;

    if (Parameters.bUseTextureAtlas)
    {
        float totalFrames = static_cast<float>(Parameters.AtlasColumns * Parameters.AtlasRows);
        Particle.TextureIndex = static_cast<UINT32>(lifeRatio * totalFrames * Parameters.AnimationSpeed) % static_cast<UINT32>(totalFrames);
    }
}

void KParticleEmitter::EmitParticle()
{
    FParticle& particle = Particles[NextParticleIndex];

    particle.Position = GetEmissionPosition();
    if (!Parameters.bLocalSpace)
    {
        particle.Position.x += Parameters.Position.x;
        particle.Position.y += Parameters.Position.y;
        particle.Position.z += Parameters.Position.z;
    }

    XMFLOAT3 direction = GetEmissionDirection();
    float speed = Parameters.SpeedMin + RandomFloat() * (Parameters.SpeedMax - Parameters.SpeedMin);
    
    particle.Velocity.x = direction.x * speed;
    particle.Velocity.y = direction.y * speed;
    particle.Velocity.z = direction.z * speed;

    particle.Lifetime = Parameters.LifetimeMin + RandomFloat() * (Parameters.LifetimeMax - Parameters.LifetimeMin);
    particle.Age = 0.0f;

    particle.Size = Parameters.SizeMin + RandomFloat() * (Parameters.SizeMax - Parameters.SizeMin);
    
    particle.Color = Parameters.ColorStart;
    particle.Rotation = RandomFloat() * XM_2PI;
    particle.RotationSpeed = (RandomFloat() - 0.5f) * 2.0f;
    particle.TextureIndex = 0;

    NextParticleIndex = (NextParticleIndex + 1) % Parameters.MaxParticles;
}

XMFLOAT3 KParticleEmitter::GetEmissionPosition()
{
    XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };

    switch (EmitterShape)
    {
    case EEmitterShape::Point:
        break;

    case EEmitterShape::Sphere:
        {
            float theta = RandomFloat() * XM_2PI;
            float phi = RandomFloat() * XM_PI;
            float r = Parameters.bEmitFromShell ? Parameters.Radius : RandomFloat() * Parameters.Radius;
            
            position.x = r * sinf(phi) * cosf(theta);
            position.y = r * sinf(phi) * sinf(theta);
            position.z = r * cosf(phi);
        }
        break;

    case EEmitterShape::Cone:
        {
            float angle = (Parameters.SpreadAngle / 180.0f) * XM_PI;
            float theta = RandomFloat() * XM_2PI;
            float r = RandomFloat() * tanf(angle * 0.5f);
            
            position.x = r * cosf(theta);
            position.z = r * sinf(theta);
        }
        break;

    case EEmitterShape::Box:
        {
            position.x = (RandomFloat() - 0.5f) * Parameters.Radius * 2.0f;
            position.y = (RandomFloat() - 0.5f) * Parameters.Radius * 2.0f;
            position.z = (RandomFloat() - 0.5f) * Parameters.Radius * 2.0f;
        }
        break;
    }

    return position;
}

XMFLOAT3 KParticleEmitter::GetEmissionDirection()
{
    XMVECTOR baseDir = XMVector3Normalize(XMLoadFloat3(&Parameters.Direction));

    float spreadRad = (Parameters.SpreadAngle / 180.0f) * XM_PI;
    float theta = RandomFloat() * XM_2PI;
    float phi = RandomFloat() * spreadRad;

    XMVECTOR randomDir = XMVectorSet(
        sinf(phi) * cosf(theta),
        cosf(phi),
        sinf(phi) * sinf(theta),
        0.0f
    );

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetY(XMVector3Dot(baseDir, up))) > 0.99f)
    {
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, baseDir));
    up = XMVector3Normalize(XMVector3Cross(baseDir, right));

    XMMATRIX rotMatrix = XMMATRIX(
        right, up, baseDir, XMVectorSet(0, 0, 0, 1)
    );

    XMVECTOR finalDir = XMVector3TransformNormal(randomDir, rotMatrix);
    
    XMFLOAT3 result;
    XMStoreFloat3(&result, finalDir);
    return result;
}

void KParticleEmitter::Render(ID3D11DeviceContext* Context, const XMMATRIX& View, const XMMATRIX& Projection)
{
    if (!bInitialized || ActiveParticleCount == 0)
    {
        return;
    }

    UpdateParticleBuffer(Context);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(ParticleConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FParticleConstantBuffer* cb = reinterpret_cast<FParticleConstantBuffer*>(mapped.pData);
        cb->ViewProjection = XMMatrixTranspose(View * Projection);
        
        XMMATRIX viewTransposed = XMMatrixTranspose(View);
        cb->CameraRight = XMFLOAT3(XMVectorGetX(viewTransposed.r[0]), XMVectorGetY(viewTransposed.r[0]), XMVectorGetZ(viewTransposed.r[0]));
        cb->CameraUp = XMFLOAT3(XMVectorGetX(viewTransposed.r[1]), XMVectorGetY(viewTransposed.r[1]), XMVectorGetZ(viewTransposed.r[1]));
        
        Context->Unmap(ParticleConstantBuffer.Get(), 0);
    }

    Context->VSSetConstantBuffers(0, 1, ParticleConstantBuffer.GetAddressOf());

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    Context->OMSetBlendState(AdditiveBlendState.Get(), blendFactor, 0xFFFFFFFF);
    Context->OMSetDepthStencilState(ParticleDepthState.Get(), 0);
    Context->RSSetState(ParticleRasterizerState.Get());

    if (ParticleTextureSRV)
    {
        Context->PSSetShaderResources(0, 1, &ParticleTextureSRV);
    }
    Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

    ParticleShader->Bind(Context);

    UINT32 stride = sizeof(FParticleVertex);
    UINT32 offset = 0;
    Context->IASetVertexBuffers(0, 1, ParticleVertexBuffer.GetAddressOf(), &stride, &offset);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (ParticleShader->GetInputLayout())
    {
        Context->IASetInputLayout(ParticleShader->GetInputLayout());
    }

    Context->Draw(ActiveParticleCount * 6, 0);

    ParticleShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);

    Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    Context->OMSetDepthStencilState(nullptr, 0);
    Context->RSSetState(nullptr);
}

void KParticleEmitter::UpdateParticleBuffer(ID3D11DeviceContext* Context)
{
    std::vector<FParticleVertex> vertices;
    vertices.reserve(ActiveParticleCount * 6);

    for (const FParticle& particle : Particles)
    {
        if (particle.Age >= particle.Lifetime)
        {
            continue;
        }

        FParticleVertex v;
        v.Position = particle.Position;
        v.Color = particle.Color;
        v.Size = XMFLOAT2(particle.Size, particle.Size);
        v.Rotation = particle.Rotation;
        v.TextureIndex = particle.TextureIndex;

        v.TexCoord = XMFLOAT2(0.0f, 0.0f); vertices.push_back(v);
        v.TexCoord = XMFLOAT2(1.0f, 0.0f); vertices.push_back(v);
        v.TexCoord = XMFLOAT2(0.0f, 1.0f); vertices.push_back(v);
        v.TexCoord = XMFLOAT2(1.0f, 0.0f); vertices.push_back(v);
        v.TexCoord = XMFLOAT2(1.0f, 1.0f); vertices.push_back(v);
        v.TexCoord = XMFLOAT2(0.0f, 1.0f); vertices.push_back(v);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(ParticleVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, vertices.data(), sizeof(FParticleVertex) * vertices.size());
        Context->Unmap(ParticleVertexBuffer.Get(), 0);
    }
}

void KParticleEmitter::Burst(UINT32 Count)
{
    for (UINT32 i = 0; i < Count && i < Parameters.MaxParticles; ++i)
    {
        EmitParticle();
    }
}

void KParticleEmitter::Reset()
{
    for (FParticle& particle : Particles)
    {
        particle.Age = particle.Lifetime;
    }
    ActiveParticleCount = 0;
    NextParticleIndex = 0;
}
