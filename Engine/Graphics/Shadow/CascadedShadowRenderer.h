#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "CascadedShadowMap.h"
#include "ShadowRenderer.h"
#include "../Mesh.h"
#include "../Shader.h"
#include "../Light.h"
#include "../Camera.h"
#include <vector>
#include <array>

struct FCascadedShadowBuffer
{
    FCascadeData CascadeData;
    XMFLOAT4 ShadowMapSize;
    float DepthBias;
    UINT32 PCFKernelSize;
    UINT32 CascadeCount;
    float Padding;
};
static_assert(sizeof(FCascadedShadowBuffer) % 16 == 0, "FCascadedShadowBuffer must be 16-byte aligned");

struct FCascadeSplit
{
    float NearDistance;
    float FarDistance;
    XMMATRIX LightViewProjection;
    XMFLOAT3 MinBounds;
    XMFLOAT3 MaxBounds;
};

class KCascadedShadowRenderer
{
public:
    KCascadedShadowRenderer() = default;
    ~KCascadedShadowRenderer() = default;

    KCascadedShadowRenderer(const KCascadedShadowRenderer&) = delete;
    KCascadedShadowRenderer& operator=(const KCascadedShadowRenderer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, const FCascadedShadowConfig& Config);
    void Cleanup();

    void BeginShadowPass(ID3D11DeviceContext* Context, const FDirectionalLight& Light, KCamera* Camera);
    void BeginCascadePass(ID3D11DeviceContext* Context, UINT32 CascadeIndex);
    void RenderShadowCasters(ID3D11DeviceContext* Context);
    void EndCascadePass(ID3D11DeviceContext* Context);
    void EndShadowPass(ID3D11DeviceContext* Context);

    void AddShadowCaster(const FShadowCaster& Caster);
    void ClearShadowCasters();

    void BindShadowMaps(ID3D11DeviceContext* Context, UINT32 Slot = 3);
    void UnbindShadowMaps(ID3D11DeviceContext* Context, UINT32 Slot = 3);

    KCascadedShadowMap* GetCascadedShadowMap() { return &CascadedShadowMap; }
    const FCascadeSplit* GetCascadeSplits() const { return CascadeSplits.data(); }
    UINT32 GetCascadeCount() const { return CascadedShadowMap.GetCascadeCount(); }
    bool IsInitialized() const { return bInitialized; }
    
    void SetVisualizeCascades(bool bVisualize) { Config.bVisualizeCascades = bVisualize; }
    bool IsVisualizingCascades() const { return Config.bVisualizeCascades; }

private:
    HRESULT CreateShadowShader(ID3D11Device* Device);
    HRESULT CreateShadowBuffer(ID3D11Device* Device);
    void CalculateCascadeSplits(const FDirectionalLight& Light, KCamera* Camera);
    void UpdateCascadeBuffer(ID3D11DeviceContext* Context);
    XMMATRIX CalculateLightViewProjection(const FDirectionalLight& Light, 
                                          const XMFLOAT3& MinBounds, 
                                          const XMFLOAT3& MaxBounds);

private:
    KCascadedShadowMap CascadedShadowMap;
    FCascadedShadowConfig Config;
    std::shared_ptr<KShaderProgram> ShadowShader;
    ComPtr<ID3D11Buffer> ShadowConstantBuffer;
    std::vector<FShadowCaster> ShadowCasters;
    std::array<FCascadeSplit, MAX_CASCADE_COUNT> CascadeSplits;
    bool bInitialized = false;
};
