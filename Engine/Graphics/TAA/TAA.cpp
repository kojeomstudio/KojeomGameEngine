#include "TAA.h"
#include <DirectXMath.h>

using namespace DirectX;

struct FTAAConstantBuffer
{
    XMMATRIX CurrentViewProjection;
    XMMATRIX InverseCurrentViewProjection;
    XMMATRIX PreviousViewProjection;
    XMFLOAT2 Resolution;
    float BlendFactor;
    float FeedbackMin;
    float FeedbackMax;
    float MotionBlurScale;
    int32 bFirstFrame;
    float SharpenStrength;
    int32 bSharpenEnabled;
};

HRESULT KTAA::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
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

    hr = CreateHistoryTextures(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create TAA history textures");
        return hr;
    }

    hr = CreateOutputTexture(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create TAA output texture");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create TAA shaders");
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

    PreviousViewProjection = XMMatrixIdentity();
    bFirstFrame = true;

    bInitialized = true;
    LOG_INFO("TAA system initialized successfully");
    return S_OK;
}

void KTAA::Cleanup()
{
    for (int32 i = 0; i < 2; ++i)
    {
        HistoryTexture[i].Reset();
        HistoryRTV[i].Reset();
        HistorySRV[i].Reset();
    }

    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();

    TAAConstantBuffer.Reset();

    TAAShader.reset();
    SharpenShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();

    FullscreenQuadMesh.reset();

    bInitialized = false;
    bFirstFrame = true;
}

HRESULT KTAA::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    for (int32 i = 0; i < 2; ++i)
    {
        HistoryTexture[i].Reset();
        HistoryRTV[i].Reset();
        HistorySRV[i].Reset();
    }

    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();

    bFirstFrame = true;

    HRESULT hr = CreateHistoryTextures(InDevice);
    if (FAILED(hr)) return hr;

    return CreateOutputTexture(InDevice);
}

HRESULT KTAA::CreateHistoryTextures(ID3D11Device* InDevice)
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

    for (int32 i = 0; i < 2; ++i)
    {
        hr = InDevice->CreateTexture2D(&texDesc, nullptr, &HistoryTexture[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateRenderTargetView(HistoryTexture[i].Get(), nullptr, &HistoryRTV[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateShaderResourceView(HistoryTexture[i].Get(), nullptr, &HistorySRV[i]);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

HRESULT KTAA::CreateOutputTexture(ID3D11Device* InDevice)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Width;
    texDesc.Height = Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &OutputTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(OutputTexture.Get(), nullptr, &OutputRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(OutputTexture.Get(), nullptr, &OutputSRV);
    return hr;
}

HRESULT KTAA::CreateShaders(ID3D11Device* InDevice)
{
    TAAShader = std::make_shared<KShaderProgram>();

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

    std::string taaPsSource = R"(
cbuffer TAAConstantBuffer : register(b0)
{
    matrix CurrentViewProjection;
    matrix InverseCurrentViewProjection;
    matrix PreviousViewProjection;
    float2 Resolution;
    float BlendFactor;
    float FeedbackMin;
    float FeedbackMax;
    float MotionBlurScale;
    int bFirstFrame;
    float SharpenStrength;
    int bSharpenEnabled;
};

Texture2D CurrentColorTexture : register(t0);
Texture2D HistoryTexture : register(t1);
Texture2D VelocityTexture : register(t2);
Texture2D DepthTexture : register(t3);

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 RGBToYCoCg(float3 c)
{
    float y = c.r * 0.25 + c.g * 0.5 + c.b * 0.25;
    float co = c.r * 0.5 - c.b * 0.5;
    float cg = -c.r * 0.25 + c.g * 0.5 - c.b * 0.25;
    return float3(y, co, cg);
}

float3 YCoCgToRGB(float3 c)
{
    float r = c.x + c.y - c.z;
    float g = c.x + c.z;
    float b = c.x - c.y - c.z;
    return float3(r, g, b);
}

float3 ClipAABB(float3 color, float3 minColor, float3 maxColor)
{
    float3 center = 0.5 * (maxColor + minColor);
    float3 extents = 0.5 * (maxColor - minColor);
    
    float3 offset = color - center;
    float3 absOffset = abs(offset);
    float3 ts = extents / max(absOffset, 1e-6);
    
    float t = min(ts.x, min(ts.y, ts.z));
    t = saturate(t);
    
    return center + offset * t;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    if (bFirstFrame)
    {
        return CurrentColorTexture.Sample(PointSampler, input.TexCoord);
    }

    float3 currentColor = CurrentColorTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    float2 velocity = VelocityTexture.Sample(PointSampler, input.TexCoord).rg;
    float2 prevUV = input.TexCoord - velocity * MotionBlurScale;
    
    float3 historyColor = HistoryTexture.Sample(LinearSampler, prevUV).rgb;
    
    float2 texelSize = 1.0 / Resolution;
    float3 minColor = float3(999999.0, 999999.0, 999999.0);
    float3 maxColor = float3(-999999.0, -999999.0, -999999.0);
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 sampleUV = input.TexCoord + float2(x, y) * texelSize;
            float3 sampleColor = CurrentColorTexture.Sample(PointSampler, sampleUV).rgb;
            sampleColor = RGBToYCoCg(sampleColor);
            minColor = min(minColor, sampleColor);
            maxColor = max(maxColor, sampleColor);
        }
    }
    
    historyColor = RGBToYCoCg(historyColor);
    historyColor = ClipAABB(historyColor, minColor, maxColor);
    historyColor = YCoCgToRGB(historyColor);
    
    float2 clampedPrevUV = saturate(prevUV);
    float blendFactor = BlendFactor;
    blendFactor = lerp(FeedbackMin, FeedbackMax, length(velocity) * 100.0);
    blendFactor = lerp(BlendFactor, blendFactor, step(0.0, prevUV.x) * step(prevUV.x, 1.0) * 
                       step(0.0, prevUV.y) * step(prevUV.y, 1.0));
    
    float3 result = lerp(historyColor, currentColor, blendFactor);
    
    if (bSharpenEnabled && SharpenStrength > 0.0)
    {
        float3 blurred = float3(0.0, 0.0, 0.0);
        float weights[9] = {
            1.0, 2.0, 1.0,
            2.0, 4.0, 2.0,
            1.0, 2.0, 1.0
        };
        float weightSum = 16.0;
        int idx = 0;
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 sampleUV = input.TexCoord + float2(x, y) * texelSize;
                blurred += CurrentColorTexture.Sample(PointSampler, sampleUV).rgb * weights[idx];
                idx++;
            }
        }
        blurred /= weightSum;
        
        result = result + (result - blurred) * SharpenStrength;
    }
    
    return float4(result, 1.0);
}
)";

    HRESULT hr = TAAShader->CompileFromSource(InDevice, vsSource, taaPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile TAA shader");
        return hr;
    }

    return S_OK;
}

