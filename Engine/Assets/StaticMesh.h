#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include "../Utils/Math.h"
#include "../Graphics/Mesh.h"
#include "../Serialization/BinaryArchive.h"
#include <vector>
#include <memory>

constexpr uint32 STATIC_MESH_VERSION = 1;
constexpr uint32 MAX_MESH_LODS = 4;

struct FMeshLOD
{
    std::vector<FVertex> Vertices;
    std::vector<uint32> Indices;
    float ScreenSize;
    float Distance;

    FMeshLOD() : ScreenSize(1.0f), Distance(0.0f) {}
};

class KStaticMesh
{
public:
    KStaticMesh() = default;
    ~KStaticMesh() = default;

    KStaticMesh(const KStaticMesh&) = delete;
    KStaticMesh& operator=(const KStaticMesh&) = delete;

    HRESULT LoadFromFile(const std::wstring& Path, ID3D11Device* Device);
    HRESULT SaveToFile(const std::wstring& Path);

    HRESULT CreateFromMeshData(ID3D11Device* Device, const std::vector<FVertex>& Vertices,
                               const std::vector<uint32>& Indices);

    void Render(ID3D11DeviceContext* Context, uint32 LODIndex = 0);

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    uint32 GetLODCount() const { return static_cast<uint32>(LODs.size()); }
    FMeshLOD* GetLOD(uint32 Index) { return Index < LODs.size() ? &LODs[Index] : nullptr; }
    const FMeshLOD* GetLOD(uint32 Index) const { return Index < LODs.size() ? &LODs[Index] : nullptr; }

    KMesh* GetRenderMesh(uint32 LODIndex = 0);
    const KMesh* GetRenderMesh(uint32 LODIndex = 0) const;
    std::shared_ptr<KMesh> GetRenderMeshShared(uint32 LODIndex = 0);

    const FBoundingBox& GetBoundingBox() const { return BoundingBox; }
    const FBoundingSphere& GetBoundingSphere() const { return BoundingSphere; }

    uint32 GetMaterialSlotCount() const { return static_cast<uint32>(MaterialSlots.size()); }
    const FMaterialSlot* GetMaterialSlot(uint32 Index) const;
    void AddMaterialSlot(const FMaterialSlot& Slot) { MaterialSlots.push_back(Slot); }

    void CalculateBounds();

    bool IsValid() const { return !LODs.empty() && !LODs[0].Vertices.empty(); }

    void Cleanup();

private:
    HRESULT CreateRenderMeshes(ID3D11Device* Device);
    void SerializeLOD(KBinaryArchive& Archive, FMeshLOD& LOD);
    void DeserializeLOD(KBinaryArchive& Archive, FMeshLOD& LOD);

private:
    std::string Name;
    std::vector<FMeshLOD> LODs;
    std::vector<std::shared_ptr<KMesh>> RenderMeshes;
    std::vector<FMaterialSlot> MaterialSlots;
    FBoundingBox BoundingBox;
    FBoundingSphere BoundingSphere;
    bool bRenderMeshesCreated = false;
};
