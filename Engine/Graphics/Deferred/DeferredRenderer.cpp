#include "DeferredRenderer.h"

HRESULT KDeferredRenderer::Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height)
{
    if (!Device)
    {
        LOG_ERROR("DeferredRenderer: Invalid device");
        return E_INVALIDARG;
    }

    HRESULT     hr = GBuffer.Initialize(Device, Width, Height);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to initialize GBuffer");
        return hr;
    }

    D3D11_TEXTURE2D_DESC lightingTexDesc = {};
    lightingTexDesc.Width = Width;
    lightingTexDesc.Height = Height;
    lightingTexDesc.MipLevels = 1;
    lightingTexDesc.ArraySize = 1;
    lightingTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    lightingTexDesc.SampleDesc.Count = 1;
    lightingTexDesc.Usage = D3D11_USAGE_DEFAULT;
    lightingTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = Device->CreateTexture2D(&lightingTexDesc, nullptr, &LightingOutputTexture);
    if (FAILED(hr))
    {
        LOG_WARNING("DeferredRenderer: Failed to create lighting output texture");
    }
    else
    {
        Device->CreateRenderTargetView(LightingOutputTexture.Get(), nullptr, &LightingOutputRTV);
        Device->CreateShaderResourceView(LightingOutputTexture.Get(), nullptr, &LightingOutputSRV);
    }

    hr = CreateGeometryPassShader(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create geometry pass shader");
        return hr;
    }

    hr = CreateLightingPassShader(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create lighting pass shader");
        return hr;
    }

    hr = CreateConstantBuffers(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create constant buffers");
        return hr;
    }

    hr = CreateFullscreenQuad(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create fullscreen quad");
        return hr;
    }

    hr = CreateSamplerStates(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create sampler states");
        return hr;
    }

    hr = CreateForwardTransparentShader(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create forward transparent shader");
        return hr;
    }

    hr = CreateBlendStates(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create blend states");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("DeferredRenderer: Initialized successfully");
    return S_OK;
}

void KDeferredRenderer::Cleanup()
{
    GBuffer.Cleanup();
    LightingOutputTexture.Reset();
    LightingOutputRTV.Reset();
    LightingOutputSRV.Reset();
    GeometryPassShader.reset();
    LightingPassShader.reset();
    ForwardTransparentShader.reset();
    TransformConstantBuffer.Reset();
    LightConstantBuffer.Reset();
    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    TransparentBlendState.Reset();
    FullscreenQuadMesh.reset();
    RenderDataList.clear();
    TransparentRenderDataList.clear();
    PointLights.clear();
    SpotLights.clear();
    bInitialized = false;
    LOG_INFO("DeferredRenderer: Cleaned up");
}

HRESULT KDeferredRenderer::CreateGeometryPassShader(ID3D11Device* Device)
{
    GeometryPassShader = std::make_shared<KShaderProgram>();
    
    std::string VSSource = R"(
cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 WorldPos : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPos = worldPos;
    output.Position = mul(worldPos, View);
    output.Position = mul(output.Position, Projection);
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    
    return output;
}
)";

    std::string PSSource = R"(
struct PSInput
{
    float4 Position : SV_POSITION;
    float4 WorldPos : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

struct PSOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 NormalRoughness : SV_TARGET1;
    float4 PositionAO : SV_TARGET2;
};

Texture2D DiffuseTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float Padding;
};

PSOutput main(PSInput input)
{
    PSOutput output;
    
    float4 albedo = DiffuseTexture.Sample(PointSampler, input.TexCoord) * input.Color * BaseColor;
    output.AlbedoMetallic = float4(albedo.rgb, Metallic);
    
    float3 normal = normalize(input.Normal);
    output.NormalRoughness = float4(normal * 0.5f + 0.5f, Roughness);
    
    output.PositionAO = float4(input.WorldPos.xyz, AO);
    
    return output;
}
)";

    return GeometryPassShader->CompileFromSource(Device, VSSource, PSSource, "main", "main");
}

HRESULT KDeferredRenderer::CreateLightingPassShader(ID3D11Device* Device)
{
    LightingPassShader = std::make_shared<KShaderProgram>();

    std::string VSSource = R"(
struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = float4(input.Position, 1.0f);
    output.TexCoord = input.TexCoord;
    return output;
}
)";

    std::string PSSource = R"(
struct LightBuffer
{
    float3 CameraPosition;
    float Padding0;
    float3 DirLightDirection;
    float Padding1;
    float3 DirLightColor;
    float DirLightIntensity;
    uint NumPointLights;
    uint NumSpotLights;
    float Padding2[2];
    
