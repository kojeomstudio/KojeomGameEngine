#include "DepthOfField.h"
#include "../../Core/Engine.h"

HRESULT KDepthOfField::Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height)
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

void KDepthOfField::Cleanup()
{
    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();
    CocTexture.Reset();
    CocRTV.Reset();
    CocSRV.Reset();
    BlurTexture.Reset();
    BlurRTV.Reset();
    BlurSRV.Reset();
    CocShader.reset();
    DoFShader.reset();
    DoFConstantBuffer.Reset();
    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    FullscreenQuadMesh.reset();
    bInitialized = false;
}

HRESULT KDepthOfField::Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
{
    Width = NewWidth;
    Height = NewHeight;
    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));

    OutputTexture.Reset();
    OutputRTV.Reset();
    OutputSRV.Reset();
    CocTexture.Reset();
    CocRTV.Reset();
    CocSRV.Reset();
    BlurTexture.Reset();
    BlurRTV.Reset();
    BlurSRV.Reset();

    return CreateRenderTargets(Device);
}

HRESULT KDepthOfField::CreateRenderTargets(ID3D11Device* Device)
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
        LOG_ERROR("Failed to create DoF output texture");
        return hr;
    }

    hr = Device->CreateRenderTargetView(OutputTexture.Get(), nullptr, &OutputRTV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DoF output RTV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(OutputTexture.Get(), nullptr, &OutputSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DoF output SRV");
        return hr;
    }

    texDesc.Format = DXGI_FORMAT_R8_UNORM;
    hr = Device->CreateTexture2D(&texDesc, nullptr, &CocTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create CoC texture");
        return hr;
    }

    hr = Device->CreateRenderTargetView(CocTexture.Get(), nullptr, &CocRTV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create CoC RTV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(CocTexture.Get(), nullptr, &CocSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create CoC SRV");
        return hr;
    }

    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateTexture2D(&texDesc, nullptr, &BlurTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create blur texture");
        return hr;
    }

    hr = Device->CreateRenderTargetView(BlurTexture.Get(), nullptr, &BlurRTV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create blur RTV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(BlurTexture.Get(), nullptr, &BlurSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create blur SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KDepthOfField::CreateShaders(ID3D11Device* Device)
{
    const char* cocVsSource = R"(
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

    const char* cocPsSource = R"(
cbuffer DoFBuffer : register(b0)
{
    float FocusDistance;
    float FocusRange;
    float BlurRadius;
    uint MaxBlurSamples;
    bool bEnabled;
    float NearBlurStart;
    float NearBlurEnd;
    float FarBlurStart;
    float FarBlurEnd;
    float2 Resolution;
    float Padding;
    float CameraNear;
    float CameraFar;
};

Texture2D DepthTexture : register(t0);
SamplerState PointSampler : register(s0);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float LinearizeDepth(float depth)
{
    float z = depth;
    return (2.0 * CameraNear) / (CameraFar + CameraNear - z * (CameraFar - CameraNear));
}

float CalculateCoC(float depth)
{
    float linearDepth = LinearizeDepth(depth);
    
    if (linearDepth < FocusDistance - FocusRange * 0.5)
    {
        return smoothstep(NearBlurStart, NearBlurEnd, linearDepth);
    }
    else if (linearDepth > FocusDistance + FocusRange * 0.5)
    {
        return smoothstep(FarBlurStart, FarBlurEnd, linearDepth);
    }
    
    return 0.0;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float depth = DepthTexture.Sample(PointSampler, input.TexCoord).r;
    float coc = CalculateCoC(depth);
    return float4(coc, 0.0, 0.0, 1.0);
}
)";

    const char* dofVsSource = R"(
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

    const char* dofPsSource = R"(
cbuffer DoFBuffer : register(b0)
{
    float FocusDistance;
    float FocusRange;
    float BlurRadius;
    uint MaxBlurSamples;
    bool bEnabled;
    float NearBlurStart;
    float NearBlurEnd;
    float FarBlurStart;
    float FarBlurEnd;
    float2 Resolution;
    float Padding;
    float CameraNear;
    float CameraFar;
};

Texture2D ColorTexture : register(t0);
Texture2D CocTexture : register(t1);
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    if (!bEnabled)
    {
        return ColorTexture.Sample(PointSampler, input.TexCoord);
    }

    float coc = CocTexture.Sample(PointSampler, input.TexCoord).r;
    
    if (coc < 0.01)
    {
        return ColorTexture.Sample(PointSampler, input.TexCoord);
    }

    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    float totalWeight = 0.0;
    
    float2 texelSize = 1.0 / Resolution;
    float blurRadius = coc * BlurRadius;

    [unroll]
    for (uint i = 0; i < 16; i++)
    {
        if (i >= MaxBlurSamples) break;
        
        float angle = float(i) * 2.3999633f;
        float radius = sqrt(float(i)) / sqrt(float(MaxBlurSamples));
        
        float2 offset = float2(cos(angle), sin(angle)) * radius * blurRadius * texelSize;
        float2 sampleUV = input.TexCoord + offset;
        
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0)
        {
            continue;
        }

        float sampleCoC = CocTexture.Sample(PointSampler, sampleUV).r;
        float weight = max(sampleCoC, coc);
        
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

    CocShader = std::make_shared<KShaderProgram>();
    HRESULT hr = CocShader->CompileFromSource(Device, cocVsSource, cocPsSource, "VSMain", "PSMain");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile CoC shader");
        return hr;
    }

    DoFShader = std::make_shared<KShaderProgram>();
    hr = DoFShader->CompileFromSource(Device, dofVsSource, dofPsSource, "VSMain", "PSMain");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile DoF shader");
        return hr;
    }

    return S_OK;
}

HRESULT KDepthOfField::CreateConstantBuffers(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    struct FDoFConstantBuffer
    {
        float FocusDistance;
        float FocusRange;
        float BlurRadius;
        UINT32 MaxBlurSamples;
        BOOL bEnabled;
        float NearBlurStart;
        float NearBlurEnd;
        float FarBlurStart;
        float FarBlurEnd;
        XMFLOAT2 Resolution;
        float Padding;
        float CameraNear;
        float CameraFar;
    };

    bufferDesc.ByteWidth = sizeof(FDoFConstantBuffer);
    bufferDesc.ByteWidth = (bufferDesc.ByteWidth + 15) & ~15;

    HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &DoFConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create DoF constant buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KDepthOfField::CreateSamplerStates(ID3D11Device* Device)
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

HRESULT KDepthOfField::CreateFullscreenQuad(ID3D11Device* Device)
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

void KDepthOfField::ApplyDepthOfField(ID3D11DeviceContext* Context,
                                       ID3D11ShaderResourceView* ColorSRV,
                                       ID3D11ShaderResourceView* DepthSRV,
                                       ID3D11RenderTargetView* OutputRTV)
{
    if (!bInitialized || !Parameters.bEnabled)
    {
        return;
    }

    UpdateConstantBuffer(Context);

    ID3D11RenderTargetView* renderTargets[] = { CocRTV.Get() };
    Context->OMSetRenderTargets(1, renderTargets, nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    Context->ClearRenderTargetView(CocRTV.Get(), clearColor);

    CocShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, DoFConstantBuffer.GetAddressOf());
    Context->PSSetShaderResources(0, 1, &DepthSRV);
    ID3D11SamplerState* samplers[] = { PointSamplerState.Get() };
    Context->PSSetSamplers(0, 1, samplers);
    RenderFullscreenQuad(Context);
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);

    renderTargets[0] = OutputRTV;
    Context->OMSetRenderTargets(1, renderTargets, nullptr);
    Context->ClearRenderTargetView(OutputRTV, clearColor);

    DoFShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, DoFConstantBuffer.GetAddressOf());
    ID3D11ShaderResourceView* srvs[] = { ColorSRV, CocSRV.Get() };
    Context->PSSetShaderResources(0, 2, srvs);
    ID3D11SamplerState* samplers2[] = { PointSamplerState.Get(), LinearSamplerState.Get() };
    Context->PSSetSamplers(0, 2, samplers2);
    RenderFullscreenQuad(Context);
    ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
}

