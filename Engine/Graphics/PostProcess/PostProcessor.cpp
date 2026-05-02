#include "PostProcessor.h"

HRESULT KPostProcessor::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
{
    if (!InDevice)
    {
        LOG_ERROR("Invalid device");
        return E_INVALIDARG;
    }

    Device = InDevice;
    Width = InWidth;
    Height = InHeight;

    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));
    Parameters.ColorFilter = XMFLOAT3(1.0f, 1.0f, 1.0f);

    HRESULT hr = CreateHDRRenderTarget(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create HDR render target");
        return hr;
    }

    hr = CreateBloomRenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create bloom render targets");
        return hr;
    }

    hr = CreateIntermediateRenderTargets(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create intermediate render targets");
        return hr;
    }

    hr = CreateShaders(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create post-process shaders");
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

    hr = CreateColorGradingResources(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create color grading resources");
        return hr;
    }

    AutoExposure = std::make_unique<KAutoExposure>();
    hr = AutoExposure->Initialize(Device, Width, Height);
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to initialize auto exposure, using fixed exposure");
    }

    bInitialized = true;
    LOG_INFO("PostProcessor initialized successfully");
    return S_OK;
}

void KPostProcessor::Cleanup()
{
    HDRTexture.Reset();
    HDRRenderTargetView.Reset();
    HDRShaderResourceView.Reset();
    DepthStencilView.Reset();

    BloomExtractTexture.Reset();
    BloomExtractRTV.Reset();
    BloomExtractSRV.Reset();

    for (int i = 0; i < 2; ++i)
    {
        BloomBlurTextures[i].Reset();
        BloomBlurRTVs[i].Reset();
        BloomBlurSRVs[i].Reset();
    }

    IntermediateTexture.Reset();
    IntermediateRTV.Reset();
    IntermediateSRV.Reset();

    Intermediate2Texture.Reset();
    Intermediate2RTV.Reset();
    Intermediate2SRV.Reset();

    BrightExtractShader.reset();
    BlurShader.reset();
    BloomCombineShader.reset();
    TonemapperShader.reset();
    FXAAShader.reset();
    ColorGradingShader.reset();

    PostProcessConstantBuffer.Reset();
    BlurConstantBuffer.Reset();
    PointSamplerState.Reset();
    LinearSamplerState.Reset();
    ColorGradingSamplerState.Reset();
    
    ColorGradingLUT.Reset();
    ColorGradingLUTSRV.Reset();
    
    FullscreenQuadMesh.reset();

    if (AutoExposure)
    {
        AutoExposure->Cleanup();
        AutoExposure.reset();
    }

    bInitialized = false;
}

