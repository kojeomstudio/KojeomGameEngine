#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include "GraphicsDevice.h"
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Light.h"
#include "Shadow/ShadowRenderer.h"
#include "Deferred/DeferredRenderer.h"

enum class ERenderPath
{
    Forward,
    Deferred
};

struct FRenderObject
{
    std::shared_ptr<KMesh> Mesh;
    std::shared_ptr<KShaderProgram> Shader;
    std::shared_ptr<KTexture> Texture;
    XMMATRIX WorldMatrix = XMMatrixIdentity();
    
    FRenderObject(std::shared_ptr<KMesh> InMesh, std::shared_ptr<KShaderProgram> InShader, std::shared_ptr<KTexture> InTexture = nullptr)
        : Mesh(InMesh), Shader(InShader), Texture(InTexture) {}
};

class KRenderer
{
public:
    KRenderer() = default;
    ~KRenderer() = default;

    KRenderer(const KRenderer&) = delete;
    KRenderer& operator=(const KRenderer&) = delete;

    HRESULT Initialize(KGraphicsDevice* InGraphicsDevice);

    void BeginFrame(KCamera* InCamera, const float ClearColor[4] = Colors::CornflowerBlue);
    void EndFrame(bool bVSync = true);

    void RenderObject(const FRenderObject& RenderObject);

    void RenderMesh(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix, 
                   std::shared_ptr<KTexture> InTexture = nullptr);

    void RenderMeshBasic(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix);

    void RenderMeshLit(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                       std::shared_ptr<KTexture> InTexture = nullptr);

    void SetDirectionalLight(const FDirectionalLight& Light) { DirectionalLight = Light; }

    void AddPointLight(const FPointLight& Light);
    void RemovePointLight(UINT32 Index);
    void ClearPointLights();
    void SetPointLight(UINT32 Index, const FPointLight& Light);
    UINT32 GetNumPointLights() const { return static_cast<UINT32>(PointLights.size()); }
    const FPointLight& GetPointLight(UINT32 Index) const { return PointLights[Index]; }

    void AddSpotLight(const FSpotLight& Light);
    void RemoveSpotLight(UINT32 Index);
    void ClearSpotLights();
    void SetSpotLight(UINT32 Index, const FSpotLight& Light);
    UINT32 GetNumSpotLights() const { return static_cast<UINT32>(SpotLights.size()); }
    const FSpotLight& GetSpotLight(UINT32 Index) const { return SpotLights[Index]; }

    void ClearAllLights();

    void DebugDrawPointLightVolumes(bool bDraw) { bDebugDrawPointLightVolumes = bDraw; }
    void DebugDrawSpotLightVolumes(bool bDraw) { bDebugDrawSpotLightVolumes = bDraw; }
    void SetDebugDrawEnabled(bool bEnabled) { bDebugDrawEnabled = bEnabled; }
    bool IsDebugDrawEnabled() const { return bDebugDrawEnabled; }
    void DebugDrawLightVolumes();

    void SetShadowEnabled(bool bEnabled);
    bool IsShadowEnabled() const { return ShadowRenderer.IsInitialized(); }
    KShadowRenderer* GetShadowRenderer() { return &ShadowRenderer; }
    void SetShadowSceneBounds(const XMFLOAT3& Center, float Radius);

    void SetRenderPath(ERenderPath Path);
    ERenderPath GetRenderPath() const { return CurrentRenderPath; }
    bool IsDeferredEnabled() const { return DeferredRenderer.IsInitialized(); }
    KDeferredRenderer* GetDeferredRenderer() { return &DeferredRenderer; }

    KShaderProgram* GetLightShader() const { return LightShader.get(); }
    void Cleanup();

    KShaderProgram* GetBasicShader() const { return BasicShader.get(); }
    KTextureManager* GetTextureManager() { return &TextureManager; }

    std::shared_ptr<KMesh> CreateTriangleMesh();
    std::shared_ptr<KMesh> CreateQuadMesh();
    std::shared_ptr<KMesh> CreateCubeMesh();
    std::shared_ptr<KMesh> CreateSphereMesh(UINT32 Slices = 16, UINT32 Stacks = 16);

private:
    HRESULT InitializeDefaultResources();
    HRESULT InitializeShadowSystem();
    void UpdateLightBuffer();
    void UpdateShadowBuffer();
    HRESULT CreateDebugResources();
    void RenderShadowPass();

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    KCamera* CurrentCamera = nullptr;

    std::shared_ptr<KShaderProgram> BasicShader;
    std::shared_ptr<KShaderProgram> LightShader;
    std::shared_ptr<KShaderProgram> ShadowLitShader;
    KTextureManager TextureManager;

    FDirectionalLight DirectionalLight;
    std::vector<FPointLight> PointLights;
    std::vector<FSpotLight> SpotLights;
    ComPtr<ID3D11Buffer> LightConstantBuffer;

    KShadowRenderer ShadowRenderer;
    KDeferredRenderer DeferredRenderer;
    ComPtr<ID3D11Buffer> ShadowConstantBuffer;
    ComPtr<ID3D11SamplerState> ShadowSamplerState;
    XMFLOAT3 ShadowSceneCenter = { 0.0f, 0.0f, 0.0f };
    float ShadowSceneRadius = 50.0f;
    bool bShadowsEnabled = false;

    ERenderPath CurrentRenderPath = ERenderPath::Forward;

    ComPtr<ID3D11RasterizerState> WireframeRasterizerState;
    std::shared_ptr<KMesh> DebugSphereMesh;

    bool bInFrame = false;
    bool bDebugDrawEnabled = false;
    bool bDebugDrawPointLightVolumes = false;
    bool bDebugDrawSpotLightVolumes = false;
}; 
