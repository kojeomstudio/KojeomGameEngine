#include "Water.h"
#include "../GraphicsDevice.h"
#include "../Renderer.h"
#include "../Camera.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

HRESULT KWater::Initialize(KGraphicsDevice* Device, const FWaterConfig& InConfig)
{
    if (!Device)
    {
        LOG_ERROR("Invalid graphics device");
        return E_INVALIDARG;
    }

    if (InConfig.Resolution < 2)
    {
        LOG_ERROR("Water resolution must be at least 2");
        return E_INVALIDARG;
    }

    GraphicsDevice = Device;
    Config = InConfig;

    HRESULT hr = CreateWaterMesh();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create water mesh");
        return hr;
    }

    hr = CreateConstantBuffer();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create constant buffer");
        return hr;
    }

    InitializeDefaultWaves();

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = GraphicsDevice->GetDevice()->CreateSamplerState(&samplerDesc, &WaterSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create water sampler state");
        return hr;
    }

    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = GraphicsDevice->GetDevice()->CreateSamplerState(&samplerDesc, &ClampedSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create clamped sampler state");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Water system initialized successfully");
    return S_OK;
}

void KWater::Cleanup()
{
    WaterMesh.reset();
    NormalMap.reset();
    NormalMap2.reset();
    DuDvMap.reset();
    FoamTexture.reset();
    WaterShader.reset();

    VertexBuffer.Reset();
    IndexBuffer.Reset();
    WaterConstantBuffer.Reset();
    WaterSamplerState.Reset();
    ClampedSamplerState.Reset();

    bInitialized = false;
}

void KWater::Update(float DeltaTime)
{
    if (bWaveAnimationEnabled)
    {
        Parameters.WaveTime += DeltaTime * Parameters.WaveSpeed;
    }
}