    struct PointLightData
    {
        float3 Position;
        float Intensity;
        float3 Color;
        float Radius;
    } PointLights[8];
    
    struct SpotLightData
    {
        float3 Position;
        float Intensity;
        float3 Direction;
        float OuterConeAngle;
        float3 Color;
        float InnerConeAngle;
    } SpotLights[4];
};

Texture2D AlbedoMetallicTex : register(t0);
Texture2D NormalRoughnessTex : register(t1);
Texture2D PositionAOTex : register(t2);
Texture2D DepthTex : register(t3);

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

cbuffer LightBufferCB : register(b1)
{
    LightBuffer LightData;
};

float3 ComputeDirectionalLight(float3 normal, float3 lightDir, float3 lightColor, float intensity)
{
    float ndotl = max(dot(normal, -lightDir), 0.0f);
    return lightColor * intensity * ndotl;
}

float3 ComputePointLight(float3 worldPos, float3 normal, float3 lightPos, float3 lightColor, float intensity, float radius)
{
    float3 lightDir = lightPos - worldPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    float attenuation = 1.0f - saturate(distance / radius);
    attenuation = attenuation * attenuation;
    
    float ndotl = max(dot(normal, lightDir), 0.0f);
    return lightColor * intensity * ndotl * attenuation;
}

float3 ComputeSpotLight(float3 worldPos, float3 normal, float3 lightPos, float3 lightDir, float3 lightColor, float intensity, float innerCone, float outerCone)
{
    float3 toLight = lightPos - worldPos;
    float distance = length(toLight);
    toLight = normalize(toLight);
    
    float theta = dot(toLight, -lightDir);
    float epsilon = innerCone - outerCone;
    float spotAttenuation = saturate((theta - outerCone) / epsilon);
    
    float ndotl = max(dot(normal, toLight), 0.0f);
    return lightColor * intensity * ndotl * spotAttenuation;
}

float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float4 albedoMetallic = AlbedoMetallicTex.Sample(PointSampler, TexCoord);
    float4 normalRoughness = NormalRoughnessTex.Sample(PointSampler, TexCoord);
    float4 positionAO = PositionAOTex.Sample(PointSampler, TexCoord);
    
    float3 albedo = albedoMetallic.rgb;
    float metallic = albedoMetallic.a;
    float3 normal = normalRoughness.rgb * 2.0f - 1.0f;
    float roughness = normalRoughness.a;
    float3 worldPos = positionAO.rgb;
    float ao = positionAO.a;
    
    if (length(normal) < 0.01f)
    {
        return float4(albedo, 1.0f);
    }
    
    normal = normalize(normal);
    
    float3 ambient = float3(0.1f, 0.1f, 0.15f) * albedo * ao;
    float3 totalLight = ambient;
    
    totalLight += ComputeDirectionalLight(normal, LightData.DirLightDirection, LightData.DirLightColor, LightData.DirLightIntensity);
    
    for (uint i = 0; i < LightData.NumPointLights; ++i)
    {
        totalLight += ComputePointLight(worldPos, normal, LightData.PointLights[i].Position, 
            LightData.PointLights[i].Color, LightData.PointLights[i].Intensity, LightData.PointLights[i].Radius);
    }
    
    for (uint j = 0; j < LightData.NumSpotLights; ++j)
    {
        totalLight += ComputeSpotLight(worldPos, normal, LightData.SpotLights[j].Position,
            LightData.SpotLights[j].Direction, LightData.SpotLights[j].Color,
            LightData.SpotLights[j].Intensity, LightData.SpotLights[j].InnerConeAngle,
            LightData.SpotLights[j].OuterConeAngle);
    }
    
    float3 finalColor = totalLight * albedo;
    return float4(finalColor, 1.0f);
}
)";

    return LightingPassShader->CompileFromSource(Device, VSSource, PSSource, "main", "main");
}

HRESULT KDeferredRenderer::CreateConstantBuffers(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    BufferDesc.ByteWidth = sizeof(FTransformBuffer);
    HRESULT hr = Device->CreateBuffer(&BufferDesc, nullptr, &TransformConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create transform buffer");
        return hr;
    }

    BufferDesc.ByteWidth = sizeof(FDeferredLightBuffer);
    hr = Device->CreateBuffer(&BufferDesc, nullptr, &LightConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create light buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KDeferredRenderer::CreateFullscreenQuad(ID3D11Device* Device)
{
    FVertex vertices[] = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(0,0) },
        { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(1,0) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(0,1) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(1,1) }
    };

    UINT32 indices[] = { 0, 1, 2, 2, 1, 3 };

    FullscreenQuadMesh = std::make_shared<KMesh>();
    HRESULT hr = FullscreenQuadMesh->Initialize(Device, vertices, 4, indices, 6);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to initialize fullscreen quad mesh");
        return hr;
    }

    return S_OK;
}

