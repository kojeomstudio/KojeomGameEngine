#include "SSGI.h"
#include <DirectXMath.h>

using namespace DirectX;

struct FSSGIConstantBuffer
{
    XMMATRIX Projection;
    XMMATRIX InverseProjection;
    XMMATRIX View;
    XMFLOAT2 Resolution;
    float Radius;
    float Intensity;
    float SampleCount;
    float MaxSteps;
    float Thickness;
    float Falloff;
    float TemporalBlend;
    int FrameIndex;
    float Padding;
};

struct FBlurConstantBuffer
{
    XMFLOAT2 Resolution;
    XMFLOAT2 Direction;
    float BlurStrength;
    float Padding[3];
};

HRESULT KSSGI::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
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

    hr = CreateSSGIRenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSGI render targets");
        return hr;
    }

    hr = CreateNoiseTexture(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create noise texture");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSGI shaders");
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
    LOG_INFO("SSGI system initialized successfully");
    return S_OK;
}

void KSSGI::Cleanup()
{
    SSGITexture.Reset();
    SSGIRTV.Reset();
    SSGISRV.Reset();

    HistoryTexture.Reset();
    HistoryRTV.Reset();
    HistorySRV.Reset();

    BlurredTexture.Reset();
    BlurredRTV.Reset();
    BlurredSRV.Reset();

    BlurTempTexture.Reset();
    BlurTempRTV.Reset();
    BlurTempSRV.Reset();

    NoiseTexture.Reset();
    NoiseSRV.Reset();

    SSGIConstantBuffer.Reset();
    BlurConstantBuffer.Reset();

    SSGIShader.reset();
    BlurShader.reset();
    TemporalShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    ClampSamplerState.Reset();

    FullscreenQuadMesh.reset();

    bInitialized = false;
}

HRESULT KSSGI::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    SSGITexture.Reset();
    SSGIRTV.Reset();
    SSGISRV.Reset();

    HistoryTexture.Reset();
    HistoryRTV.Reset();
    HistorySRV.Reset();

    BlurredTexture.Reset();
    BlurredRTV.Reset();
    BlurredSRV.Reset();

    BlurTempTexture.Reset();
    BlurTempRTV.Reset();
    BlurTempSRV.Reset();

    return CreateSSGIRenderTargets(InDevice);
}

