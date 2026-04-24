#include "GBuffer.h"

HRESULT KGBuffer::Initialize(ID3D11Device* Device, UINT32 InWidth, UINT32 InHeight)
{
    if (!Device)
    {
        LOG_ERROR("GBuffer: Invalid device");
        return E_INVALIDARG;
    }

    if (InWidth == 0 || InHeight == 0)
    {
        LOG_ERROR("GBuffer: Invalid dimensions");
        return E_INVALIDARG;
    }

    Width = InWidth;
    Height = InHeight;

    HRESULT hr = CreateGBufferTextures(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create G-Buffer textures");
        return hr;
    }

    hr = CreateDepthTexture(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create depth texture");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("GBuffer: Initialized successfully");
    return S_OK;
}

void KGBuffer::Cleanup()
{
    ReleaseResources();
    bInitialized = false;
    LOG_INFO("GBuffer: Cleaned up");
}

void KGBuffer::ReleaseResources()
{
    for (UINT32 i = 0; i < GBUFFER_RT_COUNT; ++i)
    {
        GBufferTextures[i].Texture.Reset();
        GBufferTextures[i].RTV.Reset();
        GBufferTextures[i].SRV.Reset();
    }
    DepthTexture.Reset();
    DepthStencilView.Reset();
    DepthSRV.Reset();
}

HRESULT KGBuffer::CreateGBufferTextures(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = Width;
    TexDesc.Height = Height;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = 1;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.SampleDesc.Quality = 0;
    TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;

    TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TexDesc.CPUAccessFlags = 0;
    TexDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&TexDesc, nullptr, &GBufferTextures[0].Texture);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create AlbedoMetallic texture");
        return hr;
    }

    TexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateTexture2D(&TexDesc, nullptr, &GBufferTextures[1].Texture);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create NormalRoughness texture");
        return hr;
    }

    TexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateTexture2D(&TexDesc, nullptr, &GBufferTextures[2].Texture);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create PositionAO texture");
        return hr;
    }

    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Texture2D.MipSlice = 0;

    RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = Device->CreateRenderTargetView(GBufferTextures[0].Texture.Get(), &RTVDesc, &GBufferTextures[0].RTV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create AlbedoMetallic RTV");
        return hr;
    }

    RTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateRenderTargetView(GBufferTextures[1].Texture.Get(), &RTVDesc, &GBufferTextures[1].RTV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create NormalRoughness RTV");
        return hr;
    }

    RTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateRenderTargetView(GBufferTextures[2].Texture.Get(), &RTVDesc, &GBufferTextures[2].RTV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create PositionAO RTV");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;

    SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = Device->CreateShaderResourceView(GBufferTextures[0].Texture.Get(), &SRVDesc, &GBufferTextures[0].SRV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create AlbedoMetallic SRV");
        return hr;
    }

    SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = Device->CreateShaderResourceView(GBufferTextures[1].Texture.Get(), &SRVDesc, &GBufferTextures[1].SRV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create NormalRoughness SRV");
        return hr;
    }

    hr = Device->CreateShaderResourceView(GBufferTextures[2].Texture.Get(), &SRVDesc, &GBufferTextures[2].SRV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create PositionAO SRV");
        return hr;
    }

    return S_OK;
}

HRESULT KGBuffer::CreateDepthTexture(ID3D11Device* Device)
{
    D3D11_TEXTURE2D_DESC DepthDesc = {};
    DepthDesc.Width = Width;
    DepthDesc.Height = Height;
    DepthDesc.MipLevels = 1;
    DepthDesc.ArraySize = 1;
    DepthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    DepthDesc.SampleDesc.Count = 1;
    DepthDesc.SampleDesc.Quality = 0;
    DepthDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    DepthDesc.CPUAccessFlags = 0;
    DepthDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create depth texture");
        return hr;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    DSVDesc.Texture2D.MipSlice = 0;

    hr = Device->CreateDepthStencilView(DepthTexture.Get(), &DSVDesc, &DepthStencilView);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create depth stencil view");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC DepthSRVDesc = {};
    DepthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    DepthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DepthSRVDesc.Texture2D.MostDetailedMip = 0;
    DepthSRVDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(DepthTexture.Get(), &DepthSRVDesc, &DepthSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("GBuffer: Failed to create depth SRV");
        return hr;
    }

    return S_OK;
}

void KGBuffer::Bind(ID3D11DeviceContext* Context, ID3D11DepthStencilView* InDepthStencilView)
{
    ID3D11RenderTargetView* RTVs[GBUFFER_RT_COUNT];
    for (UINT32 i = 0; i < GBUFFER_RT_COUNT; ++i)
    {
        RTVs[i] = GBufferTextures[i].RTV.Get();
    }

    ID3D11DepthStencilView* DSV = InDepthStencilView ? InDepthStencilView : DepthStencilView.Get();
    Context->OMSetRenderTargets(GBUFFER_RT_COUNT, RTVs, DSV);
}

void KGBuffer::Unbind(ID3D11DeviceContext* Context, ID3D11RenderTargetView* BackBufferRTV)
{
    ID3D11RenderTargetView* NullRTVs[GBUFFER_RT_COUNT] = { nullptr, nullptr, nullptr };
    Context->OMSetRenderTargets(GBUFFER_RT_COUNT, NullRTVs, nullptr);
    Context->OMSetRenderTargets(1, &BackBufferRTV, nullptr);
}

void KGBuffer::Clear(ID3D11DeviceContext* Context, const float ClearColor[4])
{
    for (UINT32 i = 0; i < GBUFFER_RT_COUNT; ++i)
    {
        Context->ClearRenderTargetView(GBufferTextures[i].RTV.Get(), ClearColor);
    }

    Context->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

ID3D11RenderTargetView* const* KGBuffer::GetRenderTargetViews() const
{
    thread_local ID3D11RenderTargetView* RTVs[GBUFFER_RT_COUNT];
    for (UINT32 i = 0; i < GBUFFER_RT_COUNT; ++i)
    {
        RTVs[i] = GBufferTextures[i].RTV.Get();
    }
    return RTVs;
}

HRESULT KGBuffer::Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
{
    if (NewWidth == 0 || NewHeight == 0)
    {
        LOG_ERROR("GBuffer: Invalid resize dimensions");
        return E_INVALIDARG;
    }

    ReleaseResources();

    Width = NewWidth;
    Height = NewHeight;

    HRESULT hr = CreateGBufferTextures(Device);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CreateDepthTexture(Device);
    if (FAILED(hr))
    {
        return hr;
    }

    LOG_INFO("GBuffer: Resized successfully");
    return S_OK;
}
