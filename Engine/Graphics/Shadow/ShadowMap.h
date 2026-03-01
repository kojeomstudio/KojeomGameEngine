#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"

struct FShadowConfig
{
    UINT32 Resolution = 2048;
    float DepthBias = 0.0001f;
    float SlopeScaledDepthBias = 2.0f;
    UINT32 PCFKernelSize = 3;
    bool bEnabled = true;
};

class KShadowMap
{
public:
    KShadowMap() = default;
    ~KShadowMap() = default;

    KShadowMap(const KShadowMap&) = delete;
    KShadowMap& operator=(const KShadowMap&) = delete;

    HRESULT Initialize(ID3D11Device* Device, const FShadowConfig& Config);
    void Cleanup();

    void BeginShadowPass(ID3D11DeviceContext* Context);
    void EndShadowPass(ID3D11DeviceContext* Context);

    void ClearDepth(ID3D11DeviceContext* Context);

    ID3D11ShaderResourceView* GetDepthSRV() const { return DepthSRV.Get(); }
    ID3D11DepthStencilView* GetDepthDSV() const { return DepthDSV.Get(); }

    UINT32 GetResolution() const { return Config.Resolution; }
    const FShadowConfig& GetConfig() const { return Config; }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateDepthTexture(ID3D11Device* Device);

private:
    FShadowConfig Config;
    ComPtr<ID3D11Texture2D> DepthTexture;
    ComPtr<ID3D11ShaderResourceView> DepthSRV;
    ComPtr<ID3D11DepthStencilView> DepthDSV;
    ComPtr<ID3D11DepthStencilState> ShadowDepthState;
    bool bInitialized = false;
};
