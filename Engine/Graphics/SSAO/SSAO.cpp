#include "SSAO.h"
#include <random>
#include <DirectXMath.h>

using namespace DirectX;

struct FSSAOKernelBuffer
{
    XMFLOAT4 Samples[64];
    UINT32 KernelSize;
    float Radius;
    float Bias;
    float Power;
};

struct FSSAOConstantBuffer
{
    XMMATRIX Projection;
    XMMATRIX InverseProjection;
    XMMATRIX View;
    XMFLOAT2 NoiseScale;
    XMFLOAT2 Resolution;
};

struct FBlurConstantBuffer
{
    XMFLOAT2 TexelSize;
    XMFLOAT2 Direction;
};

HRESULT KSSAO::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
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

    hr = CreateSSAORenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSAO render targets");
        return hr;
    }

    GenerateSSAOKernel();
    GenerateNoiseTexture();

    hr = CreateNoiseTexture(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create noise texture");
        return hr;
    }

    hr = CreateKernelBuffer(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create kernel buffer");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create SSAO shaders");
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
    LOG_INFO("SSAO system initialized successfully");
    return S_OK;
}

void KSSAO::Cleanup()
{
    SSAOTexture.Reset();
    SSAORTV.Reset();
    SSAOSRV.Reset();

    for (int32 i = 0; i < 2; ++i)
    {
        BlurOutputTexture[i].Reset();
        BlurOutputRTV[i].Reset();
        BlurOutputSRV[i].Reset();
    }

    NoiseTexture.Reset();
    NoiseSRV.Reset();

    KernelBuffer.Reset();
    SSAOConstantBuffer.Reset();

    SSAOShader.reset();
    BlurShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();

    FullscreenQuadMesh.reset();

    SSAOKernel.clear();
    NoiseData.clear();

    bInitialized = false;
}

HRESULT KSSAO::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    SSAOTexture.Reset();
    SSAORTV.Reset();
    SSAOSRV.Reset();

    for (int32 i = 0; i < 2; ++i)
    {
        BlurOutputTexture[i].Reset();
        BlurOutputRTV[i].Reset();
        BlurOutputSRV[i].Reset();
    }

    return CreateSSAORenderTargets(InDevice);
}

