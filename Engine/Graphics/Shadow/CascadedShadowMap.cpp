#include "CascadedShadowMap.h"
#include <cmath>

HRESULT KCascadedShadowMap::Initialize(ID3D11Device* Device, const FCascadedShadowConfig& InConfig)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for cascaded shadow map");
        return E_INVALIDARG;
    }

    Config = InConfig;
    Config.CascadeCount = std::min(Config.CascadeCount, MAX_CASCADE_COUNT);

    HRESULT hr = CreateCascadeTextures(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create cascade textures");
        return hr;
    }

    hr = CreateCascadeArray(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create cascade array");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Cascaded shadow map initialized successfully");
    return S_OK;
}

void KCascadedShadowMap::Cleanup()
{
    for (UINT32 i = 0; i < Config.CascadeCount; ++i)
    {
        CascadeTextures[i].Reset();
        CascadeDSVs[i].Reset();
        CascadeDepthStates[i].Reset();
    }
    CascadeArrayTexture.Reset();
    CascadeArraySRV.Reset();
    bInitialized = false;
}

HRESULT KCascadedShadowMap::CreateCascadeTextures(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Config.CascadeResolution;
    texDesc.Height = Config.CascadeResolution;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
    depthStateDesc.DepthEnable = TRUE;
    depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStateDesc.StencilEnable = FALSE;

    for (UINT32 i = 0; i < Config.CascadeCount; ++i)
    {
        HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &CascadeTextures[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create cascade texture");
            return hr;
        }

        hr = Device->CreateDepthStencilView(CascadeTextures[i].Get(), &dsvDesc, &CascadeDSVs[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create cascade DSV");
            return hr;
        }

        hr = Device->CreateDepthStencilState(&depthStateDesc, &CascadeDepthStates[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create cascade depth state");
            return hr;
        }
    }

    return S_OK;
}

HRESULT KCascadedShadowMap::CreateCascadeArray(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC arrayDesc = {};
    arrayDesc.Width = Config.CascadeResolution;
    arrayDesc.Height = Config.CascadeResolution;
    arrayDesc.MipLevels = 1;
    arrayDesc.ArraySize = Config.CascadeCount;
    arrayDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    arrayDesc.SampleDesc.Count = 1;
    arrayDesc.SampleDesc.Quality = 0;
    arrayDesc.Usage = D3D11_USAGE_DEFAULT;
    arrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    arrayDesc.CPUAccessFlags = 0;
    arrayDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&arrayDesc, nullptr, &CascadeArrayTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create cascade array texture");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = Config.CascadeCount;

    hr = Device->CreateShaderResourceView(CascadeArrayTexture.Get(), &srvDesc, &CascadeArraySRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create cascade array SRV");
        return hr;
    }

    return S_OK;
}

void KCascadedShadowMap::BeginCascadePass(ID3D11DeviceContext* Context, UINT32 CascadeIndex)
{
    if (!Context || CascadeIndex >= Config.CascadeCount)
        return;

    Context->ClearDepthStencilView(CascadeDSVs[CascadeIndex].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    Context->OMSetDepthStencilState(CascadeDepthStates[CascadeIndex].Get(), 0);
    
    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &nullRTV, CascadeDSVs[CascadeIndex].Get());

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(Config.CascadeResolution);
    vp.Height = static_cast<float>(Config.CascadeResolution);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    Context->RSSetViewports(1, &vp);
}

void KCascadedShadowMap::EndCascadePass(ID3D11DeviceContext* Context)
{
    if (!Context)
        return;

    ID3D11RenderTargetView* nullRTV = nullptr;
    ID3D11DepthStencilView* nullDSV = nullptr;
    Context->OMSetRenderTargets(1, &nullRTV, nullDSV);
}

void KCascadedShadowMap::ClearAllDepths(ID3D11DeviceContext* Context)
{
    for (UINT32 i = 0; i < Config.CascadeCount; ++i)
    {
        Context->ClearDepthStencilView(CascadeDSVs[i].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

ID3D11DepthStencilView* KCascadedShadowMap::GetCascadeDSV(UINT32 Index) const
{
    if (Index >= Config.CascadeCount)
        return nullptr;
    return CascadeDSVs[Index].Get();
}

void KCascadedShadowMap::UpdateSplitDistances(float NearPlane, float FarPlane)
{
    const float range = FarPlane - NearPlane;
    
    for (UINT32 i = 0; i <= Config.CascadeCount; ++i)
    {
        float p = static_cast<float>(i) / static_cast<float>(Config.CascadeCount);
        float logSplit = NearPlane * std::pow(FarPlane / NearPlane, p);
        float uniformSplit = NearPlane + range * p;
        Config.SplitDistances[i] = Config.SplitLambda * logSplit + (1.0f - Config.SplitLambda) * uniformSplit;
    }
}
