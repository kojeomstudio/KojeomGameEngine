#include "SSR.h"
#include <DirectXMath.h>

using namespace DirectX;

struct FSSRConstantBuffer
{
    XMMATRIX Projection;
    XMMATRIX InverseProjection;
    XMMATRIX View;
    XMFLOAT2 Resolution;
    float MaxDistance;
    float ResolutionScale;
    float Thickness;
    float StepCount;
    float MaxSteps;
    float EdgeFade;
    float FresnelPower;
    float Intensity;
    float Padding;
};

struct FCombineConstantBuffer
{
    float Intensity;
    float Padding[3];
};

HRESULT KSSR::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
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

    hr = CreateSSRRenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSR render targets");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSR shaders");
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
    LOG_INFO("SSR system initialized successfully");
    return S_OK;
}

void KSSR::Cleanup()
{
    SSRTexture.Reset();
    SSRRTV.Reset();
    SSRSRV.Reset();

    CombinedTexture.Reset();
    CombinedRTV.Reset();
    CombinedSRV.Reset();

    SSRConstantBuffer.Reset();
    CombineConstantBuffer.Reset();

    SSRShader.reset();
    CombineShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    ClampSamplerState.Reset();

    FullscreenQuadMesh.reset();

    bInitialized = false;
}

HRESULT KSSR::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    SSRTexture.Reset();
    SSRRTV.Reset();
    SSRSRV.Reset();

    CombinedTexture.Reset();
    CombinedRTV.Reset();
    CombinedSRV.Reset();

    return CreateSSRRenderTargets(InDevice);
}