HRESULT KPostProcessor::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;
    Parameters.Resolution = XMFLOAT2(static_cast<float>(Width), static_cast<float>(Height));

    HRESULT hr = CreateHDRRenderTarget(InDevice);
    if (FAILED(hr)) return hr;

    hr = CreateBloomRenderTargets(InDevice);
    if (FAILED(hr)) return hr;

    hr = CreateIntermediateRenderTargets(InDevice);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KPostProcessor::CreateHDRRenderTarget(ID3D11Device* InDevice)
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

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &HDRTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create HDR texture");
        return hr;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    hr = InDevice->CreateRenderTargetView(HDRTexture.Get(), &rtvDesc, &HDRRenderTargetView);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create HDR RTV");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = InDevice->CreateShaderResourceView(HDRTexture.Get(), &srvDesc, &HDRShaderResourceView);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create HDR SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::CreateBloomRenderTargets(ID3D11Device* InDevice)
{
    UINT32 bloomWidth = Width / 2;
    UINT32 bloomHeight = Height / 2;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = bloomWidth;
    texDesc.Height = bloomHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &BloomExtractTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(BloomExtractTexture.Get(), nullptr, &BloomExtractRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(BloomExtractTexture.Get(), nullptr, &BloomExtractSRV);
    if (FAILED(hr)) return hr;

    for (int i = 0; i < 2; ++i)
    {
        hr = InDevice->CreateTexture2D(&texDesc, nullptr, &BloomBlurTextures[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateRenderTargetView(BloomBlurTextures[i].Get(), nullptr, &BloomBlurRTVs[i]);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateShaderResourceView(BloomBlurTextures[i].Get(), nullptr, &BloomBlurSRVs[i]);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::CreateIntermediateRenderTargets(ID3D11Device* InDevice)
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

    HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &IntermediateTexture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(IntermediateTexture.Get(), nullptr, &IntermediateRTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(IntermediateTexture.Get(), nullptr, &IntermediateSRV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateTexture2D(&texDesc, nullptr, &Intermediate2Texture);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateRenderTargetView(Intermediate2Texture.Get(), nullptr, &Intermediate2RTV);
    if (FAILED(hr)) return hr;

    hr = InDevice->CreateShaderResourceView(Intermediate2Texture.Get(), nullptr, &Intermediate2SRV);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KPostProcessor::CreateShaders(ID3D11Device* InDevice)
{
    BrightExtractShader = std::make_shared<KShaderProgram>();
    BlurShader = std::make_shared<KShaderProgram>();
    BloomCombineShader = std::make_shared<KShaderProgram>();
    TonemapperShader = std::make_shared<KShaderProgram>();
    FXAAShader = std::make_shared<KShaderProgram>();

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

    std::string brightExtractPS = R"(
Texture2D SceneTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float Exposure;
    float Gamma;
    float BloomThreshold;
    float BloomIntensity;
    float BloomBlurSigma;
    uint BloomBlurIterations;
    bool bBloomEnabled;
    bool bTonemappingEnabled;
    bool bFXAAEnabled;
    float FXAASpanMax;
    float FXAAReduceMin;
    float FXAAReduceMul;
    float2 Resolution;
    float2 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = SceneTexture.Sample(PointSampler, input.TexCoord).rgb;
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    
    float knee = BloomThreshold * 0.5f;
    float soft = brightness - knee + sqrt(brightness * brightness - knee * knee);
    float contribution = max(soft, 0.0f) / (2.0f * max(BloomThreshold - knee, 0.001f) + brightness);
    
    return float4(color * contribution, 1.0f);
}
)";

    std::string blurPS = R"(
Texture2D InputTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer BlurBuffer : register(b1)
{
    float2 BlurDirection;
    float2 TexelSize;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

static const float Weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 texOffset = TexelSize * BlurDirection;
    float3 result = InputTexture.Sample(LinearSampler, input.TexCoord).rgb * Weights[0];
    
    for (int i = 1; i < 5; ++i)
    {
        result += InputTexture.Sample(LinearSampler, input.TexCoord + texOffset * i).rgb * Weights[i];
        result += InputTexture.Sample(LinearSampler, input.TexCoord - texOffset * i).rgb * Weights[i];
    }
    
    return float4(result, 1.0f);
}
)";

    std::string bloomCombinePS = R"(
Texture2D SceneTexture : register(t0);
Texture2D BloomTexture : register(t1);
SamplerState PointSampler : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float Exposure;
    float Gamma;
    float BloomThreshold;
    float BloomIntensity;
    float BloomBlurSigma;
    uint BloomBlurIterations;
    bool bBloomEnabled;
    bool bTonemappingEnabled;
    bool bFXAAEnabled;
    float FXAASpanMax;
    float FXAAReduceMin;
    float FXAAReduceMul;
    float2 Resolution;
    float2 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = SceneTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    if (bBloomEnabled)
    {
        float3 bloomColor = BloomTexture.Sample(PointSampler, input.TexCoord).rgb;
        sceneColor += bloomColor * BloomIntensity;
    }
    
    return float4(sceneColor, 1.0f);
}
)";

    std::string tonemapPS = R"(
Texture2D SceneTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float Exposure;
    float Gamma;
    float BloomThreshold;
    float BloomIntensity;
    float BloomBlurSigma;
    uint BloomBlurIterations;
    bool bBloomEnabled;
    bool bTonemappingEnabled;
    bool bFXAAEnabled;
    float FXAASpanMax;
    float FXAAReduceMin;
    float FXAAReduceMul;
    float2 Resolution;
    float2 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 ReinhardTonemap(float3 color)
{
    return color / (color + float3(1.0f, 1.0f, 1.0f));
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = SceneTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    if (bTonemappingEnabled)
    {
        color *= Exposure;
        color = ACESFilm(color);
    }
    
    color = pow(color, 1.0f / Gamma);
    
    return float4(color, 1.0f);
}
)";

    std::string fxaaPS = R"(
Texture2D SceneTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float Exposure;
    float Gamma;
    float BloomThreshold;
    float BloomIntensity;
    float BloomBlurSigma;
    uint BloomBlurIterations;
    bool bBloomEnabled;
    bool bTonemappingEnabled;
    bool bFXAAEnabled;
    float FXAASpanMax;
    float FXAAReduceMin;
    float FXAAReduceMul;
    float2 Resolution;
    float2 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    if (!bFXAAEnabled)
    {
        return SceneTexture.Sample(PointSampler, input.TexCoord);
    }

    float2 texelSize = 1.0f / Resolution;
    
    float3 rgbNW = SceneTexture.Sample(PointSampler, input.TexCoord + float2(-1.0f, -1.0f) * texelSize).rgb;
    float3 rgbNE = SceneTexture.Sample(PointSampler, input.TexCoord + float2(1.0f, -1.0f) * texelSize).rgb;
    float3 rgbSW = SceneTexture.Sample(PointSampler, input.TexCoord + float2(-1.0f, 1.0f) * texelSize).rgb;
    float3 rgbSE = SceneTexture.Sample(PointSampler, input.TexCoord + float2(1.0f, 1.0f) * texelSize).rgb;
    float3 rgbM = SceneTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    float lumaNW = dot(rgbNW, float3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, float3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, float3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, float3(0.299, 0.587, 0.114));
    float lumaM = dot(rgbM, float3(0.299, 0.587, 0.114));
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    float lumaRange = lumaMax - lumaMin;
    
    if (lumaRange < max(FXAAReduceMin, lumaMax * FXAAReduceMul))
    {
        return float4(rgbM, 1.0f);
    }
    
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25f * FXAAReduceMul, FXAAReduceMin);
    float rcpDirMin = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(float2(FXAASpanMax, FXAASpanMax), max(float2(-FXAASpanMax, -FXAASpanMax), dir * rcpDirMin)) * texelSize;
    
    float3 rgbA = 0.5f * (SceneTexture.Sample(PointSampler, input.TexCoord + dir * (1.0f/3.0f - 0.5f)).rgb +
                          SceneTexture.Sample(PointSampler, input.TexCoord + dir * (2.0f/3.0f - 0.5f)).rgb);
    float3 rgbB = rgbA * 0.5f + 0.25f * (
        SceneTexture.Sample(PointSampler, input.TexCoord + dir * -0.5f).rgb +
        SceneTexture.Sample(PointSampler, input.TexCoord + dir * 0.5f).rgb);
    
    float lumaB = dot(rgbB, float3(0.299, 0.587, 0.114));
    
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return float4(rgbA, 1.0f);
    }
    else
    {
        return float4(rgbB, 1.0f);
    }
}
)";

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    HRESULT hr = BrightExtractShader->CompileFromSource(InDevice, vsSource, brightExtractPS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = BrightExtractShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    hr = BlurShader->CompileFromSource(InDevice, vsSource, blurPS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = BlurShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    hr = BloomCombineShader->CompileFromSource(InDevice, vsSource, bloomCombinePS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = BloomCombineShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    hr = TonemapperShader->CompileFromSource(InDevice, vsSource, tonemapPS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = TonemapperShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    hr = FXAAShader->CompileFromSource(InDevice, vsSource, fxaaPS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = FXAAShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KPostProcessor::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FPostProcessParams);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &PostProcessConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create post-process constant buffer");
        return hr;
    }

    struct FBlurBuffer
    {
        XMFLOAT2 BlurDirection;
        XMFLOAT2 TexelSize;
    };
    static_assert(sizeof(FBlurBuffer) % 16 == 0, "FBlurBuffer must be 16-byte aligned");

    bufferDesc.ByteWidth = sizeof(FBlurBuffer);
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &BlurConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create blur constant buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::CreateSamplerStates(ID3D11Device* InDevice)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = InDevice->CreateSamplerState(&samplerDesc, &PointSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create point sampler state");
        return hr;
    }

    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    hr = InDevice->CreateSamplerState(&samplerDesc, &LinearSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create linear sampler state");
        return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::CreateFullscreenQuad(ID3D11Device* InDevice)
{
    struct FVertex
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };

    FVertex vertices[] = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    ComPtr<ID3D11Buffer> vertexBuffer;
    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create fullscreen quad vertex buffer");
        return hr;
    }

    FullscreenQuadMesh = std::make_shared<KMesh>();
    hr = FullscreenQuadMesh->InitializeFromBuffer(vertexBuffer.Get(), 4, sizeof(FVertex));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize fullscreen quad mesh");
        return hr;
    }

    return S_OK;
}

void KPostProcessor::BeginHDRPass(ID3D11DeviceContext* Context)
{
    SetRenderTarget(Context, HDRRenderTargetView.Get(), DepthStencilView.Get());
    ClearRenderTarget(Context, HDRRenderTargetView.Get(), DefaultClearColor);
}

void KPostProcessor::ApplyPostProcessing(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget)
{
    ApplyPostProcessing(Context, FinalTarget, 0.016f);
}

void KPostProcessor::ApplyPostProcessing(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget, float DeltaTime)
{
    if (!bInitialized) return;

    if (AutoExposure && AutoExposure->IsEnabled())
    {
        float autoExposure = AutoExposure->ComputeExposure(Context, HDRShaderResourceView.Get(), DeltaTime);
        Parameters.Exposure = autoExposure;
    }

    UpdatePostProcessBuffer(Context);

    if (Parameters.bBloomEnabled)
    {
        ExtractBrightAreas(Context);
        BlurBloomTexture(Context);
        ApplyBloom(Context);
    }

    if (Parameters.bColorGradingEnabled && ColorGradingLUTSRV)
    {
        ApplyColorGrading(Context);
    }

    ApplyTonemapping(Context);
    ApplyFXAA(Context, FinalTarget);
}

void KPostProcessor::ExtractBrightAreas(ID3D11DeviceContext* Context)
{
    SetRenderTarget(Context, BloomExtractRTV.Get());
    ClearRenderTarget(Context, BloomExtractRTV.Get(), DefaultClearColor);

    BrightExtractShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, PostProcessConstantBuffer.GetAddressOf());
    Context->PSSetShaderResources(0, 1, HDRShaderResourceView.GetAddressOf());
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}

