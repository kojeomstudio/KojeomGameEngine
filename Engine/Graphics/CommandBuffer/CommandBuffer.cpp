#include "CommandBuffer.h"

#include <cstring>
#include "../Culling/Frustum.h"
#include <chrono>

HRESULT KCommandBuffer::Initialize(ID3D11Device* InDevice)
{
    if (!InDevice)
    {
        LOG_ERROR("Invalid device for CommandBuffer");
        return E_INVALIDARG;
    }
    
    Device = InDevice;
    Commands.reserve(256);
    bInitialized = true;
    
    LOG_INFO("CommandBuffer initialized");
    return S_OK;
}

void KCommandBuffer::Cleanup()
{
    Commands.clear();
    Commands.shrink_to_fit();
    
    bInitialized = false;
}

void KCommandBuffer::BeginFrame()
{
    Commands.clear();
    Stats = FCommandBufferStats();
    bInFrame = true;
}

void KCommandBuffer::EndFrame()
{
    bInFrame = false;
}

void KCommandBuffer::AddCommand(const FRenderCommand& Command)
{
    FRenderCommand Cmd = Command;
    Cmd.SortKey = CalculateSortKey(Cmd);
    Commands.push_back(Cmd);
    Stats.TotalCommands++;
    
    if (Cmd.Type == ERenderCommandType::Draw ||
        Cmd.Type == ERenderCommandType::DrawIndexed ||
        Cmd.Type == ERenderCommandType::DrawInstanced ||
        Cmd.Type == ERenderCommandType::DrawIndexedInstanced)
    {
        Stats.DrawCommands++;
    }
}

void KCommandBuffer::AddDrawCommand(std::shared_ptr<KMesh> Mesh, std::shared_ptr<KShaderProgram> Shader,
                                     const XMMATRIX& WorldMatrix, std::shared_ptr<KTexture> Texture)
{
    FRenderCommand Cmd;
    Cmd.Mesh = Mesh;
    Cmd.Shader = Shader;
    Cmd.Texture = Texture;
    Cmd.WorldMatrix = WorldMatrix;
    Cmd.Type = ERenderCommandType::DrawIndexed;
    
    if (Mesh)
    {
        Cmd.IndexCount = Mesh->GetIndexCount();
        Cmd.VertexCount = Mesh->GetVertexCount();
    }
    
    AddCommand(Cmd);
}

void KCommandBuffer::AddPBRCommand(std::shared_ptr<KMesh> Mesh, KMaterial* Material,
                                    const XMMATRIX& WorldMatrix)
{
    FRenderCommand Cmd;
    Cmd.Mesh = Mesh;
    Cmd.Material = Material;
    Cmd.WorldMatrix = WorldMatrix;
    Cmd.Type = ERenderCommandType::DrawIndexed;
    
    if (Mesh)
    {
        Cmd.IndexCount = Mesh->GetIndexCount();
        Cmd.VertexCount = Mesh->GetVertexCount();
    }
    
    AddCommand(Cmd);
}

void KCommandBuffer::AddCustomCommand(std::function<void(ID3D11DeviceContext*)> CustomFunc, INT32 Priority)
{
    FRenderCommand Cmd;
    Cmd.Type = ERenderCommandType::Custom;
    Cmd.CustomCommand = CustomFunc;
    Cmd.Priority = Priority;
    AddCommand(Cmd);
}

