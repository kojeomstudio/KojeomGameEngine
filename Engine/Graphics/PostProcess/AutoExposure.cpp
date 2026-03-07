#include "AutoExposure.h"

HRESULT KAutoExposure::Initialize(ID3D11Device* InDevice, UINT32 InWidth, UINT32 InHeight)
{
    if (!InDevice)
    {
        LOG_ERROR("Invalid device");
        return E_INVALIDARG;
    }

    Device = InDevice;
    Width = InWidth;
    Height = InHeight;

    HRESULT hr = CreateLuminanceResources(InDevice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create luminance resources");
        return hr;
    }

    hr = CreateHistogramResources(InDevice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create histogram resources");
        return hr;
    }

    hr = CreateShaders(InDevice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create auto exposure shaders");
        return hr;
    }

    hr = CreateConstantBuffers(InDevice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create constant buffers");
        return hr;
    }

    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = InDevice->CreateSamplerState(&samplerDesc, &PointSamplerState);
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
    }

    bInitialized = true;
    LOG_INFO("AutoExposure initialized successfully");
    return S_OK;
}

void KAutoExposure::Cleanup()
{
    LuminanceTexture1.Reset();
    LuminanceRTV1.Reset();
    LuminanceSRV1.Reset();

    LuminanceTexture2.Reset();
    LuminanceRTV2.Reset();
    LuminanceSRV2.Reset();

    LuminanceTexture4.Reset();
    LuminanceRTV4.Reset();
    LuminanceSRV4.Reset();

    HistogramTexture.Reset();
    HistogramUAV.Reset();
    HistogramSRV.Reset();

    HistogramReadbackBuffer.Reset();

    AutoExposureConstantBuffer.Reset();
    ExposureConstantBuffer.Reset();

    LuminanceShader.reset();
    DownsampleShader.reset();
    HistogramShader.reset();

    PointSamplerState.Reset();
    LinearSamplerState.Reset();

    bInitialized = false;
}

HRESULT KAutoExposure::Resize(ID3D11Device* InDevice, UINT32 NewWidth, UINT32 NewHeight)
{
    if (!InDevice || NewWidth == 0 || NewHeight == 0)
    {
        return E_INVALIDARG;
    }

    Width = NewWidth;
    Height = NewHeight;

    HRESULT hr = CreateLuminanceResources(InDevice);
    if (FAILED(hr)) return hr;

    hr = CreateHistogramResources(InDevice);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KAutoExposure::CreateLuminanceResources(ID3D11Device* InDevice)
{
    auto CreateLuminanceTexture = [InDevice](UINT32 size, ComPtr<ID3D11Texture2D>& Texture,
        ComPtr<ID3D11RenderTargetView>& RTV, ComPtr<ID3D11ShaderResourceView>& SRV) -> HRESULT
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = size;
        texDesc.Height = size;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &Texture);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateRenderTargetView(Texture.Get(), nullptr, &RTV);
        if (FAILED(hr)) return hr;

        hr = InDevice->CreateShaderResourceView(Texture.Get(), nullptr, &SRV);
        if (FAILED(hr)) return hr;

        return S_OK;
    };

    HRESULT hr = CreateLuminanceTexture(LuminanceSize, LuminanceTexture1, LuminanceRTV1, LuminanceSRV1);
    if (FAILED(hr)) return hr;

    hr = CreateLuminanceTexture(LuminanceSize / 2, LuminanceTexture2, LuminanceRTV2, LuminanceSRV2);
    if (FAILED(hr)) return hr;

    hr = CreateLuminanceTexture(LuminanceSize / 4, LuminanceTexture4, LuminanceRTV4, LuminanceSRV4);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KAutoExposure::CreateHistogramResources(ID3D11Device* InDevice)
{
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = HistogramSize;
        texDesc.Height = 1;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_UINT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = InDevice->CreateTexture2D(&texDesc, nullptr, &HistogramTexture);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create histogram texture");
            return hr;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_UINT;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;

        hr = InDevice->CreateUnorderedAccessView(HistogramTexture.Get(), &uavDesc, &HistogramUAV);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create histogram UAV");
            return hr;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_UINT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = InDevice->CreateShaderResourceView(HistogramTexture.Get(), &srvDesc, &HistogramSRV);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create histogram SRV");
            return hr;
        }
    }

    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = HistogramSize * sizeof(UINT32);
        bufferDesc.Usage = D3D11_USAGE_STAGING;
        bufferDesc.BindFlags = 0;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &HistogramReadbackBuffer);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create histogram readback buffer");
            return hr;
        }
    }

    return S_OK;
}