void KWater::Render(KRenderer* Renderer, ID3D11ShaderResourceView* InReflectionTexture,
                    ID3D11ShaderResourceView* InRefractionTexture, ID3D11ShaderResourceView* InDepthTexture)
{
    if (!bInitialized || !WaterShader || !WaterMesh)
        return;

    ID3D11DeviceContext* context = GraphicsDevice->GetContext();

    XMMATRIX reflectionView = XMMatrixIdentity();
    KCamera* camera = Renderer->GetFrustum() ? nullptr : nullptr;

    UpdateConstantBuffer(nullptr, reflectionView);

    WaterShader->Bind(context);

    UINT stride = sizeof(FWaterVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetConstantBuffers(0, 1, WaterConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, WaterConstantBuffer.GetAddressOf());

    ID3D11ShaderResourceView* resources[8] = {};
    UINT resourceCount = 0;

    if (NormalMap) resources[resourceCount++] = NormalMap->GetShaderResourceView();
    if (NormalMap2) resources[resourceCount++] = NormalMap2->GetShaderResourceView();
    if (DuDvMap) resources[resourceCount++] = DuDvMap->GetShaderResourceView();
    if (FoamTexture) resources[resourceCount++] = FoamTexture->GetShaderResourceView();
    if (bReflectionEnabled && InReflectionTexture) resources[resourceCount++] = InReflectionTexture;
    if (bRefractionEnabled && InRefractionTexture) resources[resourceCount++] = InRefractionTexture;
    if (InDepthTexture) resources[resourceCount++] = InDepthTexture;

    if (resourceCount > 0)
    {
        context->PSSetShaderResources(0, resourceCount, resources);
    }

    context->PSSetSamplers(0, 1, WaterSamplerState.GetAddressOf());
    context->PSSetSamplers(1, 1, ClampedSamplerState.GetAddressOf());

    context->DrawIndexed(IndexCount, 0, 0);

    for (UINT i = 0; i < resourceCount; i++)
    {
        ID3D11ShaderResourceView* nullResource = nullptr;
        context->PSSetShaderResources(i, 1, &nullResource);
    }
}

HRESULT KWater::CreateWaterMesh()
{
    UINT32 resolution = Config.Resolution;
    float width = Config.Width;
    float depth = Config.Depth;
    float halfWidth = width * 0.5f;
    float halfDepth = depth * 0.5f;
    float cellWidth = width / (float)(resolution - 1);
    float cellDepth = depth / (float)(resolution - 1);

    VertexCount = resolution * resolution;
    IndexCount = (resolution - 1) * (resolution - 1) * 6;

    std::vector<FWaterVertex> vertices(VertexCount);
    std::vector<UINT32> indices(IndexCount);

    for (UINT32 z = 0; z < resolution; z++)
    {
        for (UINT32 x = 0; x < resolution; x++)
        {
            UINT32 index = z * resolution + x;

            float posX = (float)x * cellWidth - halfWidth;
            float posZ = (float)z * cellDepth - halfDepth;

            vertices[index].Position = XMFLOAT3(posX, 0.0f, posZ);
            vertices[index].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            vertices[index].TexCoord = XMFLOAT2((float)x / (float)(resolution - 1),
                                                (float)z / (float)(resolution - 1));
            vertices[index].Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
            vertices[index].Bitangent = XMFLOAT3(0.0f, 0.0f, 1.0f);
        }
    }

    UINT32 indexOffset = 0;
    for (UINT32 z = 0; z < resolution - 1; z++)
    {
        for (UINT32 x = 0; x < resolution - 1; x++)
        {
            UINT32 topLeft = z * resolution + x;
            UINT32 topRight = topLeft + 1;
            UINT32 bottomLeft = (z + 1) * resolution + x;
            UINT32 bottomRight = bottomLeft + 1;

            indices[indexOffset++] = topLeft;
            indices[indexOffset++] = bottomLeft;
            indices[indexOffset++] = topRight;

            indices[indexOffset++] = topRight;
            indices[indexOffset++] = bottomLeft;
            indices[indexOffset++] = bottomRight;
        }
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.ByteWidth = sizeof(FWaterVertex) * VertexCount;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    HRESULT hr = GraphicsDevice->GetDevice()->CreateBuffer(&vbDesc, &vbData, &VertexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create water vertex buffer");
        return hr;
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.ByteWidth = sizeof(UINT32) * IndexCount;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = GraphicsDevice->GetDevice()->CreateBuffer(&ibDesc, &ibData, &IndexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create water index buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KWater::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(FWaterBuffer);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = GraphicsDevice->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &WaterConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create water constant buffer");
        return hr;
    }

    return S_OK;
}

void KWater::UpdateWorldMatrix()
{
    XMMATRIX scaleMatrix = XMMatrixScaling(WorldScale.x, WorldScale.y, WorldScale.z);
    XMMATRIX translationMatrix = XMMatrixTranslation(WorldPosition.x, WorldPosition.y, WorldPosition.z);
    WorldMatrix = scaleMatrix * translationMatrix;
}

void KWater::UpdateConstantBuffer(KCamera* Camera, const XMMATRIX& ReflectionViewMatrix)
{
    ID3D11DeviceContext* context = GraphicsDevice->GetContext();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(WaterConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
        return;

    FWaterBuffer* buffer = reinterpret_cast<FWaterBuffer*>(mappedResource.pData);

    buffer->World = XMMatrixTranspose(WorldMatrix);
    
    if (Camera)
    {
        buffer->View = XMMatrixTranspose(Camera->GetViewMatrix());
        buffer->Projection = XMMatrixTranspose(Camera->GetProjectionMatrix());
        buffer->CameraPosition = XMFLOAT4(Camera->GetPosition().x, Camera->GetPosition().y,
                                          Camera->GetPosition().z, 1.0f);
    }
    else
    {
        buffer->View = XMMatrixIdentity();
        buffer->Projection = XMMatrixIdentity();
        buffer->CameraPosition = XMFLOAT4(0.0f, 10.0f, 0.0f, 1.0f);
    }
    
    buffer->ReflectionView = XMMatrixTranspose(ReflectionViewMatrix);

    buffer->DeepColor = Parameters.DeepColor;
    buffer->ShallowColor = Parameters.ShallowColor;
    buffer->FoamColor = Parameters.FoamColor;

    for (UINT32 i = 0; i < Parameters.WaveCount && i < 4; i++)
    {
        buffer->WaveDirections[i] = XMFLOAT4(Parameters.Waves[i].Direction.x,
                                              Parameters.Waves[i].Direction.y, 0.0f, 0.0f);
        buffer->WaveParams[i] = XMFLOAT4(Parameters.Waves[i].Amplitude,
                                         Parameters.Waves[i].Frequency,
                                         Parameters.Waves[i].Speed,
                                         Parameters.Waves[i].Steepness);
    }

    buffer->Time = Parameters.WaveTime;
    buffer->Transparency = Parameters.Transparency;
    buffer->RefractionScale = Parameters.RefractionScale;
    buffer->ReflectionScale = Parameters.ReflectionScale;

    buffer->FresnelBias = Parameters.FresnelBias;
    buffer->FresnelPower = Parameters.FresnelPower;
    buffer->FoamIntensity = Parameters.FoamIntensity;
    buffer->FoamThreshold = Parameters.FoamThreshold;

    buffer->NormalMapTiling = Parameters.NormalMapTiling;
    buffer->DepthMaxDistance = Parameters.DepthMaxDistance;
    buffer->WaveSpeed = Parameters.WaveSpeed;
    buffer->WaveCount = Parameters.WaveCount;

    context->Unmap(WaterConstantBuffer.Get(), 0);
}

void KWater::InitializeDefaultWaves()
{
    Parameters.Waves[0].Amplitude = 1.0f;
    Parameters.Waves[0].Frequency = 0.5f;
    Parameters.Waves[0].Speed = 1.0f;
    Parameters.Waves[0].Steepness = 1.0f;
    Parameters.Waves[0].Direction = XMFLOAT2(1.0f, 0.0f);

    Parameters.Waves[1].Amplitude = 0.5f;
    Parameters.Waves[1].Frequency = 1.0f;
    Parameters.Waves[1].Speed = 1.5f;
    Parameters.Waves[1].Steepness = 0.5f;
    Parameters.Waves[1].Direction = XMFLOAT2(0.8f, 0.6f);

    Parameters.Waves[2].Amplitude = 0.3f;
    Parameters.Waves[2].Frequency = 2.0f;
    Parameters.Waves[2].Speed = 2.0f;
    Parameters.Waves[2].Steepness = 0.3f;
    Parameters.Waves[2].Direction = XMFLOAT2(0.6f, 0.8f);

    Parameters.Waves[3].Amplitude = 0.2f;
    Parameters.Waves[3].Frequency = 3.0f;
    Parameters.Waves[3].Speed = 2.5f;
    Parameters.Waves[3].Steepness = 0.2f;
    Parameters.Waves[3].Direction = XMFLOAT2(-0.5f, 0.5f);

    Parameters.WaveCount = 4;
}

FBoundingBox KWater::GetBoundingBox() const
{
    FBoundingBox box;
    float halfWidth = Config.Width * 0.5f * WorldScale.x;
    float halfDepth = Config.Depth * 0.5f * WorldScale.z;

    box.Min = XMFLOAT3(WorldPosition.x - halfWidth, WorldPosition.y - 1.0f, WorldPosition.z - halfDepth);
    box.Max = XMFLOAT3(WorldPosition.x + halfWidth, WorldPosition.y + 1.0f, WorldPosition.z + halfDepth);

    return box;
}

HRESULT KWaterComponent::Initialize(KGraphicsDevice* Device, const FWaterConfig& InConfig)
{
    Config = InConfig;
    Water = std::make_unique<KWater>();

    HRESULT hr = Water->Initialize(Device, Config);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize water component");
        return hr;
    }

    return S_OK;
}

void KWaterComponent::Tick(float DeltaTime)
{
    if (Water)
    {
        Water->Update(DeltaTime);
    }
}

void KWaterComponent::Render(KRenderer* Renderer)
{
    if (Water)
    {
        Water->Render(Renderer, ReflectionTexture, RefractionTexture, DepthTexture);
    }
}

void KWaterComponent::Serialize(KBinaryArchive& Archive)
{
    KActorComponent::Serialize(Archive);
    Archive << Config.Resolution;
    Archive << Config.Width;
    Archive << Config.Depth;
    Archive << Config.TessellationFactor;

    FWaterParameters params = Water->GetParameters();
    Archive << params.DeepColor.x << params.DeepColor.y << params.DeepColor.z << params.DeepColor.w;
    Archive << params.ShallowColor.x << params.ShallowColor.y << params.ShallowColor.z << params.ShallowColor.w;
    Archive << params.Transparency << params.RefractionScale << params.ReflectionScale;
    Archive << params.FresnelBias << params.FresnelPower;
    Archive << params.FoamIntensity << params.FoamThreshold;
    Archive << params.WaveSpeed << params.NormalMapTiling << params.DepthMaxDistance;
}

void KWaterComponent::Deserialize(KBinaryArchive& Archive)
{
    KActorComponent::Deserialize(Archive);
    Archive >> Config.Resolution;
    Archive >> Config.Width;
    Archive >> Config.Depth;
    Archive >> Config.TessellationFactor;

    FWaterParameters params;
    Archive >> params.DeepColor.x >> params.DeepColor.y >> params.DeepColor.z >> params.DeepColor.w;
    Archive >> params.ShallowColor.x >> params.ShallowColor.y >> params.ShallowColor.z >> params.ShallowColor.w;
    Archive >> params.Transparency >> params.RefractionScale >> params.ReflectionScale;
    Archive >> params.FresnelBias >> params.FresnelPower;
    Archive >> params.FoamIntensity >> params.FoamThreshold;
    Archive >> params.WaveSpeed >> params.NormalMapTiling >> params.DepthMaxDistance;

    if (Water)
    {
        Water->SetParameters(params);
    }
}