void KPostProcessor::BlurBloomTexture(ID3D11DeviceContext* Context)
{
    struct FBlurBuffer
    {
        XMFLOAT2 BlurDirection;
        XMFLOAT2 TexelSize;
    };

    UINT32 bloomWidth = Width / 2;
    UINT32 bloomHeight = Height / 2;
    FBlurBuffer blurData;
    blurData.TexelSize = XMFLOAT2(1.0f / bloomWidth, 1.0f / bloomHeight);

    BlurShader->Bind(Context);
    Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

    for (UINT32 i = 0; i < Parameters.BloomBlurIterations; ++i)
    {
        ID3D11ShaderResourceView* inputSRV = (i == 0) ? BloomExtractSRV.Get() : BloomBlurSRVs[0].Get();
        ID3D11RenderTargetView* outputRTV = BloomBlurRTVs[1].Get();

        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT mapHr = Context->Map(BlurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (SUCCEEDED(mapHr))
            {
                blurData.BlurDirection = XMFLOAT2(1.0f, 0.0f);
                memcpy(mapped.pData, &blurData, sizeof(blurData));
                Context->Unmap(BlurConstantBuffer.Get(), 0);
            }
        }

        SetRenderTarget(Context, outputRTV);
        ClearRenderTarget(Context, outputRTV, DefaultClearColor);
        Context->PSSetConstantBuffers(1, 1, BlurConstantBuffer.GetAddressOf());
        Context->PSSetShaderResources(0, 1, &inputSRV);
        RenderFullscreenQuad(Context);

        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT mapHr = Context->Map(BlurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (SUCCEEDED(mapHr))
            {
                blurData.BlurDirection = XMFLOAT2(0.0f, 1.0f);
                memcpy(mapped.pData, &blurData, sizeof(blurData));
                Context->Unmap(BlurConstantBuffer.Get(), 0);
            }
        }

        SetRenderTarget(Context, BloomBlurRTVs[0].Get());
        ClearRenderTarget(Context, BloomBlurRTVs[0].Get(), DefaultClearColor);
        Context->PSSetShaderResources(0, 1, BloomBlurSRVs[1].GetAddressOf());
        RenderFullscreenQuad(Context);
    }

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}