HRESULT KSSR::CreateSSRRenderTargets(ID3D11Device* InDevice)
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

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &SSRTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(SSRTexture.Get(), nullptr, &SSRRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(SSRTexture.Get(), nullptr, &SSRSRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &CombinedTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(CombinedTexture.Get(), nullptr, &CombinedRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(CombinedTexture.Get(), nullptr, &CombinedSRV);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KSSR::CreateShaders(ID3D11Device* InDevice)
{
    SSRShader = std::make_shared<KShaderProgram>();

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

    std::string ssrPsSource = R"(
cbuffer SSRConstantBuffer : register(b0)
{
    matrix Projection;
    matrix InverseProjection;
    matrix View;
    float2 Resolution;
    float MaxDistance;
    float ResolutionScale;
    float Thickness;
    float StepCount;
    float MaxSteps;
    float EdgeFade;
    float FresnelPower;
    float Intensity;
    float Padding;
};

Texture2D ColorTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D PositionTexture : register(t2);
Texture2D DepthTexture : register(t3);

SamplerState PointSampler : register(s0);
SamplerState ClampSampler : register(s2);

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

float2 RayMarch(float3 rayOrigin, float3 rayDir, out float hitDepth)
{
    float stepSize = MaxDistance / StepCount;
    float3 currentPos = rayOrigin;
    float2 hitUV = float2(-1.0, -1.0);
    hitDepth = 1.0;

    [loop]
    for (float i = 0; i < MaxSteps; i += 1.0)
    {
        currentPos += rayDir * stepSize;

        float4 projected = mul(Projection, float4(currentPos, 1.0));
        projected.xyz /= projected.w;
        float2 screenUV = projected.xy * 0.5 + 0.5;

        if (screenUV.x < 0.0 || screenUV.x > 1.0 || 
            screenUV.y < 0.0 || screenUV.y > 1.0)
        {
            break;
        }

        float sampledDepth = DepthTexture.SampleLevel(PointSampler, screenUV, 0).r;
        float4 sampledClip = float4(screenUV * 2.0 - 1.0, sampledDepth, 1.0);
        float4 sampledView = mul(InverseProjection, sampledClip);
        float3 sampledPos = sampledView.xyz / sampledView.w;

        float depthDiff = currentPos.z - sampledPos.z;

        if (depthDiff > 0.0 && depthDiff < Thickness)
        {
            hitUV = screenUV;
            hitDepth = sampledDepth;
            break;
        }
    }

    return hitUV;
}

float EdgeFadeFactor(float2 uv)
{
    float2 edge = abs(uv * 2.0 - 1.0);
    float fade = 1.0 - smoothstep(1.0 - EdgeFade, 1.0, max(edge.x, edge.y));
    return fade;
}

float FresnelSchlick(float cosTheta, float f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, FresnelPower);
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

    float3 viewDir = normalize(-viewPos);

    float NdotV = max(dot(normal, viewDir), 0.0);
    float fresnel = FresnelSchlick(NdotV, 0.04);

    float3 reflectDir = normalize(reflect(-viewDir, normal));

    if (reflectDir.z > 0.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    float hitDepth;
    float2 hitUV = RayMarch(viewPos, reflectDir, hitDepth);

    if (hitUV.x < 0.0 || hitUV.y < 0.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    float3 reflectionColor = ColorTexture.Sample(ClampSampler, hitUV).rgb;

    float edgeFade = EdgeFadeFactor(hitUV);

    float alpha = fresnel * edgeFade * Intensity;

    return float4(reflectionColor * alpha, alpha);
}
)";

    HRESULT hr = SSRShader->CompileFromSource(InDevice, vsSource, ssrPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile SSR shader");
        return hr;
    }

    CombineShader = std::make_shared<KShaderProgram>();

    std::string combinePsSource = R"(
cbuffer CombineConstantBuffer : register(b0)
{
    float Intensity;
    float3 Padding;
};

Texture2D SourceTexture : register(t0);
Texture2D SSRTexture : register(t1);

SamplerState LinearSampler : register(s1);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 sourceColor = SourceTexture.Sample(LinearSampler, input.TexCoord).rgb;
    float4 ssrColor = SSRTexture.Sample(LinearSampler, input.TexCoord);

    float3 result = sourceColor + ssrColor.rgb * ssrColor.a * Intensity;

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

HRESULT KSSR::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSSRConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &SSRConstantBuffer);
    if (FAILED(hr)) return hr;

    bufferDesc.ByteWidth = sizeof(FCombineConstantBuffer);
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &CombineConstantBuffer);
    return hr;
}

HRESULT KSSR::CreateSamplerStates(ID3D11Device* InDevice)
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

HRESULT KSSR::CreateFullscreenQuad(ID3D11Device* InDevice)
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

void KSSR::ComputeSSR(ID3D11DeviceContext* Context,
                      ID3D11ShaderResourceView* ColorSRV,
                      ID3D11ShaderResourceView* NormalSRV,
                      ID3D11ShaderResourceView* PositionSRV,
                      ID3D11ShaderResourceView* DepthSRV,
                      const XMMATRIX& Projection,
                      const XMMATRIX& View)
{
    if (!Parameters.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &SSRRTV, nullptr);
    Context->ClearRenderTargetView(SSRRTV.Get(), DefaultClearColor);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(SSRConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FSSRConstantBuffer* cb = reinterpret_cast<FSSRConstantBuffer*>(mapped.pData);
        cb->Projection = XMMatrixTranspose(Projection);
        cb->InverseProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, Projection));
        cb->View = XMMatrixTranspose(View);
        cb->Resolution = XMFLOAT2((float)Width, (float)Height);
        cb->MaxDistance = Parameters.MaxDistance;
        cb->ResolutionScale = Parameters.Resolution;
        cb->Thickness = Parameters.Thickness;
        cb->StepCount = Parameters.StepCount;
        cb->MaxSteps = Parameters.MaxSteps;
        cb->EdgeFade = Parameters.EdgeFade;
        cb->FresnelPower = Parameters.FresnelPower;
        cb->Intensity = Parameters.Intensity;
        Context->Unmap(SSRConstantBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { SSRConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 1, cbs);
    Context->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { ColorSRV, NormalSRV, PositionSRV, DepthSRV };
    Context->PSSetShaderResources(0, 4, srvs);

    ID3D11SamplerState* samplers[] = { PointSamplerState.Get(), LinearSamplerState.Get(), ClampSamplerState.Get() };
    Context->PSSetSamplers(0, 3, samplers);

    SSRShader->Bind(Context);
    RenderFullscreenQuad(Context, SSRShader->GetInputLayout());
    SSRShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Context->PSSetShaderResources(0, 4, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

void KSSR::ApplySSR(ID3D11DeviceContext* Context,
                    ID3D11ShaderResourceView* SourceSRV)
{
    if (!Parameters.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &CombinedRTV, nullptr);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(CombineConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FCombineConstantBuffer* cb = reinterpret_cast<FCombineConstantBuffer*>(mapped.pData);
        cb->Intensity = Parameters.Intensity;
        Context->Unmap(CombineConstantBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { CombineConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 1, cbs);
    Context->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { SourceSRV, SSRSRV.Get() };
    Context->PSSetShaderResources(0, 2, srvs);

    Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

    CombineShader->Bind(Context);
    RenderFullscreenQuad(Context, CombineShader->GetInputLayout());
    CombineShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

void KSSR::RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout)
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
