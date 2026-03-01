#include "VolumetricFog.h"
#include <DirectXMath.h>

using namespace DirectX;

struct FFogConstantBuffer
{
    XMMATRIX View;
    XMMATRIX Projection;
    XMMATRIX InverseProjection;
    XMFLOAT3 CameraPosition;
    float Density;
    float HeightFalloff;
    float HeightBase;
    float Scattering;
    float Extinction;
    float Anisotropy;
    float StepCount;
    float MaxDistance;
    XMFLOAT3 FogColor;
    int bEnabled;
    XMFLOAT3 LightPosition;
    float LightIntensity;
    XMFLOAT3 LightColor;
    float LightRadius;
    int bLightEnabled;
    XMFLOAT2 Resolution;
    float Padding;
};

struct FCombineConstantBuffer
{
    float Intensity;
    float Padding[3];
};

HRESULT KVolumetricFog::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
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
    Width = InWidth;
    Height = InHeight;

    HRESULT hr = S_OK;

    hr = CreateFogRenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create fog render targets");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create fog shaders");
        return hr;
    }

    hr = CreateConstantBuffers(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create constant buffers");
        return hr;
    }

    hr = CreateSamplerStates(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sampler states");
        return hr;
    }

    hr = CreateFullscreenQuad(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create fullscreen quad");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Volumetric Fog system initialized successfully");
    return S_OK;
}

void KVolumetricFog::Cleanup()
{
    FogTexture.Reset();
    FogRTV.Reset();
    FogSRV.Reset();

    FogOutputTexture.Reset();
    FogOutputRTV.Reset();
    FogOutputSRV.Reset();

    CombinedTexture.Reset();
    CombinedRTV.Reset();
    CombinedSRV.Reset();

    FogConstantBuffer.Reset();

    FogShader.reset();
    CombineShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();

    FullscreenQuadMesh.reset();

    bInitialized = false;
}

HRESULT KVolumetricFog::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    FogTexture.Reset();
    FogRTV.Reset();
    FogSRV.Reset();

    FogOutputTexture.Reset();
    FogOutputRTV.Reset();
    FogOutputSRV.Reset();

    CombinedTexture.Reset();
    CombinedRTV.Reset();
    CombinedSRV.Reset();

    return CreateFogRenderTargets(InDevice);
}

HRESULT KVolumetricFog::CreateFogRenderTargets(ID3D11Device* InDevice)
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Width;
    texDesc.Height = Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &FogTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(FogTexture.Get(), nullptr, &FogRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(FogTexture.Get(), nullptr, &FogSRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &FogOutputTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(FogOutputTexture.Get(), nullptr, &FogOutputRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(FogOutputTexture.Get(), nullptr, &FogOutputSRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &CombinedTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(CombinedTexture.Get(), nullptr, &CombinedRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(CombinedTexture.Get(), nullptr, &CombinedSRV);
    return hr;
}

HRESULT KVolumetricFog::CreateShaders(ID3D11Device* InDevice)
{
    FogShader = std::make_shared<KShaderProgram>();

    std::string vsSource = R"(
struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = float4(input.Position, 1.0f);
    output.TexCoord = input.TexCoord;
    return output;
}
)";

    std::string fogPsSource = R"(
cbuffer FogConstantBuffer : register(b0)
{
    matrix View;
    matrix Projection;
    matrix InverseProjection;
    float3 CameraPosition;
    float Density;
    float HeightFalloff;
    float HeightBase;
    float Scattering;
    float Extinction;
    float Anisotropy;
    float StepCount;
    float MaxDistance;
    float3 FogColor;
    int bEnabled;
    float3 LightPosition;
    float LightIntensity;
    float3 LightColor;
    float LightRadius;
    int bLightEnabled;
    float2 Resolution;
    float Padding;
};

Texture2D DepthTexture : register(t0);
Texture2D PositionTexture : register(t1);

SamplerState PointSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 GetWorldPosFromDepth(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 viewPos = mul(InverseProjection, clipPos);
    viewPos /= viewPos.w;
    float4 worldPos = mul(View, viewPos);
    return worldPos.xyz;
}

float GetHeightFogDensity(float height)
{
    return Density * exp(-HeightFalloff * (height - HeightBase));
}

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float4 main(PS_INPUT input) : SV_TARGET
{
    if (!bEnabled)
    {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    float depth = DepthTexture.Sample(PointSampler, input.TexCoord).r;
    
    float3 worldPos = PositionTexture.Sample(PointSampler, input.TexCoord).xyz;
    if (length(worldPos) < 0.001)
    {
        worldPos = GetWorldPosFromDepth(input.TexCoord, depth);
    }

    float3 rayOrigin = CameraPosition;
    float3 rayDir = normalize(worldPos - CameraPosition);
    float rayLength = min(length(worldPos - CameraPosition), MaxDistance);

    float stepSize = rayLength / StepCount;
    float3 accumulatedFog = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;

    for (float t = 0.0; t < rayLength; t += stepSize)
    {
        float3 samplePos = rayOrigin + rayDir * t;
        float sampleHeight = samplePos.y;
        
        float fogDensity = GetHeightFogDensity(sampleHeight);
        float shadow = 1.0;

        if (bLightEnabled)
        {
            float3 lightDir = normalize(LightPosition - samplePos);
            float lightDist = length(LightPosition - samplePos);
            float lightAttenuation = saturate(1.0 - lightDist / LightRadius);
            
            float cosTheta = dot(rayDir, lightDir);
            float phase = HenyeyGreenstein(cosTheta, Anisotropy);
            
            float inScattering = Scattering * fogDensity * lightAttenuation * phase * LightIntensity;
            accumulatedFog += FogColor * LightColor * inScattering * transmittance * stepSize;
        }
        else
        {
            float inScattering = Scattering * fogDensity;
            accumulatedFog += FogColor * inScattering * transmittance * stepSize;
        }

        float extinctionFactor = Extinction * fogDensity * stepSize;
        transmittance *= exp(-extinctionFactor);
    }

    return float4(accumulatedFog, 1.0 - transmittance);
}
)";

    HRESULT hr = FogShader->CompileFromSource(InDevice, vsSource, fogPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile fog shader");
        return hr;
    }

    CombineShader = std::make_shared<KShaderProgram>();

    std::string combinePsSource = R"(
Texture2D SceneTexture : register(t0);
Texture2D FogTexture : register(t1);

SamplerState LinearSampler : register(s1);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneTexture.Sample(LinearSampler, input.TexCoord).rgb;
    float4 fogData = FogTexture.Sample(LinearSampler, input.TexCoord);

    float3 result = lerp(sceneColor, fogData.rgb, fogData.a);

    return float4(result, 1.0);
}
)";

    hr = CombineShader->CompileFromSource(InDevice, vsSource, combinePsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile combine shader");
        return hr;
    }

    return S_OK;
}