HRESULT KTAA::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FTAAConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &TAAConstantBuffer);
    return hr;
}

HRESULT KTAA::CreateSamplerStates(ID3D11Device* InDevice)
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

HRESULT KTAA::CreateFullscreenQuad(ID3D11Device* InDevice)
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

void KTAA::ApplyTAA(ID3D11DeviceContext* Context,
                    ID3D11ShaderResourceView* CurrentColorSRV,
                    ID3D11ShaderResourceView* VelocitySRV,
                    ID3D11ShaderResourceView* DepthSRV,
                    const XMMATRIX& CurrentViewProjection,
                    const XMMATRIX& PrevViewProjection)
{
    if (!Parameters.bEnabled) return;

    int32 historyIndex = CurrentHistoryIndex;
    int32 outputIndex = 1 - CurrentHistoryIndex;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &HistoryRTV[outputIndex], nullptr);
    Context->ClearRenderTargetView(HistoryRTV[outputIndex].Get(), DefaultClearColor);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(TAAConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FTAAConstantBuffer* cb = reinterpret_cast<FTAAConstantBuffer*>(mapped.pData);
        cb->CurrentViewProjection = XMMatrixTranspose(CurrentViewProjection);
        cb->InverseCurrentViewProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, CurrentViewProjection));
        cb->PreviousViewProjection = XMMatrixTranspose(PreviousViewProjection);
        cb->Resolution = XMFLOAT2((float)Width, (float)Height);
        cb->BlendFactor = Parameters.BlendFactor;
        cb->FeedbackMin = Parameters.FeedbackMin;
        cb->FeedbackMax = Parameters.FeedbackMax;
        cb->MotionBlurScale = Parameters.MotionBlurScale;
        cb->bFirstFrame = bFirstFrame ? 1 : 0;
        cb->SharpenStrength = Parameters.SharpenStrength;
        cb->bSharpenEnabled = Parameters.bSharpenEnabled ? 1 : 0;
        Context->Unmap(TAAConstantBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { TAAConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 1, cbs);
    Context->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { 
        CurrentColorSRV, 
        HistorySRV[historyIndex].Get(), 
        VelocitySRV, 
        DepthSRV 
    };
    Context->PSSetShaderResources(0, 4, srvs);

    ID3D11SamplerState* samplers[] = { PointSamplerState.Get(), LinearSamplerState.Get() };
    Context->PSSetSamplers(0, 2, samplers);

    TAAShader->Bind(Context);
    RenderFullscreenQuad(Context, TAAShader->GetInputLayout());
    TAAShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Context->PSSetShaderResources(0, 4, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);

    PreviousViewProjection = CurrentViewProjection;
    CurrentHistoryIndex = outputIndex;
    bFirstFrame = false;
}

void KTAA::RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout)
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