HRESULT KAutoExposure::CreateShaders(ID3D11Device* InDevice)
{
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

    std::string luminancePS = R"(
Texture2D HDRTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = HDRTexture.Sample(LinearSampler, input.TexCoord).rgb;
    float lum = Luminance(color);
    return float4(lum, 0.0f, 0.0f, 1.0f);
}
)";

    std::string downsamplePS = R"(
Texture2D LuminanceTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 texelSize = 1.0f / float2(128.0f, 128.0f);
    
    float luma = 0.0f;
    luma += LuminanceTexture.Sample(LinearSampler, input.TexCoord + float2(-1.0f, -1.0f) * texelSize).r;
    luma += LuminanceTexture.Sample(LinearSampler, input.TexCoord + float2( 1.0f, -1.0f) * texelSize).r;
    luma += LuminanceTexture.Sample(LinearSampler, input.TexCoord + float2(-1.0f,  1.0f) * texelSize).r;
    luma += LuminanceTexture.Sample(LinearSampler, input.TexCoord + float2( 1.0f,  1.0f) * texelSize).r;
    luma += LuminanceTexture.Sample(LinearSampler, input.TexCoord).r * 4.0f;
    luma /= 8.0f;
    
    return float4(luma, 0.0f, 0.0f, 1.0f);
}
)";

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    LuminanceShader = std::make_shared<KShaderProgram>();
    HRESULT hr = LuminanceShader->CompileFromSource(InDevice, vsSource, luminancePS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = LuminanceShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    DownsampleShader = std::make_shared<KShaderProgram>();
    hr = DownsampleShader->CompileFromSource(InDevice, vsSource, downsamplePS, "main", "main");
    if (FAILED(hr)) return hr;
    hr = DownsampleShader->CreateInputLayout(InDevice, layout, ARRAYSIZE(layout));
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT KAutoExposure::CreateConstantBuffers(ID3D11Device* InDevice)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FAutoExposureParams);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &AutoExposureConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create auto exposure constant buffer");
        return hr;
    }

    bufferDesc.ByteWidth = sizeof(float) * 4;
    hr = InDevice->CreateBuffer(&bufferDesc, nullptr, &ExposureConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create exposure constant buffer");
        return hr;
    }

    return S_OK;
}

float KAutoExposure::ComputeExposure(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* HDRTexture, float DeltaTime)
{
    if (!bInitialized || !Parameters.bEnabled)
    {
        return CurrentExposure;
    }

    DownsampleLuminance(Context, HDRTexture);
    float avgLuminance = ReadAverageLuminance(Context);
    AverageLuminance = avgLuminance;

    float targetExposure = 1.0f;
    if (avgLuminance > 0.0001f)
    {
        targetExposure = Parameters.TargetLuminance / avgLuminance;
    }
    targetExposure = std::max(Parameters.MinExposure, std::min(Parameters.MaxExposure, targetExposure));

    float adaptSpeed = (targetExposure > CurrentExposure) ? 
        Parameters.AdaptationSpeedUp : Parameters.AdaptationSpeedDown;
    
    float t = 1.0f - expf(-adaptSpeed * DeltaTime);
    CurrentExposure = CurrentExposure + (targetExposure - CurrentExposure) * t;

    PreviousAverageLuminance = avgLuminance;

    return CurrentExposure;
}

