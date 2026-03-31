#include "StaticMeshComponent.h"
#include "../Graphics/Renderer.h"

KStaticMeshComponent::KStaticMeshComponent()
{
    Material = std::make_shared<KMaterial>();
}

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
    KActorComponent::Serialize(Archive);

    std::string meshPath;
    if (StaticMesh)
    {
        meshPath = StaticMesh->GetName();
    }
    Archive << meshPath;
    Archive << bCastShadow;
}

void KStaticMeshComponent::Deserialize(KBinaryArchive& Archive)
{
    KActorComponent::Deserialize(Archive);

    std::string meshPath;
    Archive >> meshPath;
    Archive >> bCastShadow;
}
