#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "ShadowMap.h"
#include <array>

constexpr UINT32 MAX_CASCADE_COUNT = 4;

struct FCascadedShadowConfig
{
    UINT32 CascadeCount = 4;
    UINT32 CascadeResolution = 1024;
    float SplitLambda = 0.5f;
    std::array<float, MAX_CASCADE_COUNT + 1> SplitDistances = { 0.0f, 0.05f, 0.15f, 0.5f, 1.0f };
    float DepthBias = 0.0001f;
    float SlopeScaledDepthBias = 2.0f;
    UINT32 PCFKernelSize = 3;
    bool bEnabled = true;
    bool bVisualizeCascades = false;
};

struct FCascadeData
{
    XMMATRIX LightViewProjection[MAX_CASCADE_COUNT];
    XMFLOAT4 SplitDistances[MAX_CASCADE_COUNT];
    XMFLOAT4 CascadeScale[MAX_CASCADE_COUNT];
    XMFLOAT4 CascadeOffset[MAX_CASCADE_COUNT];
};

class KCascadedShadowMap
{
public:
    KCascadedShadowMap() = default;
    ~KCascadedShadowMap() = default;

    KCascadedShadowMap(const KCascadedShadowMap&) = delete;
    KCascadedShadowMap& operator=(const KCascadedShadowMap&) = delete;

    HRESULT Initialize(ID3D11Device* Device, const FCascadedShadowConfig& Config);
    void Cleanup();

    void BeginCascadePass(ID3D11DeviceContext* Context, UINT32 CascadeIndex);
    void EndCascadePass(ID3D11DeviceContext* Context);
    void ClearAllDepths(ID3D11DeviceContext* Context);
    void CopyCascadesToArray(ID3D11DeviceContext* Context);

    ID3D11ShaderResourceView* GetCascadeArraySRV() const { return CascadeArraySRV.Get(); }
    ID3D11DepthStencilView* GetCascadeDSV(UINT32 Index) const;
    
    UINT32 GetCascadeCount() const { return Config.CascadeCount; }
    UINT32 GetCascadeResolution() const { return Config.CascadeResolution; }
    const FCascadedShadowConfig& GetConfig() const { return Config; }
    
    void UpdateSplitDistances(float NearPlane, float FarPlane);

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateCascadeTextures(ID3D11Device* Device);
    HRESULT CreateCascadeArray(ID3D11Device* Device);

private:
    FCascadedShadowConfig Config;
    std::array<ComPtr<ID3D11Texture2D>, MAX_CASCADE_COUNT> CascadeTextures;
    std::array<ComPtr<ID3D11DepthStencilView>, MAX_CASCADE_COUNT> CascadeDSVs;
    ComPtr<ID3D11Texture2D> CascadeArrayTexture;
    ComPtr<ID3D11ShaderResourceView> CascadeArraySRV;
    ComPtr<ID3D11DepthStencilState> SharedDepthState;
    bool bInitialized = false;
};
