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

struct FTransformBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
};

struct FDeferredMeshRenderData
{
    std::shared_ptr<KMesh> Mesh;
    XMMATRIX WorldMatrix;
    std::shared_ptr<KTexture> DiffuseTexture;
    XMFLOAT4 BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float Metallic = 0.0f;
    float Roughness = 0.5f;
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
    FPointLight PointLights[8];
    FSpotLight SpotLights[4];
};

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

    void SetDirectionalLight(const FDirectionalLight& Light) { DirectionalLight = Light; }
    void AddPointLight(const FPointLight& Light);
    void ClearPointLights();
    void AddSpotLight(const FSpotLight& Light);
    void ClearSpotLights();
    void ClearAllLights();

    KGBuffer* GetGBuffer() { return &GBuffer; }
    bool IsInitialized() const { return bInitialized; }

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

private:
    HRESULT CreateGeometryPassShader(ID3D11Device* Device);
    HRESULT CreateLightingPassShader(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);

    void UpdateTransformBuffer(ID3D11DeviceContext* Context, const XMMATRIX& World, const XMMATRIX& View, const XMMATRIX& Projection);
    void UpdateLightBuffer(ID3D11DeviceContext* Context, KCamera* Camera);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context);

private:
    KGBuffer GBuffer;

    std::shared_ptr<KShaderProgram> GeometryPassShader;
    std::shared_ptr<KShaderProgram> LightingPassShader;

    ComPtr<ID3D11Buffer> TransformConstantBuffer;
    ComPtr<ID3D11Buffer> LightConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    std::vector<FDeferredMeshRenderData> RenderDataList;

    FDirectionalLight DirectionalLight;
    std::vector<FPointLight> PointLights;
    std::vector<FSpotLight> SpotLights;

    KCamera* CurrentCamera = nullptr;
    bool bInitialized = false;
};