HRESULT KSSAO::CreateSSAORenderTargets(ID3D11Device* InDevice)
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Width;
    texDesc.Height = Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &SSAOTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(SSAOTexture.Get(), nullptr, &SSAORTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(SSAOTexture.Get(), nullptr, &SSAOSRV);
    if (FAILED(hr)) return hr;

    for (int32 i = 0; i < 2; ++i)
    {
        hr = InDevice->CreateTexture2D(&texDesc, nullptr, &BlurOutputTexture[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateRenderTargetView(BlurOutputTexture[i].Get(), nullptr, &BlurOutputRTV[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateShaderResourceView(BlurOutputTexture[i].Get(), nullptr, &BlurOutputSRV[i]);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

void KSSAO::GenerateSSAOKernel()
{
    SSAOKernel.clear();
    SSAOKernel.resize(64);

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    for (UINT32 i = 0; i < 64; ++i)
    {
        float scale = (float)i / 64.0f;
        scale = 0.1f + (scale * scale) * 0.9f;

        XMFLOAT3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );

        XMVECTOR vec = XMLoadFloat3(&sample);
        vec = XMVector3Normalize(vec);
        vec = vec * (randomFloats(generator) * scale);

        XMStoreFloat3(&sample, vec);
        SSAOKernel[i] = XMFLOAT4(sample.x, sample.y, sample.z, 0.0f);
    }
}

void KSSAO::GenerateNoiseTexture()
{
    NoiseData.clear();
    NoiseData.resize(16);

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    for (UINT32 i = 0; i < 16; ++i)
    {
        XMFLOAT3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );

        XMVECTOR vec = XMLoadFloat3(&noise);
        vec = XMVector3Normalize(vec);
        XMStoreFloat3(&noise, vec);

        NoiseData[i] = noise;
    }
}

HRESULT KSSAO::CreateNoiseTexture(ID3D11Device* InDevice)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Parameters.NoiseTextureSize;
    texDesc.Height = Parameters.NoiseTextureSize;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    std::vector<XMFLOAT4> noiseDataRGBA(16);
    for (size_t i = 0; i < 16; ++i)
    {
        noiseDataRGBA[i] = XMFLOAT4(NoiseData[i].x, NoiseData[i].y, NoiseData[i].z, 1.0f);
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = noiseDataRGBA.data();
    initData.SysMemPitch = Parameters.NoiseTextureSize * sizeof(XMFLOAT4);

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, &initData, &NoiseTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(NoiseTexture.Get(), nullptr, &NoiseSRV);
    return hr;
}

HRESULT KSSAO::CreateKernelBuffer(ID3D11Device* InDevice)
{
    FSSAOKernelBuffer kernelData = {};
    kernelData.KernelSize = Parameters.KernelSize;
    kernelData.Radius = Parameters.Radius;
    kernelData.Bias = Parameters.Bias;
    kernelData.Power = Parameters.Power;

    for (size_t i = 0; i < SSAOKernel.size() && i < 64; ++i)
    {
        kernelData.Samples[i] = SSAOKernel[i];
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSSAOKernelBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &kernelData;

    return InDevice->CreateBuffer(&bufferDesc, &initData, &KernelBuffer);
}

HRESULT KSSAO::CreateShaders(ID3D11Device* InDevice)
{
    SSAOShader = std::make_shared<KShaderProgram>();

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

    std::string psSource = R"(
cbuffer SSAOKernelBuffer : register(b0)
{
    float4 Samples[64];
    uint KernelSize;
    float Radius;
    float Bias;
    float Power;
};

cbuffer SSAOConstantBuffer : register(b1)
{
    matrix Projection;
    matrix InverseProjection;
    matrix View;
    float2 NoiseScale;
    float2 Resolution;
};

Texture2D NormalTexture : register(t0);
Texture2D PositionTexture : register(t1);
Texture2D DepthTexture : register(t2);
Texture2D NoiseTexture : register(t3);

SamplerState PointSampler : register(s0);

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

float main(PS_INPUT input) : SV_TARGET
{
    float3 normal = NormalTexture.Sample(PointSampler, input.TexCoord).xyz;
    normal = normalize(normal * 2.0 - 1.0);

    float3 fragPos = PositionTexture.Sample(PointSampler, input.TexCoord).xyz;

    float depth = DepthTexture.Sample(PointSampler, input.TexCoord).r;

    float3 viewPos = fragPos;
    if (length(fragPos) < 0.001)
    {
        viewPos = GetViewPosFromDepth(input.TexCoord, depth);
    }

    float3 randomVec = NoiseTexture.Sample(PointSampler, input.TexCoord * NoiseScale).xyz;

    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for (uint i = 0; i < KernelSize; ++i)
    {
        float3 sampleVec = mul(Samples[i].xyz, TBN);
        float3 samplePos = viewPos + sampleVec * Radius;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(Projection, offset);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = DepthTexture.Sample(PointSampler, offset.xy).r;

        float4 sampleClip = float4(offset.xy * 2.0 - 1.0, sampleDepth, 1.0);
        float4 sampleView = mul(InverseProjection, sampleClip);
        sampleView.xyz /= sampleView.w;
        float realDepth = sampleView.z;

        float rangeCheck = smoothstep(0.0, 1.0, Radius / abs(viewPos.z - realDepth));
        occlusion += (realDepth >= samplePos.z + Bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / (float)KernelSize);
    occlusion = pow(occlusion, Power);

    return occlusion;
}
)";

    HRESULT hr = SSAOShader->CompileFromSource(InDevice, vsSource, psSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile SSAO shader");
        return hr;
    }

    BlurShader = std::make_shared<KShaderProgram>();

    std::string blurVsSource = vsSource;

    std::string blurPsSource = R"(
cbuffer BlurConstantBuffer : register(b0)
{
    float2 TexelSize;
    float2 Direction;
};

Texture2D InputTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 texOffset = TexelSize * Direction;

    float3 result = InputTexture.Sample(LinearSampler, input.TexCoord).rgb * 0.2270270270;

    result += InputTexture.Sample(LinearSampler, input.TexCoord + texOffset).rgb * 0.3162162162;
    result += InputTexture.Sample(LinearSampler, input.TexCoord - texOffset).rgb * 0.3162162162;

    result += InputTexture.Sample(LinearSampler, input.TexCoord + texOffset * 2.0).rgb * 0.0702702703;
    result += InputTexture.Sample(LinearSampler, input.TexCoord - texOffset * 2.0).rgb * 0.0702702703;

    return float4(result, 1.0);
}
)";

    hr = BlurShader->CompileFromSource(InDevice, blurVsSource, blurPsSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile blur shader");
        return hr;
    }

    return S_OK;
}

HRESULT KSSAO::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSSAOConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &SSAOConstantBuffer);
    return hr;
}

HRESULT KSSAO::CreateSamplerStates(ID3D11Device* InDevice)
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
    return hr;
}

HRESULT KSSAO::CreateFullscreenQuad(ID3D11Device* InDevice)
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

