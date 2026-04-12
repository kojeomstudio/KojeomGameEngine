#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Mesh.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../Material.h"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

struct FVertex;

enum class ERenderCommandType : UINT8
{
    Draw,
    DrawIndexed,
    DrawInstanced,
    DrawIndexedInstanced,
    SetShader,
    SetTexture,
    SetConstantBuffer,
    SetBlendState,
    SetRasterizerState,
    SetDepthStencilState,
    SetRenderTarget,
    SetViewport,
    ClearRenderTarget,
    Custom
};

enum class ERenderSortKey : UINT8
{
    None,
    Shader,
    Texture,
    Material,
    Depth,
    ShaderThenTexture,
    MaterialThenDepth
};

struct FRenderCommand
{
    ERenderCommandType Type = ERenderCommandType::Draw;
    UINT64 SortKey = 0;
    INT32 Priority = 0;
    
    std::shared_ptr<KMesh> Mesh;
    std::shared_ptr<KShaderProgram> Shader;
    std::shared_ptr<KTexture> Texture;
    KMaterial* Material = nullptr;
    
    XMMATRIX WorldMatrix = XMMatrixIdentity();
    FBoundingBox Bounds;
    
    UINT32 VertexCount = 0;
    UINT32 IndexCount = 0;
    UINT32 InstanceCount = 1;
    UINT32 StartVertex = 0;
    UINT32 StartIndex = 0;
    
    UINT8 TextureSlot = 0;
    UINT8 ConstantBufferSlot = 0;
    ID3D11Buffer* ConstantBuffer = nullptr;
    
    bool bUseFrustumCulling = true;
    bool bCastShadow = true;
    bool bReceiveShadow = true;
    
    std::function<void(ID3D11DeviceContext*)> CustomCommand;
    
    FRenderCommand() = default;
    
    FRenderCommand(std::shared_ptr<KMesh> InMesh, std::shared_ptr<KShaderProgram> InShader,
                   const XMMATRIX& InWorldMatrix, std::shared_ptr<KTexture> InTexture = nullptr)
        : Mesh(InMesh), Shader(InShader), Texture(InTexture), WorldMatrix(InWorldMatrix)
    {
        Type = ERenderCommandType::DrawIndexed;
        if (Mesh)
        {
            IndexCount = Mesh->GetIndexCount();
            VertexCount = Mesh->GetVertexCount();
        }
    }
};

struct FCommandBufferStats
{
    UINT32 TotalCommands = 0;
    UINT32 DrawCommands = 0;
    UINT32 StateChanges = 0;
    UINT32 CulledCommands = 0;
    float SortingTimeMs = 0.0f;
    float ExecutionTimeMs = 0.0f;
};

class KCommandBuffer
{
public:
    KCommandBuffer() = default;
    ~KCommandBuffer() = default;
    
    KCommandBuffer(const KCommandBuffer&) = delete;
    KCommandBuffer& operator=(const KCommandBuffer&) = delete;
    
    HRESULT Initialize(ID3D11Device* InDevice);
    void Cleanup();
    
    void BeginFrame();
    void EndFrame();
    
    void AddCommand(const FRenderCommand& Command);
    void AddDrawCommand(std::shared_ptr<KMesh> Mesh, std::shared_ptr<KShaderProgram> Shader,
                        const XMMATRIX& WorldMatrix, std::shared_ptr<KTexture> Texture = nullptr);
    void AddPBRCommand(std::shared_ptr<KMesh> Mesh, KMaterial* Material, const XMMATRIX& WorldMatrix);
    void AddCustomCommand(std::function<void(ID3D11DeviceContext*)> CustomFunc, INT32 Priority = 0);
    
    void SetSortKey(ERenderSortKey SortKey) { CurrentSortKey = SortKey; }
    void SetSortEnabled(bool bEnabled) { bSortEnabled = bEnabled; }
    void SetCullingEnabled(bool bEnabled) { bCullingEnabled = bEnabled; }
    void SetViewProjection(const XMMATRIX& InViewProj) { CurrentViewProjection = InViewProj; }
    
    void Execute(ID3D11DeviceContext* Context);
    void ExecuteWithFrustumCulling(ID3D11DeviceContext* Context, const class KFrustum& Frustum);
    
    void Clear();
    void Reserve(UINT32 Count);
    
    UINT32 GetCommandCount() const { return static_cast<UINT32>(Commands.size()); }
    const FCommandBufferStats& GetStats() const { return Stats; }
    bool IsInitialized() const { return bInitialized; }
    
private:
    UINT64 CalculateSortKey(const FRenderCommand& Command);
    void SortCommands();
    void ExecuteCommand(ID3D11DeviceContext* Context, const FRenderCommand& Command);
    void UpdateStats(const FRenderCommand& Current, const FRenderCommand& Previous);
    
private:
    ID3D11Device* Device = nullptr;
    std::vector<FRenderCommand> Commands;
    
    ERenderSortKey CurrentSortKey = ERenderSortKey::ShaderThenTexture;
    XMMATRIX CurrentViewProjection = XMMatrixIdentity();
    bool bSortEnabled = true;
    bool bCullingEnabled = true;
    bool bInitialized = false;
    bool bInFrame = false;
    
    FCommandBufferStats Stats;
};