void KAutoExposure::DownsampleLuminance(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* HDRTexture)
{
    auto SetViewport = [Context](UINT32 width, UINT32 height) {
        D3D11_VIEWPORT vp = {};
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width = static_cast<float>(width);
        vp.Height = static_cast<float>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        Context->RSSetViewports(1, &vp);
    };

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    {
        ID3D11RenderTargetView* rtvs[] = { LuminanceRTV1.Get() };
        Context->OMSetRenderTargets(1, rtvs, nullptr);
        Context->ClearRenderTargetView(LuminanceRTV1.Get(), clearColor);
        SetViewport(LuminanceSize, LuminanceSize);

        LuminanceShader->Bind(Context);
        Context->PSSetShaderResources(0, 1, &HDRTexture);
        Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        Context->Draw(4, 0);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        Context->PSSetShaderResources(0, 1, &nullSRV);
    }

    {
        ID3D11RenderTargetView* rtvs[] = { LuminanceRTV2.Get() };
        Context->OMSetRenderTargets(1, rtvs, nullptr);
        Context->ClearRenderTargetView(LuminanceRTV2.Get(), clearColor);
        SetViewport(LuminanceSize / 2, LuminanceSize / 2);

        DownsampleShader->Bind(Context);
        Context->PSSetShaderResources(0, 1, LuminanceSRV1.GetAddressOf());
        Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        Context->Draw(4, 0);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        Context->PSSetShaderResources(0, 1, &nullSRV);
    }

    {
        ID3D11RenderTargetView* rtvs[] = { LuminanceRTV4.Get() };
        Context->OMSetRenderTargets(1, rtvs, nullptr);
        Context->ClearRenderTargetView(LuminanceRTV4.Get(), clearColor);
        SetViewport(LuminanceSize / 4, LuminanceSize / 4);

        DownsampleShader->Bind(Context);
        Context->PSSetShaderResources(0, 1, LuminanceSRV2.GetAddressOf());
        Context->PSSetSamplers(0, 1, LinearSamplerState.GetAddressOf());

        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        Context->Draw(4, 0);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        Context->PSSetShaderResources(0, 1, &nullSRV);
    }
}

float KAutoExposure::ReadAverageLuminance(ID3D11DeviceContext* Context)
{
    float avgLuminance = 0.18f;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(LuminanceTexture4.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    
    if (SUCCEEDED(hr) && mapped.pData)
    {
        const UINT32 size = LuminanceSize / 4;
        const float* data = static_cast<const float*>(mapped.pData);
        
        float sum = 0.0f;
        UINT32 count = 0;
        
        for (UINT32 y = 0; y < size; ++y)
        {
            for (UINT32 x = 0; x < size; ++x)
            {
                float lum = data[y * (mapped.RowPitch / sizeof(float)) + x];
                if (lum > 0.0001f)
                {
                    sum += logf(lum + 0.0001f);
                    count++;
                }
            }
        }

        if (count > 0)
        {
            avgLuminance = expf(sum / count);
        }

        Context->Unmap(LuminanceTexture4.Get(), 0);
    }
    else
    {
        ID3D11Texture2D* stagingTexture = nullptr;
        D3D11_TEXTURE2D_DESC srcDesc;
        LuminanceTexture4->GetDesc(&srcDesc);

        D3D11_TEXTURE2D_DESC stagingDesc = srcDesc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        if (SUCCEEDED(Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture)))
        {
            Context->CopyResource(stagingTexture, LuminanceTexture4.Get());
            
            hr = Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr) && mapped.pData)
            {
                const UINT32 size = LuminanceSize / 4;
                const float* data = static_cast<const float*>(mapped.pData);
                
                float sum = 0.0f;
                UINT32 count = 0;
                
                for (UINT32 y = 0; y < size; ++y)
                {
                    for (UINT32 x = 0; x < size; ++x)
                    {
                        float lum = data[y * (mapped.RowPitch / sizeof(float)) + x];
                        if (lum > 0.0001f)
                        {
                            sum += logf(lum + 0.0001f);
                            count++;
                        }
                    }
                }

                if (count > 0)
                {
                    avgLuminance = expf(sum / count);
                }

                Context->Unmap(stagingTexture, 0);
            }
            stagingTexture->Release();
        }
    }

    return avgLuminance;
}