void KDepthOfField::UpdateConstantBuffer(ID3D11DeviceContext* Context)
{
    struct FDoFConstantBuffer
    {
        float FocusDistance;
        float FocusRange;
        float BlurRadius;
        UINT32 MaxBlurSamples;
        BOOL bEnabled;
        float NearBlurStart;
        float NearBlurEnd;
        float FarBlurStart;
        float FarBlurEnd;
        XMFLOAT2 Resolution;
        float Padding;
        float CameraNear;
        float CameraFar;
    };

    FDoFConstantBuffer buffer;
    buffer.FocusDistance = Parameters.FocusDistance;
    buffer.FocusRange = Parameters.FocusRange;
    buffer.BlurRadius = Parameters.BlurRadius;
    buffer.MaxBlurSamples = Parameters.MaxBlurSamples;
    buffer.bEnabled = Parameters.bEnabled;
    buffer.NearBlurStart = Parameters.NearBlurStart;
    buffer.NearBlurEnd = Parameters.NearBlurEnd;
    buffer.FarBlurStart = Parameters.FarBlurStart;
    buffer.FarBlurEnd = Parameters.FarBlurEnd;
    buffer.Resolution = Parameters.Resolution;
    buffer.Padding = 0.0f;
    buffer.CameraNear = CameraNear;
    buffer.CameraFar = CameraFar;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Context->Map(DoFConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &buffer, sizeof(FDoFConstantBuffer));
        Context->Unmap(DoFConstantBuffer.Get(), 0);
    }
}

void KDepthOfField::RenderFullscreenQuad(ID3D11DeviceContext* Context)
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

float KDepthOfField::CalculateCoc(float Depth)
{
    float linearDepth = Depth;
    
    if (linearDepth < Parameters.FocusDistance - Parameters.FocusRange * 0.5f)
    {
        return (linearDepth - Parameters.NearBlurStart) / (Parameters.NearBlurEnd - Parameters.NearBlurStart);
    }
    else if (linearDepth > Parameters.FocusDistance + Parameters.FocusRange * 0.5f)
    {
        return (linearDepth - Parameters.FarBlurStart) / (Parameters.FarBlurEnd - Parameters.FarBlurStart);
    }
    
    return 0.0f;
}
