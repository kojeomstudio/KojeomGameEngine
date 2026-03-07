#include "MotionBlur.h"
#include "../../Core/Engine.h"

HRESULT KMotionBlur::Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height)
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

void KMotionBlur::Cleanup()
{
    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();
    MotionBlurShader.reset();
    MotionBlurConstantBuffer.Reset();
    MatrixConstantBuffer.Reset();
    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    FullscreenQuadMesh.reset();
    bInitialized = false;
}

HRESULT KMotionBlur::Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
{
    Width = NewWidth;
    Height = NewHeight;
    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));

    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();

    return CreateRenderTargets(Device);
}

HRESULT KMotionBlur::CreateRenderTargets(ID3D11Device* Device)
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
        LOG_ERROR("Failed to create motion blur output texture");
        return hr;
    }

    hr = Device->CreateRenderTargetView(OutputTexture.Get(), nullptr, &OutputRTV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create motion blur output RTV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(OutputTexture.Get(), nullptr, &OutputSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create motion blur output SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KMotionBlur::CreateShaders(ID3D11Device* Device)
{
    const char* vsSource = R"(
cbuffer MatrixBuffer : register(b0)
{
    matrix CurrentViewProj;
    matrix PreviousViewProj;
};

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
cbuffer MotionBlurBuffer : register(b0)
{
    float Intensity;
    uint MaxSamples;
    float MinVelocity;
    float MaxVelocity;
    bool bEnabled;
    float2 Resolution;
    float Padding[2];
};

Texture2D ColorTexture : register(t0);
Texture2D VelocityTexture : register(t1);
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float2 GetVelocity(float2 uv)
{
    float2 velocity = VelocityTexture.Sample(PointSampler, uv).xy;
    velocity = (velocity - 0.5) * 2.0;
    return velocity;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    if (!bEnabled)
    {
        return ColorTexture.Sample(PointSampler, input.TexCoord);
    }

    float2 velocity = GetVelocity(input.TexCoord);
    float speed = length(velocity) * MaxVelocity;

    if (speed < MinVelocity)
    {
        return ColorTexture.Sample(PointSampler, input.TexCoord);
    }

    float2 dir = normalize(velocity) * Intensity;
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    [unroll]
    for (uint i = 0; i < MaxSamples; i++)
    {
        float t = (float(i) / float(MaxSamples - 1)) - 0.5;
        float2 sampleUV = input.TexCoord + dir * t;
        
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || 
            sampleUV.y < 0.0 || sampleUV.y > 1.0)
        {
            continue;
        }

        float2 sampleVelocity = GetVelocity(sampleUV);
        float sampleSpeed = length(sampleVelocity) * MaxVelocity;
        
        float weight = 1.0 - abs(t) * 2.0;
        weight *= smoothstep(MinVelocity, MaxVelocity, sampleSpeed);
        
        color += ColorTexture.Sample(LinearSampler, sampleUV) * weight;
        totalWeight += weight;
    }

    if (totalWeight > 0.0)
    {
        color /= totalWeight;
    }
    else
    {
        color = ColorTexture.Sample(PointSampler, input.TexCoord);
    }

    return color;
}
)";

    MotionBlurShader = std::make_shared<KShaderProgram>();
    HRESULT hr = MotionBlurShader->CompileFromSource(Device, vsSource, psSource, "VSMain", "PSMain");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile motion blur shader");
        return hr;
    }

    return S_OK;
}

HRESULT KMotionBlur::CreateConstantBuffers(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    bufferDesc.ByteWidth = sizeof(FMotionBlurParams);
    HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &MotionBlurConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create motion blur constant buffer");
        return hr;
    }

    struct FMatrixBuffer
    {
        XMMATRIX CurrentViewProj;
        XMMATRIX PreviousViewProj;
    };
    bufferDesc.ByteWidth = sizeof(FMatrixBuffer);
    hr = Device->CreateBuffer(&bufferDesc, nullptr, &MatrixConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create matrix constant buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KMotionBlur::CreateSamplerStates(ID3D11Device* Device)
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

HRESULT KMotionBlur::CreateFullscreenQuad(ID3D11Device* Device)
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

void KMotionBlur::SetMatrices(const XMMATRIX& InCurrentViewProj, const XMMATRIX& InPreviousViewProj)
{
    PreviousViewProj = CurrentViewProj;
    CurrentViewProj = InCurrentViewProj;
}

void KMotionBlur::ApplyMotionBlur(ID3D11DeviceContext* Context,
                                   ID3D11ShaderResourceView* ColorSRV,
                                   ID3D11ShaderResourceView* VelocitySRV,
                                   ID3D11RenderTargetView* OutputRTV)
{
    if (!bInitialized || !Parameters.bEnabled)
    {
        return;
    }

    ID3D11RenderTargetView* renderTargets[] = { OutputRTV };
    Context->OMSetRenderTargets(1, renderTargets, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    Context->ClearRenderTargetView(OutputRTV, clearColor);

    UpdateConstantBuffer(Context);

    MotionBlurShader->Bind(Context);

    Context->PSSetConstantBuffers(0, 1, MotionBlurConstantBuffer.GetAddressOf());
    Context->PSSetConstantBuffers(1, 1, MatrixConstantBuffer.GetAddressOf());

    ID3D11ShaderResourceView* srvs[] = { ColorSRV, VelocitySRV };
    Context->PSSetShaderResources(0, 2, srvs);

    ID3D11SamplerState* samplers[] = { PointSamplerState.Get(), LinearSamplerState.Get() };
    Context->PSSetSamplers(0, 2, samplers);

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
}

void KMotionBlur::UpdateConstantBuffer(ID3D11DeviceContext* Context)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Context->Map(MotionBlurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &Parameters, sizeof(FMotionBlurParams));
        Context->Unmap(MotionBlurConstantBuffer.Get(), 0);
    }

    struct FMatrixBuffer
    {
        XMMATRIX CurrentViewProj;
        XMMATRIX PreviousViewProj;
    };
    FMatrixBuffer matrixBuffer;
    matrixBuffer.CurrentViewProj = XMMatrixTranspose(CurrentViewProj);
    matrixBuffer.PreviousViewProj = XMMatrixTranspose(PreviousViewProj);

    hr = Context->Map(MatrixConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &matrixBuffer, sizeof(FMatrixBuffer));
        Context->Unmap(MatrixConstantBuffer.Get(), 0);
    }
}

void KMotionBlur::RenderFullscreenQuad(ID3D11DeviceContext* Context)
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
