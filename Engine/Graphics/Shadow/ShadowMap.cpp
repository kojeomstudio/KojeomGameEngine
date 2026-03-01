#include "ShadowMap.h"

HRESULT KShadowMap::Initialize(ID3D11Device* Device, const FShadowConfig& InConfig)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for shadow map initialization");
        return E_INVALIDARG;
    }

    Config = InConfig;

    HRESULT hr = CreateDepthTexture(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow map depth texture");
        return hr;
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthDesc.StencilEnable = FALSE;

    hr = Device->CreateDepthStencilState(&depthDesc, &ShadowDepthState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow depth stencil state");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Shadow map initialized with resolution " + std::to_string(Config.Resolution));
    return S_OK;
}

HRESULT KShadowMap::CreateDepthTexture(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = Config.Resolution;
    texDesc.Height = Config.Resolution;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &DepthTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow map texture");
        return hr;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = Device->CreateDepthStencilView(DepthTexture.Get(), &dsvDesc, &DepthDSV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow map DSV");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(DepthTexture.Get(), &srvDesc, &DepthSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow map SRV");
        return hr;
    }

    return S_OK;
}

void KShadowMap::BeginShadowPass(ID3D11DeviceContext* Context)
{
    if (!Context || !bInitialized) return;

    Context->OMSetRenderTargets(0, nullptr, DepthDSV.Get());
    Context->OMSetDepthStencilState(ShadowDepthState.Get(), 0);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(Config.Resolution);
    viewport.Height = static_cast<float>(Config.Resolution);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    Context->RSSetViewports(1, &viewport);
}

void KShadowMap::EndShadowPass(ID3D11DeviceContext* Context)
{
    if (!Context) return;
    Context->OMSetRenderTargets(0, nullptr, nullptr);
}

void KShadowMap::ClearDepth(ID3D11DeviceContext* Context)
{
    if (!Context || !bInitialized) return;
    Context->ClearDepthStencilView(DepthDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void KShadowMap::Cleanup()
{
    DepthTexture.Reset();
    DepthSRV.Reset();
    DepthDSV.Reset();
    ShadowDepthState.Reset();
    bInitialized = false;
}
