#include "StaticMesh.h"

HRESULT KStaticMesh::LoadFromFile(const std::wstring& Path, ID3D11Device* Device)
{
    KBinaryArchive Archive(KBinaryArchive::EMode::Read);
    
    if (!Archive.Open(Path))
    {
        LOG_ERROR("Failed to open static mesh file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    uint32 Version;
    Archive >> Version;
    
    if (Version != STATIC_MESH_VERSION)
    {
        LOG_ERROR("Unsupported static mesh version: " + std::to_string(Version));
        Archive.Close();
        return E_FAIL;
    }

    Archive >> Name;

    uint32 NumLODs;
    Archive >> NumLODs;

    if (NumLODs > MAX_MESH_LODS)
    {
        LOG_ERROR("Static mesh LOD count exceeds maximum: " + std::to_string(NumLODs) + " (max: " + std::to_string(MAX_MESH_LODS) + ")");
        Archive.Close();
        return E_FAIL;
    }

    LODs.resize(NumLODs);

    for (uint32 i = 0; i < NumLODs; ++i)
    {
        DeserializeLOD(Archive, LODs[i]);
    }

    uint32 NumMaterialSlots;
    Archive >> NumMaterialSlots;
    MaterialSlots.resize(NumMaterialSlots);

    for (uint32 i = 0; i < NumMaterialSlots; ++i)
    {
        Archive >> MaterialSlots[i].Name;
        Archive >> MaterialSlots[i].StartIndex;
        Archive >> MaterialSlots[i].IndexCount;
        Archive >> MaterialSlots[i].MaterialIndex;
    }

    Archive >> BoundingBox.Min.x >> BoundingBox.Min.y >> BoundingBox.Min.z;
    Archive >> BoundingBox.Max.x >> BoundingBox.Max.y >> BoundingBox.Max.z;
    Archive >> BoundingSphere.Center.x >> BoundingSphere.Center.y >> BoundingSphere.Center.z;
    Archive >> BoundingSphere.Radius;

    Archive.Close();

    if (Device)
    {
        HRESULT hr = CreateRenderMeshes(Device);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create render meshes for: " + Name);
            return hr;
        }
    }

    LOG_INFO("Loaded static mesh: " + Name);
    return S_OK;
}

HRESULT KStaticMesh::SaveToFile(const std::wstring& Path)
{
    if (LODs.empty())
    {
        LOG_ERROR("Cannot save empty static mesh");
        return E_FAIL;
    }

    KBinaryArchive Archive(KBinaryArchive::EMode::Write);

    if (!Archive.Open(Path))
    {
        LOG_ERROR("Failed to create static mesh file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    Archive << STATIC_MESH_VERSION;
    Archive << Name;

    uint32 NumLODs = static_cast<uint32>(LODs.size());
    Archive << NumLODs;

    for (uint32 i = 0; i < NumLODs; ++i)
    {
        SerializeLOD(Archive, LODs[i]);
    }

    uint32 NumMaterialSlots = static_cast<uint32>(MaterialSlots.size());
    Archive << NumMaterialSlots;

    for (uint32 i = 0; i < NumMaterialSlots; ++i)
    {
        Archive << MaterialSlots[i].Name;
        Archive << MaterialSlots[i].StartIndex;
        Archive << MaterialSlots[i].IndexCount;
        Archive << MaterialSlots[i].MaterialIndex;
    }

    Archive << BoundingBox.Min.x << BoundingBox.Min.y << BoundingBox.Min.z;
    Archive << BoundingBox.Max.x << BoundingBox.Max.y << BoundingBox.Max.z;
    Archive << BoundingSphere.Center.x << BoundingSphere.Center.y << BoundingSphere.Center.z;
    Archive << BoundingSphere.Radius;

    Archive.Close();

    LOG_INFO("Saved static mesh: " + Name);
    return S_OK;
}

HRESULT KStaticMesh::CreateFromMeshData(ID3D11Device* Device, const std::vector<FVertex>& Vertices,
                                        const std::vector<uint32>& Indices)
{
    if (Vertices.empty())
    {
        LOG_ERROR("Cannot create static mesh from empty vertex data");
        return E_INVALIDARG;
    }

    LODs.clear();
    RenderMeshes.clear();

    FMeshLOD LOD0;
    LOD0.Vertices = Vertices;
    LOD0.Indices = Indices;
    LOD0.ScreenSize = 1.0f;
    LOD0.Distance = 0.0f;
    LODs.push_back(std::move(LOD0));

    CalculateBounds();

    if (Device)
    {
        HRESULT hr = CreateRenderMeshes(Device);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create render meshes");
            return hr;
        }
    }

    return S_OK;
}

void KStaticMesh::SetLODData(uint32 LODIndex, const std::vector<FVertex>& Vertices, const std::vector<uint32>& Indices)
{
    if (LODIndex >= LODs.size())
    {
        LODs.resize(LODIndex + 1);
    }

    LODs[LODIndex].Vertices = Vertices;
    LODs[LODIndex].Indices = Indices;
    LODs[LODIndex].ScreenSize = 1.0f / (LODIndex + 1);
    LODs[LODIndex].Distance = 0.0f;

    bRenderMeshesCreated = false;
}

void KStaticMesh::AddLOD(const std::vector<FVertex>& Vertices, const std::vector<uint32>& Indices, float ScreenSize)
{
    FMeshLOD newLOD;
    newLOD.Vertices = Vertices;
    newLOD.Indices = Indices;
    newLOD.ScreenSize = ScreenSize;
    newLOD.Distance = 0.0f;
    LODs.push_back(std::move(newLOD));

    bRenderMeshesCreated = false;
}

HRESULT KStaticMesh::CreateRenderMeshes(ID3D11Device* Device)
{
    if (!Device) return E_INVALIDARG;

    RenderMeshes.clear();
    RenderMeshes.resize(LODs.size());

    for (size_t i = 0; i < LODs.size(); ++i)
    {
        auto mesh = std::make_shared<KMesh>();
        
        HRESULT hr = mesh->Initialize(
            Device,
            LODs[i].Vertices.data(),
            static_cast<uint32>(LODs[i].Vertices.size()),
            LODs[i].Indices.empty() ? nullptr : LODs[i].Indices.data(),
            static_cast<uint32>(LODs[i].Indices.size())
        );

        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create render mesh for LOD " + std::to_string(i));
            return hr;
        }

        RenderMeshes[i] = std::move(mesh);
    }

    bRenderMeshesCreated = true;
    return S_OK;
}

void KStaticMesh::Render(ID3D11DeviceContext* Context, uint32 LODIndex)
{
    KMesh* mesh = GetRenderMesh(LODIndex);
    if (mesh)
    {
        mesh->Render(Context);
    }
}

KMesh* KStaticMesh::GetRenderMesh(uint32 LODIndex)
{
    if (LODIndex < RenderMeshes.size() && RenderMeshes[LODIndex])
    {
        return RenderMeshes[LODIndex].get();
    }
    return nullptr;
}

const KMesh* KStaticMesh::GetRenderMesh(uint32 LODIndex) const
{
    if (LODIndex < RenderMeshes.size() && RenderMeshes[LODIndex])
    {
        return RenderMeshes[LODIndex].get();
    }
    return nullptr;
}

std::shared_ptr<KMesh> KStaticMesh::GetRenderMeshShared(uint32 LODIndex)
{
    if (LODIndex < RenderMeshes.size())
    {
        return RenderMeshes[LODIndex];
    }
    return nullptr;
}

const FMaterialSlot* KStaticMesh::GetMaterialSlot(uint32 Index) const
{
    if (Index < MaterialSlots.size())
    {
        return &MaterialSlots[Index];
    }
    return nullptr;
}

void KStaticMesh::CalculateBounds()
{
    if (LODs.empty() || LODs[0].Vertices.empty())
    {
        BoundingBox.Reset();
        BoundingSphere = FBoundingSphere();
        return;
    }

    BoundingBox.Reset();
    
    for (const auto& vertex : LODs[0].Vertices)
    {
        BoundingBox.Expand(vertex.Position);
    }

    BoundingSphere.FromBoundingBox(BoundingBox);
}

void KStaticMesh::SerializeLOD(KBinaryArchive& Archive, FMeshLOD& LOD)
{
    uint32 VertexCount = static_cast<uint32>(LOD.Vertices.size());
    uint32 IndexCount = static_cast<uint32>(LOD.Indices.size());

    Archive << VertexCount << IndexCount;
    Archive << LOD.ScreenSize << LOD.Distance;

    for (const auto& vertex : LOD.Vertices)
    {
        Archive << vertex.Position.x << vertex.Position.y << vertex.Position.z;
        Archive << vertex.Color.x << vertex.Color.y << vertex.Color.z << vertex.Color.w;
        Archive << vertex.Normal.x << vertex.Normal.y << vertex.Normal.z;
        Archive << vertex.TexCoord.x << vertex.TexCoord.y;
    }

    for (const auto& index : LOD.Indices)
    {
        Archive << index;
    }
}

void KStaticMesh::DeserializeLOD(KBinaryArchive& Archive, FMeshLOD& LOD)
{
    uint32 VertexCount, IndexCount;
    Archive >> VertexCount >> IndexCount;
    Archive >> LOD.ScreenSize >> LOD.Distance;

    static constexpr uint32 MAX_VERTICES_PER_LOD = 5000000;
    static constexpr uint32 MAX_INDICES_PER_LOD = 15000000;

    if (VertexCount > MAX_VERTICES_PER_LOD)
    {
        LOG_ERROR("LOD vertex count exceeds maximum: " + std::to_string(VertexCount));
        return;
    }
    if (IndexCount > MAX_INDICES_PER_LOD)
    {
        LOG_ERROR("LOD index count exceeds maximum: " + std::to_string(IndexCount));
        return;
    }

    LOD.Vertices.resize(VertexCount);
    LOD.Indices.resize(IndexCount);

    for (auto& vertex : LOD.Vertices)
    {
        Archive >> vertex.Position.x >> vertex.Position.y >> vertex.Position.z;
        Archive >> vertex.Color.x >> vertex.Color.y >> vertex.Color.z >> vertex.Color.w;
        Archive >> vertex.Normal.x >> vertex.Normal.y >> vertex.Normal.z;
        Archive >> vertex.TexCoord.x >> vertex.TexCoord.y;
    }

    for (auto& index : LOD.Indices)
    {
        Archive >> index;
    }
}

void KStaticMesh::Cleanup()
{
    LODs.clear();
    RenderMeshes.clear();
    MaterialSlots.clear();
    bRenderMeshesCreated = false;
}
