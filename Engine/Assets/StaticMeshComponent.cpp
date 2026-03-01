#include "StaticMeshComponent.h"
#include "../Graphics/Renderer.h"

void KStaticMeshComponent::Render(KRenderer* Renderer)
{
    if (!StaticMesh || !Renderer || !GetOwner())
        return;

    XMMATRIX worldMatrix = GetOwner()->GetWorldMatrix();
    std::shared_ptr<KMesh> renderMesh = StaticMesh->GetRenderMeshShared(0);
    
    if (!renderMesh)
        return;

    if (Shader)
    {
        FRenderObject renderObject(renderMesh, Shader);
        renderObject.WorldMatrix = worldMatrix;
        Renderer->RenderObject(renderObject);
    }
    else
    {
        Renderer->RenderMeshLit(renderMesh, worldMatrix, nullptr);
    }
}

void KStaticMeshComponent::SetStaticMesh(std::shared_ptr<KStaticMesh> InMesh)
{
    StaticMesh = InMesh;
}

void KStaticMeshComponent::Serialize(KBinaryArchive& Archive)
{
    std::string meshPath;
    if (StaticMesh)
    {
        // Store mesh path for later loading
    }
    Archive << meshPath;
    Archive << bCastShadow;
}

void KStaticMeshComponent::Deserialize(KBinaryArchive& Archive)
{
    std::string meshPath;
    Archive >> meshPath;
    Archive >> bCastShadow;
    // Mesh loading would be done by asset manager
}