void KPostProcessor::ApplyBloom(ID3D11DeviceContext* Context)
{
    SetRenderTarget(Context, IntermediateRTV.Get());
    ClearRenderTarget(Context, IntermediateRTV.Get(), DefaultClearColor);

    BloomCombineShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, PostProcessConstantBuffer.GetAddressOf());
    Context->PSSetShaderResources(0, 1, HDRShaderResourceView.GetAddressOf());
    Context->PSSetShaderResources(1, 1, BloomBlurSRVs[0].GetAddressOf());
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
}

void KPostProcessor::ApplyTonemapping(ID3D11DeviceContext* Context)
{
    bool bHasColorGraded = Parameters.bColorGradingEnabled && ColorGradingLUTSRV;
    ID3D11ShaderResourceView* inputSRV = bHasColorGraded ? Intermediate2SRV.Get() :
        (Parameters.bBloomEnabled ? IntermediateSRV.Get() : HDRShaderResourceView.Get());
    ID3D11RenderTargetView* outputRTV = bHasColorGraded ? IntermediateRTV.Get() : Intermediate2RTV.Get();

    SetRenderTarget(Context, outputRTV);

    TonemapperShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, PostProcessConstantBuffer.GetAddressOf());
    Context->PSSetShaderResources(0, 1, &inputSRV);
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}

