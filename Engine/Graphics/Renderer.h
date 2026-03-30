#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include "GraphicsDevice.h"
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Light.h"
#include "Material.h"
#include "Shadow/ShadowRenderer.h"
#include "Deferred/DeferredRenderer.h"
#include "PostProcess/PostProcessor.h"
#include "Culling/Frustum.h"
#include "Culling/OcclusionQuery.h"
#include "CommandBuffer/CommandBuffer.h"
#include "Instanced/InstancedRenderer.h"
#include "Performance/GPUTimer.h"
#include "SSAO/SSAO.h"
#include "SSR/SSR.h"
#include "TAA/TAA.h"
#include "Volumetric/VolumetricFog.h"
#include "SSGI/SSGI.h"
#include "PostProcess/MotionBlur.h"
#include "PostProcess/DepthOfField.h"
#include "PostProcess/LensEffects.h"
#include "../DebugUI/DebugUI.h"
#include "Sky/SkySystem.h"

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
    FBoundingBox Bounds;
    bool bUseFrustumCulling = true;
    
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

    void RenderMeshPBR(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                       class KMaterial* Material);

    void RenderSkeletalMesh(class KSkeletalMesh* InMesh, const XMMATRIX& WorldMatrix,
                            ID3D11Buffer* BoneMatrixBuffer);

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
    void SetDebugMode(bool bEnabled) { SetDebugDrawEnabled(bEnabled); }
    void DebugDrawLightVolumes();
    
    int32 GetDrawCallCount() const { return DrawCallCount; }
    int32 GetVertexCount() const { return VertexCount; }
    float GetFrameTime() const { return GPUTimer.GetFrameStats().FrameTimeMs; }

    void SetShadowEnabled(bool bEnabled);
    bool IsShadowEnabled() const { return ShadowRenderer.IsInitialized(); }
    KShadowRenderer* GetShadowRenderer() { return &ShadowRenderer; }
    void SetShadowSceneBounds(const XMFLOAT3& Center, float Radius);

    void SetRenderPath(ERenderPath Path);
    ERenderPath GetRenderPath() const { return CurrentRenderPath; }
    bool IsDeferredEnabled() const { return DeferredRenderer.IsInitialized(); }
    KDeferredRenderer* GetDeferredRenderer() { return &DeferredRenderer; }

    void SetPostProcessEnabled(bool bEnabled) { bPostProcessEnabled = bEnabled; }
    bool IsPostProcessEnabled() const { return bPostProcessEnabled && PostProcessor.IsInitialized(); }
    KPostProcessor* GetPostProcessor() { return &PostProcessor; }
    void BeginHDRPass();
    void EndHDRPass();
    void EndHDRPass(float DeltaTime);

    KShaderProgram* GetLightShader() const { return LightShader.get(); }
    KShaderProgram* GetPBRShader() const { return PBRShader.get(); }
    KShaderProgram* GetSkinnedShader() const { return SkinnedShader.get(); }
    void Cleanup();

    KGraphicsDevice* GetGraphicsDevice() const { return GraphicsDevice; }

    KShaderProgram* GetBasicShader() const { return BasicShader.get(); }
    KTextureManager* GetTextureManager() { return &TextureManager; }

    std::shared_ptr<KMesh> CreateTriangleMesh();
    std::shared_ptr<KMesh> CreateQuadMesh();
    std::shared_ptr<KMesh> CreateCubeMesh();
    std::shared_ptr<KMesh> CreateSphereMesh(UINT32 Slices = 16, UINT32 Stacks = 16);

    void SetFrustumCullingEnabled(bool bEnabled) { bFrustumCullingEnabled = bEnabled; }
    bool IsFrustumCullingEnabled() const { return bFrustumCullingEnabled; }
    KFrustum* GetFrustum() { return &Frustum; }
    bool IsVisibleInFrustum(const FBoundingSphere& Sphere) const;
    bool IsVisibleInFrustum(const FBoundingBox& Box) const;

    KInstancedRenderer* GetInstancedRenderer() { return &InstancedRenderer; }
    KGPUTimer* GetGPUTimer() { return &GPUTimer; }
    
    const FFrameStats& GetFrameStats() const { return GPUTimer.GetFrameStats(); }
    void BeginGPUTimer(const std::string& Name);
    void EndGPUTimer(const std::string& Name);

    void SetOcclusionCullingEnabled(bool bEnabled);
    bool IsOcclusionCullingEnabled() const { return bOcclusionCullingEnabled; }
    KOcclusionCuller* GetOcclusionCuller() { return &OcclusionCuller; }
    bool IsVisibleWithOcclusion(ID3D11DeviceContext* Context, const std::string& QueryName);

    void SetCommandBufferEnabled(bool bEnabled) { bCommandBufferEnabled = bEnabled; }
    bool IsCommandBufferEnabled() const { return bCommandBufferEnabled && CommandBuffer.IsInitialized(); }
    KCommandBuffer* GetCommandBuffer() { return &CommandBuffer; }
    void AddRenderCommand(const FRenderCommand& Command);
    void ExecuteCommandBuffer();
    const FCommandBufferStats& GetCommandBufferStats() const { return CommandBuffer.GetStats(); }

    void SetSSAOEnabled(bool bEnabled);
    bool IsSSAOEnabled() const { return bSSAOEnabled && SSAO.IsInitialized(); }
    KSSAO* GetSSAO() { return &SSAO; }
    void ComputeSSAO();

    void SetSSREnabled(bool bEnabled);
    bool IsSSREnabled() const { return bSSREnabled && SSR.IsInitialized(); }
    KSSR* GetSSR() { return &SSR; }
    void ComputeSSR();

    void SetTAAEnabled(bool bEnabled);
    bool IsTAAEnabled() const { return bTAAEnabled && TAA.IsInitialized(); }
    KTAA* GetTAA() { return &TAA; }
    void ApplyTAA();

    void SetVolumetricFogEnabled(bool bEnabled);
    bool IsVolumetricFogEnabled() const { return bVolumetricFogEnabled && VolumetricFog.IsInitialized(); }
    KVolumetricFog* GetVolumetricFog() { return &VolumetricFog; }
    void ComputeVolumetricFog();

    void SetSSGIEnabled(bool bEnabled);
    bool IsSSGIEnabled() const { return bSSGIEnabled && SSGI.IsInitialized(); }
    KSSGI* GetSSGI() { return &SSGI; }
    void ComputeSSGI();

    void SetMotionBlurEnabled(bool bEnabled);
    bool IsMotionBlurEnabled() const { return bMotionBlurEnabled && MotionBlur.IsInitialized(); }
    KMotionBlur* GetMotionBlur() { return &MotionBlur; }
    void ApplyMotionBlur();

    void SetDepthOfFieldEnabled(bool bEnabled);
    bool IsDepthOfFieldEnabled() const { return bDepthOfFieldEnabled && DepthOfField.IsInitialized(); }
    KDepthOfField* GetDepthOfField() { return &DepthOfField; }
    void ApplyDepthOfField();

    void SetLensEffectsEnabled(bool bEnabled);
    bool IsLensEffectsEnabled() const { return bLensEffectsEnabled && LensEffects.IsInitialized(); }
    KLensEffects* GetLensEffects() { return &LensEffects; }
    void ApplyLensEffects(float DeltaTime);
    
    KojeomEngine::KDebugUI* GetDebugUI() { return &DebugUI; }
    void SetDebugUIEnabled(bool bEnabled);
    bool IsDebugUIEnabled() const { return bDebugUIEnabled && DebugUI.IsInitialized(); }
    void RenderDebugUI();

    void SetSkyEnabled(bool bEnabled);
    bool IsSkyEnabled() const { return bSkyEnabled && SkySystem.IsInitialized(); }
    KSkySystem* GetSkySystem() { return &SkySystem; }
    void RenderSky();

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
    std::shared_ptr<KShaderProgram> PBRShader;
    std::shared_ptr<KShaderProgram> SkinnedShader;
    KTextureManager TextureManager;

    FDirectionalLight DirectionalLight;
    std::vector<FPointLight> PointLights;
    std::vector<FSpotLight> SpotLights;
    ComPtr<ID3D11Buffer> LightConstantBuffer;

    ComPtr<ID3D11Buffer> DefaultMaterialBuffer;
    KShadowRenderer ShadowRenderer;
    KDeferredRenderer DeferredRenderer;
    KPostProcessor PostProcessor;
    ComPtr<ID3D11Buffer> ShadowConstantBuffer;
    ComPtr<ID3D11Buffer> SkeletalTransformBuffer;
    ComPtr<ID3D11SamplerState> ShadowSamplerState;
    ComPtr<ID3D11SamplerState> MaterialSamplerState;
    XMFLOAT3 ShadowSceneCenter = { 0.0f, 0.0f, 0.0f };
    float ShadowSceneRadius = 50.0f;
    bool bShadowsEnabled = false;
    bool bPostProcessEnabled = false;

    ERenderPath CurrentRenderPath = ERenderPath::Forward;

    ComPtr<ID3D11RasterizerState> WireframeRasterizerState;
    std::shared_ptr<KMesh> DebugSphereMesh;

    bool bInFrame = false;
    bool bDebugDrawEnabled = false;
    bool bDebugDrawPointLightVolumes = false;
    bool bDebugDrawSpotLightVolumes = false;
    bool bFrustumCullingEnabled = true;

    KFrustum Frustum;
    KCommandBuffer CommandBuffer;
    KInstancedRenderer InstancedRenderer;
    KGPUTimer GPUTimer;
    KOcclusionCuller OcclusionCuller;
    KSSAO SSAO;
    KSSR SSR;
    KTAA TAA;
    KVolumetricFog VolumetricFog;
    KSSGI SSGI;
    KMotionBlur MotionBlur;
    KDepthOfField DepthOfField;
    KLensEffects LensEffects;
    bool bOcclusionCullingEnabled = false;
    bool bCommandBufferEnabled = true;
    bool bSSAOEnabled = false;
    bool bSSREnabled = false;
    bool bTAAEnabled = false;
    bool bVolumetricFogEnabled = false;
    bool bSSGIEnabled = false;
    bool bMotionBlurEnabled = false;
    bool bDepthOfFieldEnabled = false;
    bool bLensEffectsEnabled = false;
    bool bDebugUIEnabled = false;
    bool bSkyEnabled = false;
    
    int32 DrawCallCount = 0;
    int32 VertexCount = 0;
    
    KojeomEngine::KDebugUI DebugUI;
    KSkySystem SkySystem;
};