UINT64 KCommandBuffer::CalculateSortKey(const FRenderCommand& Command)
{
    UINT64 Key = 0;
    
    switch (CurrentSortKey)
    {
    case ERenderSortKey::None:
        Key = static_cast<UINT64>(Commands.size());
        break;
        
    case ERenderSortKey::Shader:
        {
            UINT64 ShaderId = Command.Shader ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Shader.get())) : 0;
            Key = (ShaderId & 0xFFFFFFFF) << 32;
            Key |= (static_cast<UINT64>(Command.Priority + 1000) & 0xFFFF);
        }
        break;
        
    case ERenderSortKey::Texture:
        {
            UINT64 TextureId = Command.Texture ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Texture.get())) : 0;
            Key = (TextureId & 0xFFFFFFFF) << 32;
            Key |= (static_cast<UINT64>(Command.Priority + 1000) & 0xFFFF);
        }
        break;
        
    case ERenderSortKey::Material:
        {
            UINT64 MaterialId = Command.Material ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Material)) : 0;
            Key = (MaterialId & 0xFFFFFFFF) << 32;
            Key |= (static_cast<UINT64>(Command.Priority + 1000) & 0xFFFF);
        }
        break;
        
    case ERenderSortKey::Depth:
        {
            XMFLOAT3 Translation;
            XMStoreFloat3(&Translation, Command.WorldMatrix.r[3]);
            XMVECTOR Pos = XMVector3TransformCoord(XMLoadFloat3(&Translation), CurrentViewProjection);
            float Depth = XMVectorGetZ(Pos);
            UINT32 DepthInt;
            std::memcpy(&DepthInt, &Depth, sizeof(Depth));
            Key = DepthInt;
        }
        break;
        
    case ERenderSortKey::ShaderThenTexture:
        {
            UINT64 ShaderId = Command.Shader ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Shader.get())) : 0;
            UINT64 TextureId = Command.Texture ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Texture.get())) : 0;
            Key = ((ShaderId & 0xFFFF) << 48);
            Key |= ((TextureId & 0xFFFF) << 32);
            Key |= (static_cast<UINT64>(Command.Priority + 1000) & 0xFFFF) << 16;
        }
        break;
        
    case ERenderSortKey::MaterialThenDepth:
        {
            UINT64 MaterialId = Command.Material ? static_cast<UINT64>(reinterpret_cast<uintptr_t>(Command.Material)) : 0;
            XMFLOAT3 Translation;
            XMStoreFloat3(&Translation, Command.WorldMatrix.r[3]);
            XMVECTOR Pos = XMVector3TransformCoord(XMLoadFloat3(&Translation), CurrentViewProjection);
            float Depth = XMVectorGetZ(Pos);
            UINT32 DepthInt;
            std::memcpy(&DepthInt, &Depth, sizeof(Depth));
            Key = ((MaterialId & 0xFFFF) << 48);
            Key |= (static_cast<UINT64>(DepthInt & 0xFFFFFFFF));
        }
        break;
    }
    
    return Key;
}

void KCommandBuffer::SortCommands()
{
    if (!bSortEnabled || Commands.empty())
        return;
    
    auto StartTime = std::chrono::high_resolution_clock::now();
    
    std::sort(Commands.begin(), Commands.end(),
        [](const FRenderCommand& A, const FRenderCommand& B) -> bool
        {
            if (A.Priority != B.Priority)
                return A.Priority > B.Priority;
            return A.SortKey < B.SortKey;
        });
    
    auto EndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> Duration = EndTime - StartTime;
    Stats.SortingTimeMs = Duration.count();
}

void KCommandBuffer::Execute(ID3D11DeviceContext* Context)
{
    if (!Context || Commands.empty())
        return;
    
    auto StartTime = std::chrono::high_resolution_clock::now();
    
    SortCommands();
    
    FRenderCommand PreviousCmd;
    for (const FRenderCommand& Cmd : Commands)
    {
        ExecuteCommand(Context, Cmd);
        UpdateStats(Cmd, PreviousCmd);
        PreviousCmd = Cmd;
    }
    
    auto EndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> Duration = EndTime - StartTime;
    Stats.ExecutionTimeMs = Duration.count();
}

void KCommandBuffer::ExecuteWithFrustumCulling(ID3D11DeviceContext* Context, const KFrustum& Frustum)
{
    if (!Context || Commands.empty())
        return;
    
    auto StartTime = std::chrono::high_resolution_clock::now();
    
    SortCommands();
    
    UINT32 CulledCount = 0;
    FRenderCommand PreviousCmd;
    
    for (const FRenderCommand& Cmd : Commands)
    {
        if (Cmd.bUseFrustumCulling && Cmd.Mesh)
        {
            FBoundingSphere Sphere;
            XMFLOAT4X4 World;
            XMStoreFloat4x4(&World, Cmd.WorldMatrix);
            Sphere.Center = XMFLOAT3(World._41, World._42, World._43);
            
            if (Cmd.Bounds.IsValid())
            {
                Sphere.Radius = Cmd.Bounds.GetRadius();
            }
            else
            {
                Sphere.Radius = 1.0f;
            }
            
            if (!Frustum.ContainsSphere(Sphere))
            {
                CulledCount++;
                continue;
            }
        }
        
        ExecuteCommand(Context, Cmd);
        UpdateStats(Cmd, PreviousCmd);
        PreviousCmd = Cmd;
    }
    
    Stats.CulledCommands = CulledCount;
    
    auto EndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> Duration = EndTime - StartTime;
    Stats.ExecutionTimeMs = Duration.count();
}