void KPostProcessor::ApplyFXAA(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget)
{
    SetRenderTarget(Context, FinalTarget);

    bool bHasColorGraded = Parameters.bColorGradingEnabled && ColorGradingLUTSRV;
    ID3D11ShaderResourceView* inputSRV = bHasColorGraded ? IntermediateSRV.Get() : Intermediate2SRV.Get();

    FXAAShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, PostProcessConstantBuffer.GetAddressOf());
    Context->PSSetShaderResources(0, 1, &inputSRV);
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}

void KPostProcessor::SetAutoExposureEnabled(bool bEnabled)
{
    if (AutoExposure)
    {
        AutoExposure->SetEnabled(bEnabled);
    }
}

bool KPostProcessor::IsAutoExposureEnabled() const
{
    return AutoExposure ? AutoExposure->IsEnabled() : false;
}

void KPostProcessor::RenderFullscreenQuad(ID3D11DeviceContext* Context)
{
    if (!FullscreenQuadMesh) return;

    UINT32 stride = sizeof(float) * 5;
    UINT32 offset = 0;
    ID3D11Buffer* vertexBuffer = FullscreenQuadMesh->GetVertexBuffer();
    Context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Context->Draw(4, 0);
}

void KPostProcessor::UpdatePostProcessBuffer(ID3D11DeviceContext* Context)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(PostProcessConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, &Parameters, sizeof(Parameters));
        Context->Unmap(PostProcessConstantBuffer.Get(), 0);
    }
}

void KPostProcessor::SetRenderTarget(ID3D11DeviceContext* Context, ID3D11RenderTargetView* RTV, ID3D11DepthStencilView* DSV)
{
    Context->OMSetRenderTargets(1, &RTV, DSV);
    
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    
    ComPtr<ID3D11Resource> resource;
    if (RTV)
    {
        RTV->GetResource(&resource);
        if (resource)
        {
            D3D11_TEXTURE2D_DESC texDesc;
            ComPtr<ID3D11Texture2D> texture;
            resource.As(&texture);
            texture->GetDesc(&texDesc);
            viewport.Width = static_cast<float>(texDesc.Width);
            viewport.Height = static_cast<float>(texDesc.Height);
        }
    }
    else
    {
        viewport.Width = static_cast<float>(Width);
        viewport.Height = static_cast<float>(Height);
    }
    
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    Context->RSSetViewports(1, &viewport);
}

void KPostProcessor::ClearRenderTarget(ID3D11DeviceContext* Context, ID3D11RenderTargetView* RTV, const float ClearColor[4])
{
    Context->ClearRenderTargetView(RTV, ClearColor);
}

HRESULT KPostProcessor::CreateColorGradingResources(ID3D11Device* InDevice)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = InDevice->CreateSamplerState(&samplerDesc, &ColorGradingSamplerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create color grading sampler state");
        return hr;
    }

    hr = CreateDefaultColorGradingLUT(InDevice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create default color grading LUT");
        return hr;
    }

    ColorGradingShader = std::make_shared<KShaderProgram>();

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

    std::string colorGradingPS = R"(
Texture2D SceneTexture : register(t0);
Texture3D ColorGradingLUT : register(t1);
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

