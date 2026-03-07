#include "LensEffects.h"
#include "../../Core/Engine.h"

HRESULT KLensEffects::Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height)
{
    if (bInitialized)
    {
        Cleanup();
    }

    this->Device = Device;
    this->Width = Width;
    this->Height = Height;

    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));

    HRESULT hr = S_OK;

    hr = CreateRenderTargets(Device);
    if (FAILED(hr)) return hr;

    hr = CreateShaders(Device);
    if (FAILED(hr)) return hr;

    hr = CreateConstantBuffers(Device);
    if (FAILED(hr)) return hr;

    hr = CreateSamplerStates(Device);
    if (FAILED(hr)) return hr;

    hr = CreateFullscreenQuad(Device);
    if (FAILED(hr)) return hr;

    bInitialized = true;
    return S_OK;
}

void KLensEffects::Cleanup()
{
    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();
    LensEffectsShader.reset();
    LensEffectsConstantBuffer.Reset();
    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    FullscreenQuadMesh.reset();
    bInitialized = false;
}

HRESULT KLensEffects::Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
{
    Width = NewWidth;
    Height = NewHeight;
    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));

    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();

    return CreateRenderTargets(Device);
}

HRESULT KLensEffects::CreateRenderTargets(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Width;
    texDesc.Height = Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &OutputTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create lens effects output texture");
        return hr;
    }

    hr = Device->CreateRenderTargetView(OutputTexture.Get(), nullptr, &OutputRTV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create lens effects output RTV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(OutputTexture.Get(), nullptr, &OutputSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create lens effects output SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KLensEffects::CreateShaders(ID3D11Device* Device)
{
    const char* vsSource = R"(
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

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = float4(input.Position.xy, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;
    return output;
}
)";

    const char* psSource = R"(
cbuffer LensEffectsBuffer : register(b0)
{
    bool bChromaticAberrationEnabled;
    float ChromaticAberrationStrength;
    float ChromaticAberrationOffset;
    bool bVignetteEnabled;
    float VignetteIntensity;
    float VignetteSmoothness;
    float VignetteRoundness;
    bool bFilmGrainEnabled;
    float FilmGrainIntensity;
    float FilmGrainSpeed;
    float Time;
    float2 Resolution;
    float Padding[3];
};

Texture2D InputTexture : register(t0);
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float3 ApplyChromaticAberration(float2 uv, float3 color)
{
    if (!bChromaticAberrationEnabled)
        return color;

    float2 center = uv - 0.5;
    float dist = length(center);
    float2 direction = normalize(center);
    
    float2 offset = direction * dist * ChromaticAberrationStrength;
    
    float r = InputTexture.Sample(LinearSampler, uv + offset).r;
    float g = color.g;
    float b = InputTexture.Sample(LinearSampler, uv - offset).b;
    
    return float3(r, g, b);
}

float ApplyVignette(float2 uv)
{
    if (!bVignetteEnabled)
        return 1.0;

    float2 center = uv - 0.5;
    float dist = length(center) * 2.0;
    
    float vignette = 1.0 - smoothstep(1.0 - VignetteSmoothness, 1.0, dist * VignetteRoundness);
    vignette = lerp(1.0, vignette, VignetteIntensity);
    
    return vignette;
}

float3 ApplyFilmGrain(float2 uv, float3 color)
{
    if (!bFilmGrainEnabled)
        return color;

    float grainTime = Time * FilmGrainSpeed;
    float grain = Random(uv + grainTime) * 2.0 - 1.0;
    
    return color + grain * FilmGrainIntensity;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float3 color = InputTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    color = ApplyChromaticAberration(input.TexCoord, color);
    
    float vignette = ApplyVignette(input.TexCoord);
    color *= vignette;
    
    color = ApplyFilmGrain(input.TexCoord, color);
    
    return float4(color, 1.0);
}
)";

    LensEffectsShader = std::make_shared<KShaderProgram>();
    HRESULT hr = LensEffectsShader->CompileFromSource(Device, vsSource, psSource, "VSMain", "PSMain");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile lens effects shader");
        return hr;
    }

    return S_OK;
}

HRESULT KLensEffects::CreateConstantBuffers(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bufferDesc.ByteWidth = sizeof(FLensEffectsParams);
    bufferDesc.ByteWidth = (bufferDesc.ByteWidth + 15) & ~15;

    HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &LensEffectsConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create lens effects constant buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KLensEffects::CreateSamplerStates(ID3D11Device* Device)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = Device->CreateSamplerState(&samplerDesc, &PointSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create point sampler state");
        return hr;
    }

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    hr = Device->CreateSamplerState(&samplerDesc, &LinearSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create linear sampler state");
        return hr;
    }

    return S_OK;
}

HRESULT KLensEffects::CreateFullscreenQuad(ID3D11Device* Device)
{
    FullscreenQuadMesh = std::make_shared<KMesh>();
    
    FVertex vertices[] = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT4(1,1,1,1), XMFLOAT3(0,0,-1), XMFLOAT2(1.0f, 1.0f) }
    };

    UINT32 indices[] = { 0, 1, 2, 2, 1, 3 };

    return FullscreenQuadMesh->Initialize(Device, vertices, 4, indices, 6);
}

void KLensEffects::ApplyLensEffects(ID3D11DeviceContext* Context,
                                     ID3D11ShaderResourceView* InputSRV,
                                     ID3D11RenderTargetView* OutputRTV,
                                     float DeltaTime)
{
    if (!bInitialized)
    {
        return;
    }

    bool anyEnabled = Parameters.bChromaticAberrationEnabled || 
                      Parameters.bVignetteEnabled || 
                      Parameters.bFilmGrainEnabled;
    
    if (!anyEnabled)
    {
        return;
    }

    ID3D11RenderTargetView* renderTargets[] = { OutputRTV };
    Context->OMSetRenderTargets(1, renderTargets, nullptr);

    UpdateConstantBuffer(Context, DeltaTime);

    LensEffectsShader->Bind(Context);

    Context->PSSetConstantBuffers(0, 1, LensEffectsConstantBuffer.GetAddressOf());

    Context->PSSetShaderResources(0, 1, &InputSRV);

    ID3D11SamplerState* samplers[] = { PointSamplerState.Get(), LinearSamplerState.Get() };
    Context->PSSetSamplers(0, 2, samplers);

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}

void KLensEffects::UpdateConstantBuffer(ID3D11DeviceContext* Context, float DeltaTime)
{
    Parameters.Time += DeltaTime * Parameters.FilmGrainSpeed;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Context->Map(LensEffectsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &Parameters, sizeof(FLensEffectsParams));
        Context->Unmap(LensEffectsConstantBuffer.Get(), 0);
    }
}

void KLensEffects::RenderFullscreenQuad(ID3D11DeviceContext* Context)
{
    if (FullscreenQuadMesh)
    {
        UINT32 stride = sizeof(FVertex);
        UINT32 offset = 0;
        ID3D11Buffer* vertexBuffer = FullscreenQuadMesh->GetVertexBuffer();
        ID3D11Buffer* indexBuffer = FullscreenQuadMesh->GetIndexBuffer();

        Context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        Context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        Context->DrawIndexed(6, 0, 0);
    }
}
