#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "ShadowMap.h"
#include "../Mesh.h"
#include "../Shader.h"
#include "../Light.h"
#include <vector>

struct FShadowBuffer
{
    XMMATRIX LightViewProjection;
    XMFLOAT2 ShadowMapSize;
    float DepthBias;
    UINT32 PCFKernelSize;
};
static_assert(sizeof(FShadowBuffer) % 16 == 0, "FShadowBuffer must be 16-byte aligned");

struct FShadowCaster
{
    std::shared_ptr<KMesh> Mesh;
    XMMATRIX WorldMatrix;
    bool bIsSkeletal = false;
    ID3D11Buffer* SkeletalVertexBuffer = nullptr;
    ID3D11Buffer* SkeletalIndexBuffer = nullptr;
    uint32 SkeletalIndexCount = 0;
    ID3D11Buffer* BoneMatrixBuffer = nullptr;
};

class KShadowRenderer
{
public:
    KShadowRenderer() = default;
    ~KShadowRenderer() = default;

    KShadowRenderer(const KShadowRenderer&) = delete;
    KShadowRenderer& operator=(const KShadowRenderer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, const FShadowConfig& Config);
    void Cleanup();

    void BeginShadowPass(ID3D11DeviceContext* Context, const FDirectionalLight& Light,
                        const XMFLOAT3& SceneCenter, float SceneRadius);
    void RenderShadowCasters(ID3D11DeviceContext* Context);
    void EndShadowPass(ID3D11DeviceContext* Context);

    void AddShadowCaster(const FShadowCaster& Caster);
    void ClearShadowCasters();

    void BindShadowMap(ID3D11DeviceContext* Context, UINT32 Slot = 3);
    void UnbindShadowMap(ID3D11DeviceContext* Context, UINT32 Slot = 3);

    KShadowMap* GetShadowMap() { return &ShadowMap; }
    const XMMATRIX& GetLightViewProjection() const { return LightViewProjection; }
    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateShadowShader(ID3D11Device* Device);
    HRESULT CreateSkinnedShadowShader(ID3D11Device* Device);
    HRESULT CreateShadowBuffer(ID3D11Device* Device);
    void UpdateShadowBuffer(ID3D11DeviceContext* Context);

private:
    KShadowMap ShadowMap;
    std::shared_ptr<KShaderProgram> ShadowShader;
    std::shared_ptr<KShaderProgram> SkinnedShadowShader;
    ComPtr<ID3D11Buffer> ShadowConstantBuffer;
    std::vector<FShadowCaster> ShadowCasters;

    XMMATRIX LightViewProjection;
    bool bInitialized = false;
};