cbuffer PostProcessBuffer : register(b0)
{
    float Exposure;
    float Gamma;
    float BloomThreshold;
    float BloomIntensity;
    float BloomBlurSigma;
    uint BloomBlurIterations;
    bool bBloomEnabled;
    bool bTonemappingEnabled;
    bool bFXAAEnabled;
    float FXAASpanMax;
    float FXAAReduceMin;
    float FXAAReduceMul;
    float2 Resolution;
    bool bColorGradingEnabled;
    float ColorGradingIntensity;
    float3 ColorFilter;
    float Saturation;
    float Contrast;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 ApplyLUT(Texture3D lut, SamplerState lutSampler, float3 color)
{
    float lutSize = 32.0f;
    float scale = (lutSize - 1.0f) / lutSize;
    float offset = 0.5f / lutSize;
    
    float3 lutCoord = color * scale + offset;
    return lut.Sample(lutSampler, lutCoord).rgb;
}

float3 AdjustSaturation(float3 color, float saturation)
{
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    return lerp(float3(luminance, luminance, luminance), color, saturation);
}

float3 AdjustContrast(float3 color, float contrast)
{
    return (color - 0.5f) * contrast + 0.5f;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = SceneTexture.Sample(PointSampler, input.TexCoord).rgb;
    
    color *= ColorFilter;
    
    if (bColorGradingEnabled)
    {
        float3 gradedColor = ApplyLUT(ColorGradingLUT, LinearSampler, saturate(color));
        color = lerp(color, gradedColor, ColorGradingIntensity);
    }
    
    color = AdjustSaturation(color, Saturation);
    color = AdjustContrast(color, Contrast);
    
    return float4(color, 1.0f);
}
)";

    hr = ColorGradingShader->CompileFromSource(InDevice, vsSource, colorGradingPS, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile color grading shader");
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    hr = ColorGradingShader->CreateInputLayout(InDevice, layout, 2);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create color grading input layout");
        return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::CreateDefaultColorGradingLUT(ID3D11Device* InDevice)
{
    constexpr UINT32 LUT_SIZE = 32;
    
    std::vector<float> lutData(LUT_SIZE * LUT_SIZE * LUT_SIZE * 4);
    
    for (UINT32 z = 0; z < LUT_SIZE; ++z)
    {
        for (UINT32 y = 0; y < LUT_SIZE; ++y)
        {
            for (UINT32 x = 0; x < LUT_SIZE; ++x)
            {
                UINT32 index = (z * LUT_SIZE * LUT_SIZE + y * LUT_SIZE + x) * 4;
                lutData[index + 0] = static_cast<float>(x) / (LUT_SIZE - 1);
                lutData[index + 1] = static_cast<float>(y) / (LUT_SIZE - 1);
                lutData[index + 2] = static_cast<float>(z) / (LUT_SIZE - 1);
                lutData[index + 3] = 1.0f;
            }
        }
    }

    D3D11_TEXTURE3D_DESC texDesc = {};
    texDesc.Width = LUT_SIZE;
    texDesc.Height = LUT_SIZE;
    texDesc.Depth = LUT_SIZE;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = lutData.data();
    initData.SysMemPitch = LUT_SIZE * sizeof(float) * 4;
    initData.SysMemSlicePitch = LUT_SIZE * LUT_SIZE * sizeof(float) * 4;

    HRESULT hr = InDevice->CreateTexture3D(&texDesc, &initData, &ColorGradingLUT);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create color grading LUT texture");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MostDetailedMip = 0;
    srvDesc.Texture3D.MipLevels = 1;

    hr = InDevice->CreateShaderResourceView(ColorGradingLUT.Get(), &srvDesc, &ColorGradingLUTSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create color grading LUT SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KPostProcessor::LoadColorGradingLUT(ID3D11Device* InDevice, const std::wstring& Path)
{
    (void)Path;
    (void)InDevice;
    LOG_WARNING("LoadColorGradingLUT: Custom LUT loading not implemented, using default LUT");
    return CreateDefaultColorGradingLUT(InDevice);
}

void KPostProcessor::ApplyColorGrading(ID3D11DeviceContext* Context)
{
    if (!ColorGradingShader || !ColorGradingLUTSRV)
        return;

    SetRenderTarget(Context, Intermediate2RTV.Get());

    ColorGradingShader->Bind(Context);
    Context->PSSetConstantBuffers(0, 1, PostProcessConstantBuffer.GetAddressOf());

    ID3D11ShaderResourceView* inputSRV = Parameters.bBloomEnabled ? IntermediateSRV.Get() : HDRShaderResourceView.Get();
    Context->PSSetShaderResources(0, 1, &inputSRV);
    Context->PSSetShaderResources(1, 1, ColorGradingLUTSRV.GetAddressOf());
    Context->PSSetSamplers(0, 1, PointSamplerState.GetAddressOf());
    Context->PSSetSamplers(1, 1, ColorGradingSamplerState.GetAddressOf());

    RenderFullscreenQuad(Context);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Context->PSSetShaderResources(0, 2, nullSRVs);
}