HRESULT KSSGI::CreateSSGIRenderTargets(ID3D11Device* InDevice)
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

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &SSGITexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(SSGITexture.Get(), nullptr, &SSGIRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(SSGITexture.Get(), nullptr, &SSGISRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &HistoryTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(HistoryTexture.Get(), nullptr, &HistoryRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(HistoryTexture.Get(), nullptr, &HistorySRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &BlurredTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(BlurredTexture.Get(), nullptr, &BlurredRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(BlurredTexture.Get(), nullptr, &BlurredSRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &BlurTempTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(BlurTempTexture.Get(), nullptr, &BlurTempRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(BlurTempTexture.Get(), nullptr, &BlurTempSRV);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KSSGI::CreateNoiseTexture(ID3D11Device* InDevice)
{
    const UINT32 NoiseSize = 4;
    std::vector<float> noiseData(NoiseSize * NoiseSize * 4);

    for (UINT32 i = 0; i < NoiseSize * NoiseSize; ++i)
    {
        float angle = static_cast<float>(rand()) / RAND_MAX * XM_2PI;
        noiseData[i * 4 + 0] = cosf(angle);
        noiseData[i * 4 + 1] = sinf(angle);
        noiseData[i * 4 + 2] = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
        noiseData[i * 4 + 3] = 0.0f;
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = NoiseSize;
    texDesc.Height = NoiseSize;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = noiseData.data();
    initData.SysMemPitch = NoiseSize * sizeof(float) * 4;

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, &initData, &NoiseTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(NoiseTexture.Get(), nullptr, &NoiseSRV);
    return hr;
}

HRESULT KSSGI::CreateShaders(ID3D11Device* InDevice)
{
    SSGIShader = std::make_shared<KShaderProgram>();

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

    std::string ssgiPsSource = R"(
cbuffer SSGIConstantBuffer : register(b0)
{
    matrix Projection;
    matrix InverseProjection;
    matrix View;
    float2 Resolution;
    float Radius;
    float Intensity;
    float SampleCount;
    float MaxSteps;
    float Thickness;
    float Falloff;
    float TemporalBlend;
    int FrameIndex;
    float Padding;
};

Texture2D NormalTexture : register(t0);
Texture2D PositionTexture : register(t1);
Texture2D DepthTexture : register(t2);
Texture2D AlbedoTexture : register(t3);
Texture2D NoiseTexture : register(t4);

SamplerState PointSampler : register(s0);
SamplerState ClampSampler : register(s2);

static const float3 SampleKernel[16] = {
    float3( 0.538125,  0.185659,  0.431496),
    float3( 0.137907,  0.248642,  0.443018),
    float3( 0.337150,  0.567940, -0.400222),
    float3(-0.699980,  0.045961, -0.683418),
    float3( 0.156787,  0.076457, -0.224713),
    float3(-0.414697,  0.012181,  0.507472),
    float3( 0.044054, -0.439998,  0.478456),
    float3(-0.310667, -0.444471, -0.348071),
    float3(-0.081857, -0.582839,  0.090033),
    float3( 0.165078,  0.214468,  0.501115),
    float3(-0.289901,  0.025673,  0.417858),
    float3( 0.370941, -0.094615, -0.091726),
    float3(-0.215918, -0.108169,  0.389033),
    float3( 0.241135, -0.186467,  0.266698),
    float3( 0.139793, -0.340842, -0.431964),
    float3(-0.226705,  0.387711, -0.268966)
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 GetViewPosFromDepth(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 viewPos = mul(InverseProjection, clipPos);
    return viewPos.xyz / viewPos.w;
}

float3 ScreenSpaceToViewPos(float2 uv, float depth)
{
    return GetViewPosFromDepth(uv, depth);
}

float3 UVToViewPos(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 viewPos = mul(InverseProjection, clipPos);
    return viewPos.xyz / viewPos.w;
}

float2 ViewPosToUV(float3 viewPos)
{
    float4 clipPos = mul(Projection, float4(viewPos, 1.0));
    float2 uv = clipPos.xy / clipPos.w;
    return uv * 0.5 + 0.5;
}

float3 SampleIndirectLighting(float3 origin, float3 normal, float2 uv, float2 noiseScale)
{
    float3 indirectLight = float3(0.0, 0.0, 0.0);

    float3 noiseVec = NoiseTexture.Sample(PointSampler, uv * noiseScale).xyz;

    float3 tangent = normalize(noiseVec - normal * dot(noiseVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 tbn = float3x3(tangent, bitangent, normal);

    float stepSize = Radius / SampleCount;

    for (int i = 0; i < 16; i++)
    {
        float3 sampleDir = mul(SampleKernel[i], tbn);
        float3 samplePos = origin + sampleDir * Radius;

        float2 sampleUV = ViewPosToUV(samplePos);

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0)
        {
            continue;
        }

        float sampleDepth = DepthTexture.Sample(PointSampler, sampleUV).r;
        float3 sampleViewPos = UVToViewPos(sampleUV, sampleDepth);

        float3 diff = sampleViewPos - origin;
        float dist = length(diff);
        float3 sampleNormal = NormalTexture.Sample(PointSampler, sampleUV).xyz;
        sampleNormal = normalize(sampleNormal * 2.0 - 1.0);

        if (dist < Radius && dot(sampleNormal, normalize(diff)) > 0.0)
        {
            float3 albedo = AlbedoTexture.Sample(PointSampler, sampleUV).rgb;

            float attenuation = 1.0 - saturate(dist / Radius);
            attenuation = pow(attenuation, Falloff);

            float nDotL = max(dot(normal, normalize(diff)), 0.0);
            float receiverNDotL = max(dot(normal, sampleNormal), 0.0);

            indirectLight += albedo * attenuation * nDotL * receiverNDotL;
        }
    }

    return indirectLight / 16.0 * Intensity;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 normal = NormalTexture.Sample(PointSampler, input.TexCoord).xyz;
    normal = normalize(normal * 2.0 - 1.0);

    float depth = DepthTexture.Sample(PointSampler, input.TexCoord).r;

    float3 viewPos = PositionTexture.Sample(PointSampler, input.TexCoord).xyz;
    if (length(viewPos) < 0.001)
    {
        viewPos = GetViewPosFromDepth(input.TexCoord, depth);
    }

    float2 noiseScale = Resolution / 4.0;

    float3 indirectLight = SampleIndirectLighting(viewPos, normal, input.TexCoord, noiseScale);

    return float4(indirectLight, 1.0);
}
)";

    HRESULT hr = SSGIShader->CompileFromSource(InDevice, vsSource, ssgiPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile SSGI shader");
        return hr;
    }

    BlurShader = std::make_shared<KShaderProgram>();

    std::string blurPsSource = R"(
cbuffer BlurConstantBuffer : register(b0)
{
    float2 Resolution;
    float2 Direction;
    float BlurStrength;
    float3 Padding;
};

Texture2D SourceTexture : register(t0);

SamplerState LinearSampler : register(s1);

static const float Weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
static const float Offsets[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 texelSize = 1.0 / Resolution;
    float3 result = SourceTexture.Sample(LinearSampler, input.TexCoord).rgb * Weights[0];

    for (int i = 1; i < 5; i++)
    {
        float2 offset = Direction * texelSize * Offsets[i] * BlurStrength;
        result += SourceTexture.Sample(LinearSampler, input.TexCoord + offset).rgb * Weights[i];
        result += SourceTexture.Sample(LinearSampler, input.TexCoord - offset).rgb * Weights[i];
    }

    return float4(result, 1.0);
}
)";

    hr = BlurShader->CompileFromSource(InDevice, vsSource, blurPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile blur shader");
        return hr;
    }

    return S_OK;
}

HRESULT KSSGI::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSSGIConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &SSGIConstantBuffer);
    if (FAILED(hr)) return hr;

    bufferDesc.ByteWidth = sizeof(FBlurConstantBuffer);
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &BlurConstantBuffer);
    return hr;
}

HRESULT KSSGI::CreateSamplerStates(ID3D11Device* InDevice)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = InDevice->CreateSamplerState(&samplerDesc, &PointSamplerState);
    if (FAILED(hr)) return hr;

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = InDevice->CreateSamplerState(&samplerDesc, &LinearSamplerState);
    if (FAILED(hr)) return hr;

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = InDevice->CreateSamplerState(&samplerDesc, &ClampSamplerState);
    return hr;
}

HRESULT KSSGI::CreateFullscreenQuad(ID3D11Device* InDevice)
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

void KSSGI::ComputeSSGI(ID3D11DeviceContext* Context,
                        ID3D11ShaderResourceView* NormalSRV,
                        ID3D11ShaderResourceView* PositionSRV,
                        ID3D11ShaderResourceView* DepthSRV,
                        ID3D11ShaderResourceView* AlbedoSRV,
                        const XMMATRIX& Projection,
                        const XMMATRIX& View)
{
    if (!Parameters.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &SSGIRTV, nullptr);
    Context->ClearRenderTargetView(SSGIRTV.Get(), DefaultClearColor);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(SSGIConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FSSGIConstantBuffer* cb = reinterpret_cast<FSSGIConstantBuffer*>(mapped.pData);
        cb->Projection = XMMatrixTranspose(Projection);
        cb->InverseProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, Projection));
        cb->View = XMMatrixTranspose(View);
        cb->Resolution = XMFLOAT2((float)Width, (float)Height);
        cb->Radius = Parameters.Radius;
        cb->Intensity = Parameters.Intensity;
        cb->SampleCount = Parameters.SampleCount;
        cb->MaxSteps = Parameters.MaxSteps;
        cb->Thickness = Parameters.Thickness;
        cb->Falloff = Parameters.Falloff;
        cb->TemporalBlend = Parameters.TemporalBlend;
        cb->FrameIndex = FrameIndex++;
        Context->Unmap(SSGIConstantBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { SSGIConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 1, cbs);
    Context->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { NormalSRV, PositionSRV, DepthSRV, AlbedoSRV, NoiseSRV.Get() };
    Context->PSSetShaderResources(0, 5, srvs);

    ID3D11SamplerState* samplers[] = { PointSamplerState.Get(), LinearSamplerState.Get(), ClampSamplerState.Get() };
    Context->PSSetSamplers(0, 3, samplers);

    SSGIShader->Bind(Context);
    RenderFullscreenQuad(Context, SSGIShader->GetInputLayout());
    SSGIShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    Context->PSSetShaderResources(0, 5, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);

    if (Parameters.bBlurEnabled)
    {
        ApplyBlur(Context);
    }
}

void KSSGI::ApplyBlur(ID3D11DeviceContext* Context)
{
    if (!Parameters.bBlurEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;

    for (int pass = 0; pass < 2; pass++)
    {
        ID3D11ShaderResourceView* inputSRV = (pass == 0) ? SSGISRV.Get() : BlurTempSRV.Get();
        ID3D11RenderTargetView* outputRTV = (pass == 0) ? BlurTempRTV.Get() : BlurredRTV.Get();

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(Context->Map(BlurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            FBlurConstantBuffer* cb = reinterpret_cast<FBlurConstantBuffer*>(mapped.pData);
            cb->Resolution = XMFLOAT2((float)Width, (float)Height);
            cb->Direction = (pass == 0) ? XMFLOAT2(1.0f, 0.0f) : XMFLOAT2(0.0f, 1.0f);
            cb->BlurStrength = Parameters.BlurStrength;
            Context->Unmap(BlurConstantBuffer.Get(), 0);
        }

        ID3D11Buffer* cbs[] = { BlurConstantBuffer.Get() };
        Context->VSSetConstantBuffers(0, 1, cbs);
        Context->PSSetConstantBuffers(0, 1, cbs);

        Context->OMSetRenderTargets(1, &outputRTV, nullptr);
        Context->ClearRenderTargetView(outputRTV, DefaultClearColor);

        ID3D11ShaderResourceView* srvs[] = { inputSRV };
        Context->PSSetShaderResources(0, 1, srvs);

        Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

        BlurShader->Bind(Context);
        RenderFullscreenQuad(Context, BlurShader->GetInputLayout());
        BlurShader->Unbind(Context);

        ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
        Context->PSSetShaderResources(0, 1, nullSRVs);
        Context->OMSetRenderTargets(1, &nullRTV, nullptr);
    }
}

void KSSGI::RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout)
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