void KCommandBuffer::ExecuteCommand(ID3D11DeviceContext* Context, const FRenderCommand& Command)
{
    ID3D11Buffer* pVertexBuffer = nullptr;
    ID3D11Buffer* pIndexBuffer = nullptr;
    UINT Stride = sizeof(FVertex);
    UINT Offset = 0;
    
    switch (Command.Type)
    {
    case ERenderCommandType::Draw:
        if (Command.Mesh)
        {
            if (Command.Shader) Command.Shader->Bind(Context);
            pVertexBuffer = Command.Mesh->GetVertexBuffer();
            Context->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
            Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context->Draw(Command.VertexCount, Command.StartVertex);
        }
        break;
        
    case ERenderCommandType::DrawIndexed:
        if (Command.Mesh)
        {
            if (Command.Shader) Command.Shader->Bind(Context);
            pVertexBuffer = Command.Mesh->GetVertexBuffer();
            pIndexBuffer = Command.Mesh->GetIndexBuffer();
            Context->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
            Context->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context->DrawIndexed(Command.IndexCount, Command.StartIndex, Command.StartVertex);
        }
        break;
        
    case ERenderCommandType::DrawInstanced:
        if (Command.Mesh)
        {
            if (Command.Shader) Command.Shader->Bind(Context);
            pVertexBuffer = Command.Mesh->GetVertexBuffer();
            Context->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
            Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context->DrawInstanced(Command.VertexCount, Command.InstanceCount, 
                                   Command.StartVertex, 0);
        }
        break;
        
    case ERenderCommandType::DrawIndexedInstanced:
        if (Command.Mesh)
        {
            if (Command.Shader) Command.Shader->Bind(Context);
            pVertexBuffer = Command.Mesh->GetVertexBuffer();
            pIndexBuffer = Command.Mesh->GetIndexBuffer();
            Context->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
            Context->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context->DrawIndexedInstanced(Command.IndexCount, Command.InstanceCount,
                                          Command.StartIndex, Command.StartVertex, 0);
        }
        break;
        
    case ERenderCommandType::SetShader:
        if (Command.Shader)
        {
            Command.Shader->Bind(Context);
        }
        break;
        
    case ERenderCommandType::SetTexture:
        if (Command.Texture)
        {
            Command.Texture->Bind(Context, Command.TextureSlot);
        }
        break;
        
    case ERenderCommandType::SetConstantBuffer:
        if (Command.ConstantBuffer)
        {
            Context->VSSetConstantBuffers(Command.ConstantBufferSlot, 1, &Command.ConstantBuffer);
            Context->PSSetConstantBuffers(Command.ConstantBufferSlot, 1, &Command.ConstantBuffer);
        }
        break;
        
    case ERenderCommandType::Custom:
        if (Command.CustomCommand)
        {
            Command.CustomCommand(Context);
        }
        break;
        
    default:
        break;
    }
}

void KCommandBuffer::UpdateStats(const FRenderCommand& Current, const FRenderCommand& Previous)
{
    if (Current.Shader && Current.Shader.get() != Previous.Shader.get())
    {
        Stats.StateChanges++;
    }
    
    if (Current.Texture && Current.Texture.get() != Previous.Texture.get())
    {
        Stats.StateChanges++;
    }
    
    if (Current.Material && Current.Material != Previous.Material)
    {
        Stats.StateChanges++;
    }
}

void KCommandBuffer::Clear()
{
    Commands.clear();
    Stats = FCommandBufferStats();
}

void KCommandBuffer::Reserve(UINT32 Count)
{
    Commands.reserve(Count);
}