void KSSAO::ComputeSSAO(ID3D11DeviceContext* Context,
                        ID3D11ShaderResourceView* NormalSRV,
                        ID3D11ShaderResourceView* PositionSRV,
                        ID3D11ShaderResourceView* DepthSRV,
                        const XMMATRIX& Projection,
                        const XMMATRIX& View)
{
    if (!Parameters.bEnabled) return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &SSAORTV, nullptr);
    Context->ClearRenderTargetView(SSAORTV.Get(), DefaultClearColor);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(Context->Map(SSAOConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FSSAOConstantBuffer* cb = reinterpret_cast<FSSAOConstantBuffer*>(mapped.pData);
        cb->Projection = XMMatrixTranspose(Projection);
        cb->InverseProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, Projection));
        cb->View = XMMatrixTranspose(View);
        cb->NoiseScale = XMFLOAT2(
            (float)Width / (float)Parameters.NoiseTextureSize,
            (float)Height / (float)Parameters.NoiseTextureSize
        );
        cb->Resolution = XMFLOAT2((float)Width, (float)Height);
        Context->Unmap(SSAOConstantBuffer.Get(), 0);
    }

    if (SUCCEEDED(Context->Map(KernelBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        FSSAOKernelBuffer* kb = reinterpret_cast<FSSAOKernelBuffer*>(mapped.pData);
        kb->KernelSize = Parameters.KernelSize;
        kb->Radius = Parameters.Radius;
        kb->Bias = Parameters.Bias;
        kb->Power = Parameters.Power;
        for (size_t i = 0; i < SSAOKernel.size() && i < 64; ++i)
        {
            kb->Samples[i] = SSAOKernel[i];
        }
        Context->Unmap(KernelBuffer.Get(), 0);
    }

    ID3D11Buffer* cbs[] = { KernelBuffer.Get(), SSAOConstantBuffer.Get() };
    Context->VSSetConstantBuffers(0, 2, cbs);
    Context->PSSetConstantBuffers(0, 2, cbs);

    ID3D11ShaderResourceView* srvs[] = { NormalSRV, PositionSRV, DepthSRV, NoiseSRV.Get() };
    Context->PSSetShaderResources(0, 4, srvs);

    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    SSAOShader->Bind(Context);
    RenderFullscreenQuad(Context);
    SSAOShader->Unbind(Context);

    ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Context->PSSetShaderResources(0, 4, nullSRVs);
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);

    if (Parameters.bBlurEnabled)
    {
        BlurAO(Context);
    }
}

void KSSAO::BlurAO(ID3D11DeviceContext* Context)
{
    ID3D11RenderTargetView* nullRTV = nullptr;
    ID3D11ShaderResourceView* nullSRV = nullptr;

    FBlurConstantBuffer blurData;
    blurData.TexelSize = XMFLOAT2(1.0f / Width, 1.0f / Height);

    BlurShader->Bind(Context);

    ID3D11Buffer* blurCB = nullptr;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(FBlurConstantBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Device->CreateBuffer(&desc, nullptr, &blurCB);

    for (UINT32 i = 0; i < Parameters.BlurIterations; ++i)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(Context->Map(blurCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            FBlurConstantBuffer* data = reinterpret_cast<FBlurConstantBuffer*>(mapped.pData);
            data->TexelSize = blurData.TexelSize;
            data->Direction = (i % 2 == 0) ? XMFLOAT2(1.0f, 0.0f) : XMFLOAT2(0.0f, 1.0f);
            Context->Unmap(blurCB, 0);
        }

        Context->VSSetConstantBuffers(0, 1, &blurCB);
        Context->PSSetConstantBuffers(0, 1, &blurCB);

        ID3D11ShaderResourceView* inputSRV = (i == 0) ? SSAOSRV.Get() : BlurOutputSRV[1 - (i % 2)].Get();
        int32 outputIndex = i % 2;

        Context->OMSetRenderTargets(1, &BlurOutputRTV[outputIndex], nullptr);
        Context->ClearRenderTargetView(BlurOutputRTV[outputIndex].Get(), DefaultClearColor);

        Context->PSSetShaderResources(0, 1, &inputSRV);
        Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

        RenderFullscreenQuad(Context);

        Context->PSSetShaderResources(0, 1, &nullSRV);
        Context->OMSetRenderTargets(1, &nullRTV, nullptr);
    }

    if (blurCB)
    {
        blurCB->Release();
        blurCB = nullptr;
    }
    BlurShader->Unbind(Context);
}

void KSSAO::ApplyAO(ID3D11DeviceContext* Context)
{
}

void KSSAO::RenderFullscreenQuad(ID3D11DeviceContext* Context)
{
    if (!FullscreenQuadMesh) return;

    UINT32 stride = sizeof(float) * 5;
    UINT32 offset = 0;
    ID3D11Buffer* vb = FullscreenQuadMesh->GetVertexBuffer();
    ID3D11Buffer* ib = FullscreenQuadMesh->GetIndexBuffer();

    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (SSAOShader)
    {
        Context->IASetInputLayout(SSAOShader->GetInputLayout());
    }

    Context->DrawIndexed(6, 0, 0);
}

ID3D11ShaderResourceView* KSSAO::GetAOOutputSRV() const
{
    if (Parameters.bBlurEnabled && BlurOutputSRV[0])
    {
        return BlurOutputSRV[0].Get();
    }
    return SSAOSRV.Get();
}