HRESULT KDeferredRenderer::CreateSamplerStates(ID3D11Device* Device)
{
    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = Device->CreateSamplerState(&SamplerDesc, &PointSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create point sampler state");
        return hr;
    }

    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    hr = Device->CreateSamplerState(&SamplerDesc, &LinearSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create linear sampler state");
        return hr;
    }

    return S_OK;
}

void KDeferredRenderer::BeginGeometryPass(ID3D11DeviceContext* Context, KCamera* Camera)
{
    CurrentCamera = Camera;
    
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GBuffer.Clear(Context, clearColor);
    GBuffer.Bind(Context, nullptr);
    
    if (GeometryPassShader)
    {
        GeometryPassShader->Bind(Context);
    }
    
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void KDeferredRenderer::RenderGeometry(ID3D11DeviceContext* Context)
{
    if (!CurrentCamera || !GeometryPassShader)
        return;

    XMMATRIX View = CurrentCamera->GetViewMatrix();
    XMMATRIX Projection = CurrentCamera->GetProjectionMatrix();

    for (const auto& RenderData : RenderDataList)
    {
        if (!RenderData.Mesh)
            continue;

        UpdateTransformBuffer(Context, RenderData.WorldMatrix, View, Projection);
        Context->VSSetConstantBuffers(0, 1, TransformConstantBuffer.GetAddressOf());

        if (RenderData.DiffuseTexture)
        {
            ID3D11ShaderResourceView* SRV = RenderData.DiffuseTexture->GetShaderResourceView();
            Context->PSSetShaderResources(0, 1, &SRV);
        }
        else
        {
            ID3D11ShaderResourceView* NullSRV = nullptr;
            Context->PSSetShaderResources(0, 1, &NullSRV);
        }

        RenderData.Mesh->Render(Context);
    }
}

void KDeferredRenderer::EndGeometryPass(ID3D11DeviceContext* Context)
{
    GBuffer.Unbind(Context, nullptr);
    if (GeometryPassShader)
    {
        GeometryPassShader->Unbind(Context);
    }
}

void KDeferredRenderer::RenderLightingPass(ID3D11DeviceContext* Context, KCamera* Camera)
{
    if (!LightingPassShader || !FullscreenQuadMesh)
        return;

    if (LightingOutputRTV)
    {
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        Context->ClearRenderTargetView(LightingOutputRTV.Get(), clearColor);
        Context->OMSetRenderTargets(1, &LightingOutputRTV, nullptr);
    }

    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    LightingPassShader->Bind(Context);
    
    ID3D11ShaderResourceView* SRVs[4] = {
        GBuffer.GetAlbedoMetallicSRV(),
        GBuffer.GetNormalRoughnessSRV(),
        GBuffer.GetPositionAOSRV(),
        GBuffer.GetDepthSRV()
    };
    Context->PSSetShaderResources(0, 4, SRVs);
    
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());
    Context->PSSetSamplers(1, 1, LinearSamplerState.GetAddressOf());
    
    UpdateLightBuffer(Context, Camera);
    Context->PSSetConstantBuffers(1, 1, LightConstantBuffer.GetAddressOf());
    
    FullscreenQuadMesh->Render(Context);
    
    ID3D11ShaderResourceView* NullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Context->PSSetShaderResources(0, 4, NullSRVs);

    if (LightingOutputRTV)
    {
        ID3D11RenderTargetView* nullRTV = nullptr;
        Context->OMSetRenderTargets(1, &nullRTV, nullptr);
    }
    
    LightingPassShader->Unbind(Context);
}

