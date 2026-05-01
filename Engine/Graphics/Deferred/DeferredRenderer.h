#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "GBuffer.h"
#include "../Mesh.h"
#include "../Shader.h"
#include "../Light.h"
#include "../Camera.h"
#include "../Texture.h"
#include <vector>
#include <algorithm>

struct FTransformBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
};
static_assert(sizeof(FTransformBuffer) % 16 == 0, "FTransformBuffer must be 16-byte aligned");

struct FDeferredMeshRenderData
{
    std::shared_ptr<KMesh> Mesh;
    XMMATRIX WorldMatrix;
    std::shared_ptr<KTexture> DiffuseTexture;
    XMFLOAT4 BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float AO = 1.0f;
};

struct FForwardTransparentRenderData
{
    std::shared_ptr<KMesh> Mesh;
    XMMATRIX WorldMatrix;
    std::shared_ptr<KTexture> DiffuseTexture;
    XMFLOAT4 BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float Alpha = 1.0f;
};

struct FDeferredPointLightData
{
    XMFLOAT3 Position;
    float Intensity;
    XMFLOAT3 Color;
    float Radius;
};

struct FDeferredSpotLightData
{
    XMFLOAT3 Position;
    float Intensity;
    XMFLOAT3 Direction;
    float OuterCone;
    XMFLOAT3 Color;
    float InnerCone;
};

struct FDeferredLightBuffer
{
    XMFLOAT3 CameraPosition;
    float Padding0;
    XMFLOAT3 DirLightDirection;
    float Padding1;
    XMFLOAT3 DirLightColor;
    float DirLightIntensity;
    UINT32 NumPointLights;
    UINT32 NumSpotLights;
    float Padding2[2];
    FDeferredPointLightData PointLights[8];
    FDeferredSpotLightData SpotLights[4];
};
static_assert(sizeof(FDeferredLightBuffer) % 16 == 0, "FDeferredLightBuffer must be 16-byte aligned");

struct FDeferredMaterialBuffer
{
    XMFLOAT4 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float Padding;
};
static_assert(sizeof(FDeferredMaterialBuffer) % 16 == 0, "FDeferredMaterialBuffer must be 16-byte aligned");

class KDeferredRenderer
{
public:
    KDeferredRenderer() = default;
    ~KDeferredRenderer() = default;

    KDeferredRenderer(const KDeferredRenderer&) = delete;
    KDeferredRenderer& operator=(const KDeferredRenderer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    void BeginGeometryPass(ID3D11DeviceContext* Context, KCamera* Camera);
    void RenderGeometry(ID3D11DeviceContext* Context);
    void EndGeometryPass(ID3D11DeviceContext* Context);

    void RenderLightingPass(ID3D11DeviceContext* Context, KCamera* Camera);

    void AddRenderData(const FDeferredMeshRenderData& Data);
    void ClearRenderData();

    void AddTransparentRenderData(const FForwardTransparentRenderData& Data);
    void ClearTransparentRenderData();
    void RenderForwardTransparentPass(ID3D11DeviceContext* Context, KCamera* Camera, ID3D11RenderTargetView* BackBufferRTV);

    void SetDirectionalLight(const FDirectionalLight& Light) { DirectionalLight = Light; }
    void AddPointLight(const FPointLight& Light);
    void ClearPointLights();
    void AddSpotLight(const FSpotLight& Light);
    void ClearSpotLights();
    void ClearAllLights();

    KGBuffer* GetGBuffer() { return &GBuffer; }
    ID3D11ShaderResourceView* GetLightingOutputSRV() const { return LightingOutputSRV.Get(); }
    ID3D11RenderTargetView* GetLightingOutputRTV() const { return LightingOutputRTV.Get(); }
    bool IsInitialized() const { return bInitialized; }

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

private:
    HRESULT CreateGeometryPassShader(ID3D11Device* Device);
    HRESULT CreateLightingPassShader(ID3D11Device* Device);
    HRESULT CreateForwardTransparentShader(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateBlendStates(ID3D11Device* Device);

    void UpdateTransformBuffer(ID3D11DeviceContext* Context, const XMMATRIX& World, const XMMATRIX& View, const XMMATRIX& Projection);
    void UpdateLightBuffer(ID3D11DeviceContext* Context, KCamera* Camera);
    void UpdateMaterialBuffer(ID3D11DeviceContext* Context, const XMFLOAT4& InBaseColor, float InMetallic, float InRoughness, float InAO);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context);

private:
    KGBuffer GBuffer;

    ComPtr<ID3D11Texture2D> LightingOutputTexture;
    ComPtr<ID3D11RenderTargetView> LightingOutputRTV;
    ComPtr<ID3D11ShaderResourceView> LightingOutputSRV;

    std::shared_ptr<KShaderProgram> GeometryPassShader;
    std::shared_ptr<KShaderProgram> LightingPassShader;
    std::shared_ptr<KShaderProgram> ForwardTransparentShader;

    ComPtr<ID3D11Buffer> TransformConstantBuffer;
    ComPtr<ID3D11Buffer> LightConstantBuffer;
    ComPtr<ID3D11Buffer> MaterialConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;
    ComPtr<ID3D11BlendState> TransparentBlendState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    std::vector<FDeferredMeshRenderData> RenderDataList;
    std::vector<FForwardTransparentRenderData> TransparentRenderDataList;

    FDirectionalLight DirectionalLight;
    std::vector<FPointLight> PointLights;
    std::vector<FSpotLight> SpotLights;

    KCamera* CurrentCamera = nullptr;
    bool bInitialized = false;
};
