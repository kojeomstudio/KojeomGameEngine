#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"

constexpr UINT32 GBUFFER_RT_COUNT = 3;

enum class EGBufferTarget
{
    AlbedoMetallic = 0,
    NormalRoughness = 1,
    PositionAO = 2
};

struct FGBufferTextures
{
    ComPtr<ID3D11Texture2D> Texture;
    ComPtr<ID3D11RenderTargetView> RTV;
    ComPtr<ID3D11ShaderResourceView> SRV;
};

class KGBuffer
{
public:
    KGBuffer() = default;
    ~KGBuffer() = default;

    KGBuffer(const KGBuffer&) = delete;
    KGBuffer& operator=(const KGBuffer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    void Bind(ID3D11DeviceContext* Context, ID3D11DepthStencilView* DepthStencilView);
    void Unbind(ID3D11DeviceContext* Context, ID3D11RenderTargetView* BackBufferRTV);

    void Clear(ID3D11DeviceContext* Context, const float ClearColor[4]);

    ID3D11ShaderResourceView* GetAlbedoMetallicSRV() const { return GBufferTextures[0].SRV.Get(); }
    ID3D11ShaderResourceView* GetNormalRoughnessSRV() const { return GBufferTextures[1].SRV.Get(); }
    ID3D11ShaderResourceView* GetPositionAOSRV() const { return GBufferTextures[2].SRV.Get(); }
    ID3D11ShaderResourceView* GetDepthSRV() const { return DepthSRV.Get(); }

    ID3D11RenderTargetView* const* GetRenderTargetViews() const;
    ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView.Get(); }

    UINT32 GetWidth() const { return Width; }
    UINT32 GetHeight() const { return Height; }

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

private:
    HRESULT CreateGBufferTextures(ID3D11Device* Device);
    HRESULT CreateDepthTexture(ID3D11Device* Device);
    void ReleaseResources();

private:
    FGBufferTextures GBufferTextures[GBUFFER_RT_COUNT];

    ComPtr<ID3D11Texture2D> DepthTexture;
    ComPtr<ID3D11DepthStencilView> DepthStencilView;
    ComPtr<ID3D11ShaderResourceView> DepthSRV;

    UINT32 Width = 0;
    UINT32 Height = 0;
    bool bInitialized = false;
};