void KDeferredRenderer::UpdateTransformBuffer(ID3D11DeviceContext* Context, const XMMATRIX& World, const XMMATRIX& View, const XMMATRIX& Projection)
{
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (SUCCEEDED(Context->Map(TransformConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
    {
        FTransformBuffer* Buffer = static_cast<FTransformBuffer*>(MappedResource.pData);
        Buffer->World = XMMatrixTranspose(World);
        Buffer->View = XMMatrixTranspose(View);
        Buffer->Projection = XMMatrixTranspose(Projection);
        Context->Unmap(TransformConstantBuffer.Get(), 0);
    }
}

void KDeferredRenderer::UpdateLightBuffer(ID3D11DeviceContext* Context, KCamera* Camera)
{
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (SUCCEEDED(Context->Map(LightConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
    {
        FDeferredLightBuffer* Buffer = static_cast<FDeferredLightBuffer*>(MappedResource.pData);
        
        Buffer->CameraPosition = Camera->GetPosition();
        Buffer->Padding0 = 0.0f;
        
        Buffer->DirLightDirection = DirectionalLight.Direction;
        Buffer->Padding1 = 0.0f;
        Buffer->DirLightColor = XMFLOAT3(DirectionalLight.Color.x, DirectionalLight.Color.y, DirectionalLight.Color.z);
        Buffer->DirLightIntensity = DirectionalLight.Color.w > 0.0f ? DirectionalLight.Color.w : 1.0f;
        Buffer->NumPointLights = static_cast<UINT32>(std::min(PointLights.size(), size_t(8)));
        Buffer->NumSpotLights = static_cast<UINT32>(std::min(SpotLights.size(), size_t(4)));
        Buffer->Padding2[0] = 0.0f;
        Buffer->Padding2[1] = 0.0f;
        
        for (size_t i = 0; i < Buffer->NumPointLights; ++i)
        {
            Buffer->PointLights[i] = PointLights[i];
        }
        
        for (size_t i = 0; i < Buffer->NumSpotLights; ++i)
        {
            Buffer->SpotLights[i] = SpotLights[i];
        }
        
        Context->Unmap(LightConstantBuffer.Get(), 0);
    }
}

void KDeferredRenderer::AddRenderData(const FDeferredMeshRenderData& Data)
{
    RenderDataList.push_back(Data);
}

void KDeferredRenderer::ClearRenderData()
{
    RenderDataList.clear();
}

void KDeferredRenderer::AddPointLight(const FPointLight& Light)
{
    if (PointLights.size() < 8)
    {
        PointLights.push_back(Light);
    }
}

void KDeferredRenderer::ClearPointLights()
{
    PointLights.clear();
}

void KDeferredRenderer::AddSpotLight(const FSpotLight& Light)
{
    if (SpotLights.size() < 4)
    {
        SpotLights.push_back(Light);
    }
}

void KDeferredRenderer::ClearSpotLights()
{
    SpotLights.clear();
}

void KDeferredRenderer::ClearAllLights()
{
    ClearPointLights();
    ClearSpotLights();
}

HRESULT KDeferredRenderer::Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
{
    return GBuffer.Resize(Device, NewWidth, NewHeight);
}

HRESULT KDeferredRenderer::CreateForwardTransparentShader(ID3D11Device* Device)
{
    ForwardTransparentShader = std::make_shared<KShaderProgram>();

    std::string VSSource = R"(
cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(worldPos, View);
    output.Position = mul(output.Position, Projection);
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    
    return output;
}
)";

    std::string PSSource = R"(
struct LightBuffer
{
    float3 CameraPosition;
    float Padding0;
    float3 DirLightDirection;
    float Padding1;
    float3 DirLightColor;
    float DirLightIntensity;
    uint NumPointLights;
    uint NumSpotLights;
    float Padding2[2];
    
    struct PointLightData
    {
        float3 Position;
        float Intensity;
        float3 Color;
        float Radius;
    } PointLights[8];
    
    struct SpotLightData
    {
        float3 Position;
        float Intensity;
        float3 Direction;
        float OuterConeAngle;
        float3 Color;
        float InnerConeAngle;
    } SpotLights[4];
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

Texture2D DiffuseTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer LightBufferCB : register(b1)
{
    LightBuffer LightData;
};

cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Alpha;
    float Padding[3];
};

float3 ComputeDirectionalLight(float3 normal, float3 lightDir, float3 lightColor, float intensity)
{
    float ndotl = max(dot(normal, -lightDir), 0.0f);
    return lightColor * intensity * ndotl;
}

float3 ComputePointLight(float3 worldPos, float3 normal, float3 lightPos, float3 lightColor, float intensity, float radius)
{
    float3 lightDir = lightPos - worldPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    float attenuation = 1.0f - saturate(distance / radius);
    attenuation = attenuation * attenuation;
    
    float ndotl = max(dot(normal, lightDir), 0.0f);
    return lightColor * intensity * ndotl * attenuation;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = DiffuseTexture.Sample(LinearSampler, input.TexCoord);
    float4 albedo = texColor * input.Color * BaseColor;
    
    float3 normal = normalize(input.Normal);
    
    float3 ambient = float3(0.15f, 0.15f, 0.2f) * albedo.rgb;
    float3 totalLight = ambient;
    
    totalLight += ComputeDirectionalLight(normal, LightData.DirLightDirection, LightData.DirLightColor, LightData.DirLightIntensity);
    
    for (uint i = 0; i < LightData.NumPointLights; ++i)
    {
        totalLight += ComputePointLight(input.WorldPos, normal, LightData.PointLights[i].Position, 
            LightData.PointLights[i].Color, LightData.PointLights[i].Intensity, LightData.PointLights[i].Radius);
    }
    
    float3 finalColor = totalLight * albedo.rgb;
    return float4(finalColor, albedo.a * Alpha);
}
)";

    return ForwardTransparentShader->CompileFromSource(Device, VSSource, PSSource, "main", "main");
}

HRESULT KDeferredRenderer::CreateBlendStates(ID3D11Device* Device)
{
    D3D11_BLEND_DESC BlendDesc = {};
    BlendDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Device->CreateBlendState(&BlendDesc, &TransparentBlendState);
    if (FAILED(hr))
    {
        LOG_ERROR("DeferredRenderer: Failed to create transparent blend state");
        return hr;
    }

    return S_OK;
}

void KDeferredRenderer::AddTransparentRenderData(const FForwardTransparentRenderData& Data)
{
    TransparentRenderDataList.push_back(Data);
}

void KDeferredRenderer::ClearTransparentRenderData()
{
    TransparentRenderDataList.clear();
}

void KDeferredRenderer::RenderForwardTransparentPass(ID3D11DeviceContext* Context, KCamera* Camera, ID3D11RenderTargetView* BackBufferRTV)
{
    if (!ForwardTransparentShader || TransparentRenderDataList.empty())
        return;

    ID3D11DepthStencilView* DepthDSV = GBuffer.GetDepthStencilView();
    Context->OMSetRenderTargets(1, &BackBufferRTV, DepthDSV);
    
    Context->OMSetBlendState(TransparentBlendState.Get(), nullptr, 0xFFFFFFFF);
    
    ForwardTransparentShader->Bind(Context);
    
    Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());
    
    XMMATRIX View = Camera->GetViewMatrix();
    XMMATRIX Projection = Camera->GetProjectionMatrix();

    UpdateLightBuffer(Context, Camera);
    Context->PSSetConstantBuffers(1, 1, LightConstantBuffer.GetAddressOf());

    std::sort(TransparentRenderDataList.begin(), TransparentRenderDataList.end(),
        [Camera](const FForwardTransparentRenderData& A, const FForwardTransparentRenderData& B)
        {
            XMFLOAT3 camPos = Camera->GetPosition();
            XMVECTOR camPosVec = XMLoadFloat3(&camPos);
            
            XMVECTOR posA = XMVector3TransformCoord(XMVectorSet(0, 0, 0, 1), A.WorldMatrix);
            XMVECTOR posB = XMVector3TransformCoord(XMVectorSet(0, 0, 0, 1), B.WorldMatrix);
            
            float distA = XMVectorGetX(XMVector3Length(posA - camPosVec));
            float distB = XMVectorGetX(XMVector3Length(posB - camPosVec));
            
            return distA > distB;
        });

    for (const auto& RenderData : TransparentRenderDataList)
    {
        if (!RenderData.Mesh)
            continue;

        UpdateTransformBuffer(Context, RenderData.WorldMatrix, View, Projection);
        Context->VSSetConstantBuffers(0, 1, TransformConstantBuffer.GetAddressOf());

        struct FMaterialBuffer
        {
            XMFLOAT4 BaseColor;
            float Alpha;
            float Padding[3];
        } MaterialCB;
        
        MaterialCB.BaseColor = RenderData.BaseColor;
        MaterialCB.Alpha = RenderData.Alpha;
        MaterialCB.Padding[0] = MaterialCB.Padding[1] = MaterialCB.Padding[2] = 0.0f;
        
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        if (SUCCEEDED(Context->Map(TransformConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
        {
            memcpy(MappedResource.pData, &MaterialCB, sizeof(FMaterialBuffer));
            Context->Unmap(TransformConstantBuffer.Get(), 0);
        }

        if (RenderData.DiffuseTexture)
        {
            ID3D11ShaderResourceView* SRV = RenderData.DiffuseTexture->GetShaderResourceView();
            Context->PSSetShaderResources(0, 1, &SRV);
        }
        else
        {
            ID3D11ShaderResourceView* NullSRV = nullptr;
            Context->PSSetShaderResources(0, 1, &NullSRV);
        }

        RenderData.Mesh->Render(Context);
    }

    ID3D11ShaderResourceView* NullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &NullSRV);
    
    Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    
    ForwardTransparentShader->Unbind(Context);
}
