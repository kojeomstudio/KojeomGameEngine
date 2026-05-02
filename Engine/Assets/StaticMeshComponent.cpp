#include "StaticMeshComponent.h"
#include "../Graphics/Renderer.h"
#include "../Utils/Common.h"

KStaticMeshComponent::KStaticMeshComponent()
{
    Material = std::make_shared<KMaterial>();
}

void KStaticMeshComponent::Render(KRenderer* Renderer)
{
    if (!StaticMesh || !Renderer || !GetOwner())
        return;

    XMMATRIX worldMatrix = GetOwner()->GetWorldMatrix();

    uint32 selectedLOD = 0;
    if (StaticMesh->GetLODCount() > 1)
    {
        KCamera* camera = Renderer->GetCamera();
        if (camera && StaticMesh->GetBoundingSphere().Radius > 0.0f)
        {
            XMFLOAT3 actorPos;
            XMStoreFloat3(&actorPos, GetOwner()->GetWorldMatrix().r[3]);
            XMVECTOR camPos = XMLoadFloat3(&camera->GetPosition());
            XMVECTOR actPos = XMLoadFloat3(&actorPos);
            float distance = XMVectorGetX(XMVector3Length(camPos - actPos));

            uint32 lodCount = StaticMesh->GetLODCount();
            for (uint32 i = 0; i < lodCount; ++i)
            {
                const FMeshLOD* lod = StaticMesh->GetLOD(i);
                if (lod && distance <= lod->Distance && lod->Distance > 0.0f)
                {
                    selectedLOD = i;
                    break;
                }
                if (i == lodCount - 1)
                {
                    selectedLOD = i;
                }
            }
        }
    }

    std::shared_ptr<KMesh> renderMesh = StaticMesh->GetRenderMeshShared(selectedLOD);
    
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

HRESULT KStaticMeshComponent::CreateDeviceResources(ID3D11Device* Device)
{
    if (!Device || !StaticMesh) return S_OK;

    std::wstring sourcePath = StaticMesh->GetSourcePath();
    if (!sourcePath.empty() && !StaticMesh->GetRenderMesh(0))
    {
        HRESULT hr = StaticMesh->LoadFromFile(sourcePath, Device);
        if (FAILED(hr))
        {
            LOG_ERROR("StaticMeshComponent::CreateDeviceResources: failed to load mesh from: " + StringUtils::WideToMultiByte(sourcePath));
            return hr;
        }
        LOG_INFO("StaticMeshComponent::CreateDeviceResources: loaded mesh from: " + StringUtils::WideToMultiByte(sourcePath));
    }

    return S_OK;
}

void KStaticMeshComponent::Serialize(KBinaryArchive& Archive)
{
    KActorComponent::Serialize(Archive);

    std::string meshPath;
    std::string meshSourcePath;
    if (StaticMesh)
    {
        meshPath = StaticMesh->GetName();
        meshSourcePath = StringUtils::WideToMultiByte(StaticMesh->GetSourcePath());
    }
    Archive << meshPath;
    Archive << meshSourcePath;
    Archive << bCastShadow;

    if (Material)
    {
        const FPBRMaterialParams& params = Material->GetParams();
        Archive << params.AlbedoColor.x << params.AlbedoColor.y << params.AlbedoColor.z << params.AlbedoColor.w;
        Archive << params.Metallic;
        Archive << params.Roughness;
        Archive << params.AO;
    }
    else
    {
        XMFLOAT4 white(1, 1, 1, 1);
        Archive << white.x << white.y << white.z << white.w;
        float zero = 0.0f;
        Archive << zero << zero << zero;
    }
}

void KStaticMeshComponent::Deserialize(KBinaryArchive& Archive)
{
    KActorComponent::Deserialize(Archive);

    std::string meshPath;
    std::string meshSourcePath;
    Archive >> meshPath;
    Archive >> meshSourcePath;
    Archive >> bCastShadow;

    if (Material)
    {
        float albedoR, albedoG, albedoB, albedoA;
        float metallic, roughness, ao;
        Archive >> albedoR >> albedoG >> albedoB >> albedoA;
        Archive >> metallic;
        Archive >> roughness;
        Archive >> ao;
        Material->SetAlbedoColor(XMFLOAT4(albedoR, albedoG, albedoB, albedoA));
        Material->SetMetallic(metallic);
        Material->SetRoughness(roughness);
        Material->SetAO(ao);
    }
    else
    {
        XMFLOAT4 color;
        float val;
        Archive >> color.x >> color.y >> color.z >> color.w;
        Archive >> val >> val >> val;
    }

    if (!meshPath.empty() && !StaticMesh)
    {
        auto mesh = std::make_shared<KStaticMesh>();
        mesh->SetName(meshPath);
        if (!meshSourcePath.empty())
        {
            mesh->SetSourcePath(StringUtils::MultiByteToWide(meshSourcePath));
        }
        StaticMesh = mesh;
        LOG_INFO("StaticMeshComponent::Deserialize: mesh '" + meshPath + "' queued for deferred GPU resource creation");
    }
}