HRESULT KVolumetricFog::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FFogConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &FogConstantBuffer);
    return hr;
}

HRESULT KVolumetricFog::CreateSamplerStates(ID3D11Device* InDevice)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = InDevice->CreateSamplerState(&samplerDesc, &PointSamplerState);
    if (FAILED(hr)) return hr;

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    hr = InDevice->CreateSamplerState(&samplerDesc, &LinearSamplerState);
    return hr;
}

HRESULT KVolumetricFog::CreateFullscreenQuad(ID3D11Device* InDevice)
{
    FullscreenQuadMesh = std::make_shared<KMesh>();

    FVertex vertices[] = {
        FVertex(XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,1), XMFLOAT2(0.0f, 0.0f)),
        FVertex(XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,1), XMFLOAT2(1.0f, 0.0f)),
        FVertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,1), XMFLOAT2(0.0f, 1.0f)),
        FVertex(XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,1), XMFLOAT2(1.0f, 1.0f)),
    };

    UINT32 indices[] = { 0, 2, 1, 1, 2, 3 };

    HRESULT hr = FullscreenQuadMesh->Initialize(InDevice, vertices, 4, indices, 6);
    return hr;
}

void KVolumetricFog::ComputeFog(ID3D11DeviceContext* Context,
                                ID3D11ShaderResourceView* DepthSRV,
                                ID3D11ShaderResourceView* PositionSRV,
                                const XMMATRIX& View,
                                const XMMATRIX& Projection,
                                const XMMATRIX& InverseProjection,
                                const XMFLOAT3& CameraPosition)
{
    if (!FogParams.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &FogOutputRTV, nullptr);
    Context->ClearRenderTargetView(FogOutputRTV.Get(), DefaultClearColor);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(FogConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FFogConstantBuffer* cb = reinterpret_cast<FFogConstantBuffer*>(mapped.pData);
        cb->View = XMMatrixTranspose(XMMatrixInverse(nullptr, View));
        cb->Projection = XMMatrixTranspose(Projection);
        cb->InverseProjection = XMMatrixTranspose(InverseProjection);
        cb->CameraPosition = CameraPosition;
        cb->Density = FogParams.Density;
        cb->HeightFalloff = FogParams.HeightFalloff;
        cb->HeightBase = FogParams.HeightBase;
        cb->Scattering = FogParams.Scattering;
        cb->Extinction = FogParams.Extinction;
        cb->Anisotropy = FogParams.Anisotropy;
        cb->StepCount = FogParams.StepCount;
        cb->MaxDistance = FogParams.MaxDistance;
        cb->FogColor = FogParams.FogColor;
        cb->bEnabled = FogParams.bEnabled ? 1 : 0;
        cb->LightPosition = LightParams.LightPosition;
        cb->LightIntensity = LightParams.LightIntensity;
        cb->LightColor = LightParams.LightColor;
        cb->LightRadius = LightParams.LightRadius;
        cb->bLightEnabled = LightParams.bEnabled ? 1 : 0;
        cb->Resolution = XMFLOAT2((float)Width, (float)Height);
        Context->Unmap(FogConstantBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { FogConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 1, cbs);
    Context->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { DepthSRV, PositionSRV };
    Context->PSSetShaderResources(0, 2, srvs);

    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    FogShader->Bind(Context);
    RenderFullscreenQuad(Context, FogShader->GetInputLayout());
    FogShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

void KVolumetricFog::ApplyFog(ID3D11DeviceContext* Context,
                              ID3D11ShaderResourceView* SceneColorSRV)
{
    if (!FogParams.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &CombinedRTV, nullptr);

    ID3D11ShaderResourceView* srvs[] = { SceneColorSRV, FogOutputSRV.Get() };
    Context->PSSetShaderResources(0, 2, srvs);

    Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

    CombineShader->Bind(Context);
    RenderFullscreenQuad(Context, CombineShader->GetInputLayout());
    CombineShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

void KVolumetricFog::RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout)
{
    if (!FullscreenQuadMesh) return;

    UINT32 stride = sizeof(float) * 5;
    UINT32 offset = 0;
    ID3D11Buffer* vb = FullscreenQuadMesh->GetVertexBuffer();
    ID3D11Buffer* ib = FullscreenQuadMesh->GetIndexBuffer();

    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (Layout)
    {
        Context->IASetInputLayout(Layout);
    }

    Context->DrawIndexed(6, 0, 0);
}
