#include "LODGenerator.h"
#include <queue>
#include <algorithm>
#include <limits>

std::vector<FVertex> KLODGenerator::SimplifyMesh(
    const std::vector<FVertex>& InVertices,
    const std::vector<uint32>& InIndices,
    const FLODGenerationParams& Params)
{
    if (InVertices.empty() || InIndices.empty())
    {
        LOG_WARNING("LODGenerator: Cannot simplify empty mesh");
        return InVertices;
    }

    std::vector<FVertex> WorkingVertices = InVertices;
    std::vector<FTriangle> Triangles;
    Triangles.reserve(InIndices.size() / 3);

    for (size_t i = 0; i < InIndices.size(); i += 3)
    {
        FTriangle Tri;
        Tri.V0 = InIndices[i];
        Tri.V1 = InIndices[i + 1];
        Tri.V2 = InIndices[i + 2];
        
        XMVECTOR V0 = XMLoadFloat3(&WorkingVertices[Tri.V0].Position);
        XMVECTOR V1 = XMLoadFloat3(&WorkingVertices[Tri.V1].Position);
        XMVECTOR V2 = XMLoadFloat3(&WorkingVertices[Tri.V2].Position);
        
        XMVECTOR Edge1 = XMVectorSubtract(V1, V0);
        XMVECTOR Edge2 = XMVectorSubtract(V2, V0);
        XMVECTOR Normal = XMVector3Cross(Edge1, Edge2);
        Normal = XMVector3Normalize(Normal);
        XMStoreFloat3(&Tri.Normal, Normal);
        
        Tri.bIsValid = true;
        Triangles.push_back(Tri);
    }

    std::vector<FQuadricMatrix> Quadrics(WorkingVertices.size());
    ComputeQuadrics(WorkingVertices, InIndices, Quadrics);

    uint32 OriginalTriCount = static_cast<uint32>(Triangles.size());
    uint32 TargetTriCount = Params.TargetTriangleCount;
    if (TargetTriCount == 0)
    {
        TargetTriCount = static_cast<uint32>(OriginalTriCount * Params.ReductionFactor);
    }
    TargetTriCount = std::max(TargetTriCount, 4u);

    std::unordered_map<FEdge, std::vector<uint32>, FEdgeHash> EdgeTriangles;
    for (size_t i = 0; i < Triangles.size(); ++i)
    {
        if (!Triangles[i].bIsValid) continue;
        
        FEdge Edges[3] = {
            { Triangles[i].V0, Triangles[i].V1 },
            { Triangles[i].V1, Triangles[i].V2 },
            { Triangles[i].V2, Triangles[i].V0 }
        };
        
        for (int j = 0; j < 3; ++j)
        {
            EdgeTriangles[Edges[j]].push_back(static_cast<uint32>(i));
        }
    }

    auto ComputeCost = [&](uint32 V1, uint32 V2) -> std::pair<float, XMFLOAT3>
    {
        FQuadricMatrix Combined = Quadrics[V1] + Quadrics[V2];
        
        XMFLOAT3 MidPoint;
        XMVECTOR Pos1 = XMLoadFloat3(&WorkingVertices[V1].Position);
        XMVECTOR Pos2 = XMLoadFloat3(&WorkingVertices[V2].Position);
        XMStoreFloat3(&MidPoint, XMVectorScale(XMVectorAdd(Pos1, Pos2), 0.5f));

        float Cost = Combined.Evaluate(MidPoint);
        return { Cost, MidPoint };
    };

    std::priority_queue<FEdgeCollapse> EdgeQueue;
    
    for (auto& Pair : EdgeTriangles)
    {
        const FEdge& Edge = Pair.first;
        auto [Cost, NewPos] = ComputeCost(Edge.V0, Edge.V1);
        
        FEdgeCollapse Collapse;
        Collapse.V1 = Edge.V0;
        Collapse.V2 = Edge.V1;
        Collapse.Cost = Cost;
        Collapse.NewPosition = NewPos;
        EdgeQueue.push(Collapse);
    }

    uint32 CurrentTriCount = OriginalTriCount;
    std::vector<bool> VertexRemoved(WorkingVertices.size(), false);

    while (CurrentTriCount > TargetTriCount && !EdgeQueue.empty())
    {
        FEdgeCollapse Collapse = EdgeQueue.top();
        EdgeQueue.pop();

        if (VertexRemoved[Collapse.V1] || VertexRemoved[Collapse.V2])
            continue;

        auto It1 = EdgeTriangles.find({ Collapse.V1, Collapse.V2 });
        if (It1 == EdgeTriangles.end())
            continue;

        uint32 VKeep = Collapse.V1;
        uint32 VRemove = Collapse.V2;

        WorkingVertices[VKeep].Position = Collapse.NewPosition;
        
        XMVECTOR N1 = XMLoadFloat3(&WorkingVertices[VKeep].Normal);
        XMVECTOR N2 = XMLoadFloat3(&WorkingVertices[VRemove].Normal);
        XMStoreFloat3(&WorkingVertices[VKeep].Normal, XMVector3Normalize(XMVectorAdd(N1, N2)));

        VertexRemoved[VRemove] = true;
        Quadrics[VKeep] = Quadrics[VKeep] + Quadrics[VRemove];

        auto& TrisToUpdate = It1->second;
        for (uint32 TriIdx : TrisToUpdate)
        {
            if (Triangles[TriIdx].bIsValid)
            {
                Triangles[TriIdx].bIsValid = false;
                CurrentTriCount--;
            }
        }

        for (uint32 TriIdx : TrisToUpdate)
        {
            FTriangle& Tri = Triangles[TriIdx];
            if (Tri.V0 == VRemove) Tri.V0 = VKeep;
            if (Tri.V1 == VRemove) Tri.V1 = VKeep;
            if (Tri.V2 == VRemove) Tri.V2 = VKeep;
            
            if (Tri.V0 == Tri.V1 || Tri.V1 == Tri.V2 || Tri.V0 == Tri.V2)
            {
                continue;
            }

            XMVECTOR TV0 = XMLoadFloat3(&WorkingVertices[Tri.V0].Position);
            XMVECTOR TV1 = XMLoadFloat3(&WorkingVertices[Tri.V1].Position);
            XMVECTOR TV2 = XMLoadFloat3(&WorkingVertices[Tri.V2].Position);
            
            XMVECTOR E1 = XMVectorSubtract(TV1, TV0);
            XMVECTOR E2 = XMVectorSubtract(TV2, TV0);
            XMVECTOR NewNormal = XMVector3Cross(E1, E2);
            
            float Area;
            XMStoreFloat(&Area, XMVector3Length(NewNormal));
            
            if (Area > 0.0001f)
            {
                Tri.bIsValid = true;
                XMStoreFloat3(&Tri.Normal, XMVector3Normalize(NewNormal));
                CurrentTriCount++;

                FEdge Edges[3] = {
                    { Tri.V0, Tri.V1 },
                    { Tri.V1, Tri.V2 },
                    { Tri.V2, Tri.V0 }
                };
                
                for (int j = 0; j < 3; ++j)
                {
                    if (!VertexRemoved[Edges[j].V0] && !VertexRemoved[Edges[j].V1])
                    {
                        auto [Cost, NewPos] = ComputeCost(Edges[j].V0, Edges[j].V1);
                        FEdgeCollapse NewCollapse;
                        NewCollapse.V1 = Edges[j].V0;
                        NewCollapse.V2 = Edges[j].V1;
                        NewCollapse.Cost = Cost;
                        NewCollapse.NewPosition = NewPos;
                        EdgeQueue.push(NewCollapse);
                    }
                }
            }
        }
    }

    std::vector<FVertex> ResultVertices;
    std::vector<uint32> VertexMap(WorkingVertices.size(), UINT32_MAX);
    uint32 NewVertexIndex = 0;

    for (size_t i = 0; i < WorkingVertices.size(); ++i)
    {
        if (!VertexRemoved[i])
        {
            ResultVertices.push_back(WorkingVertices[i]);
            VertexMap[i] = NewVertexIndex++;
        }
    }

    return ResultVertices;
}

bool KLODGenerator::GenerateLODs(
    KStaticMesh* StaticMesh,
    uint32 LODCount,
    const FLODGenerationParams& BaseParams)
{
    if (!StaticMesh || !StaticMesh->IsValid())
    {
        LOG_ERROR("Invalid static mesh for LOD generation");
        return false;
    }

    if (LODCount < 2 || LODCount > MAX_MESH_LODS)
    {
        LOG_ERROR("LODGenerator: Invalid LOD count (must be 2-MAX_MESH_LODS)");
        return false;
    }

    FMeshLOD* BaseLOD = StaticMesh->GetLOD(0);
    if (!BaseLOD)
    {
        LOG_ERROR("Base LOD not found");
        return false;
    }

    std::vector<FVertex> CurrentVertices = BaseLOD->Vertices;
    std::vector<uint32> CurrentIndices = BaseLOD->Indices;
    std::vector<FVertex> SimplifiedVertices = CurrentVertices;

    float ReductionFactor = BaseParams.ReductionFactor;
    float ScreenSize = 1.0f;
    float ScreenSizeStep = 1.0f / static_cast<float>(LODCount);

    for (uint32 LOD = 1; LOD < LODCount; ++LOD)
    {
        ScreenSize -= ScreenSizeStep;
        
        FLODGenerationParams LODParams = BaseParams;
        LODParams.ReductionFactor = ReductionFactor;
        
        std::vector<FVertex> NewVertices = SimplifyMesh(
            SimplifiedVertices, CurrentIndices, LODParams);
        
        if (NewVertices.empty() || NewVertices.size() < 4)
        {
            LOG_WARNING("LODGenerator: LOD generation produced too few vertices, stopping");
            break;
        }

        float ThisLODScreenSize = ScreenSize;
        StaticMesh->AddLOD(NewVertices, CurrentIndices, ThisLODScreenSize);
        
        SimplifiedVertices = NewVertices;
        ReductionFactor *= BaseParams.ReductionFactor;
    }

    return true;
}

std::vector<FMeshLOD> KLODGenerator::GenerateLODChain(
    const std::vector<FVertex>& BaseVertices,
    const std::vector<uint32>& BaseIndices,
    uint32 LODCount,
    float ReductionFactor)
{
    std::vector<FMeshLOD> LODs;
    
    if (BaseVertices.empty() || BaseIndices.empty())
    {
        return LODs;
    }

    FMeshLOD BaseLOD;
    BaseLOD.Vertices = BaseVertices;
    BaseLOD.Indices = BaseIndices;
    BaseLOD.ScreenSize = 1.0f;
    LODs.push_back(BaseLOD);

    std::vector<FVertex> CurrentVertices = BaseVertices;
    std::vector<uint32> CurrentIndices = BaseIndices;
    float CurrentReduction = ReductionFactor;

    for (uint32 i = 1; i < LODCount; ++i)
    {
        FLODGenerationParams Params;
        Params.ReductionFactor = CurrentReduction;

        std::vector<FVertex> Simplified = SimplifyMesh(CurrentVertices, CurrentIndices, Params);
        
        if (Simplified.empty() || Simplified.size() < 4)
        {
            break;
        }

        FMeshLOD NewLOD;
        NewLOD.Vertices = Simplified;
        NewLOD.Indices = CurrentIndices;
        NewLOD.ScreenSize = 1.0f - (static_cast<float>(i) / static_cast<float>(LODCount));
        LODs.push_back(NewLOD);

        CurrentVertices = Simplified;
        CurrentReduction *= ReductionFactor;
    }

    return LODs;
}

void KLODGenerator::ComputeQuadrics(
    const std::vector<FVertex>& Vertices,
    const std::vector<uint32>& Indices,
    std::vector<FQuadricMatrix>& OutQuadrics)
{
    OutQuadrics.assign(Vertices.size(), FQuadricMatrix());

    for (size_t i = 0; i < Indices.size(); i += 3)
    {
        uint32 I0 = Indices[i];
        uint32 I1 = Indices[i + 1];
        uint32 I2 = Indices[i + 2];

        const XMFLOAT3& P0 = Vertices[I0].Position;
        const XMFLOAT3& P1 = Vertices[I1].Position;
        const XMFLOAT3& P2 = Vertices[I2].Position;

        XMVECTOR V0 = XMLoadFloat3(&P0);
        XMVECTOR V1 = XMLoadFloat3(&P1);
        XMVECTOR V2 = XMLoadFloat3(&P2);

        XMVECTOR Edge1 = XMVectorSubtract(V1, V0);
        XMVECTOR Edge2 = XMVectorSubtract(V2, V0);
        XMVECTOR Normal = XMVector3Cross(Edge1, Edge2);
        Normal = XMVector3Normalize(Normal);

        float a, b, c, d;
        XMVECTOR N = Normal;
        XMFLOAT3 NormalFloat3;
        XMStoreFloat3(&NormalFloat3, N);
        a = NormalFloat3.x;
        b = NormalFloat3.y;
        c = NormalFloat3.z;
        d = -(a * P0.x + b * P0.y + c * P0.z);

        FQuadricMatrix Q(a, b, c, d);
        OutQuadrics[I0] += Q;
        OutQuadrics[I1] += Q;
        OutQuadrics[I2] += Q;
    }
}

void KLODGenerator::BuildEdgeList(
    const std::vector<uint32>& Indices,
    std::unordered_map<FEdge, std::vector<uint32>, FEdgeHash>& EdgeTriangles)
{
    for (size_t i = 0; i < Indices.size(); i += 3)
    {
        uint32 TriIdx = static_cast<uint32>(i / 3);
        
        FEdge Edges[3] = {
            { Indices[i], Indices[i + 1] },
            { Indices[i + 1], Indices[i + 2] },
            { Indices[i + 2], Indices[i] }
        };

        for (int j = 0; j < 3; ++j)
        {
            EdgeTriangles[Edges[j]].push_back(TriIdx);
        }
    }
}

float KLODGenerator::ComputeEdgeCollapseCost(
    const XMFLOAT3& P1, const XMFLOAT3& P2,
    const FQuadricMatrix& Q1, const FQuadricMatrix& Q2,
    XMFLOAT3& OutNewPosition)
{
    XMVECTOR V1 = XMLoadFloat3(&P1);
    XMVECTOR V2 = XMLoadFloat3(&P2);
    XMStoreFloat3(&OutNewPosition, XMVectorScale(XMVectorAdd(V1, V2), 0.5f));

    FQuadricMatrix Combined = Q1 + Q2;
    return Combined.Evaluate(OutNewPosition);
}

void KLODGenerator::CollapseEdge(
    std::vector<FVertex>& Vertices,
    std::vector<FTriangle>& Triangles,
    uint32 VKeep, uint32 VRemove,
    const XMFLOAT3& NewPosition)
{
    Vertices[VKeep].Position = NewPosition;

    XMVECTOR N1 = XMLoadFloat3(&Vertices[VKeep].Normal);
    XMVECTOR N2 = XMLoadFloat3(&Vertices[VRemove].Normal);
    XMStoreFloat3(&Vertices[VKeep].Normal, XMVector3Normalize(XMVectorAdd(N1, N2)));

    for (auto& Tri : Triangles)
    {
        if (!Tri.bIsValid) continue;

        if (Tri.V0 == VRemove) Tri.V0 = VKeep;
        else if (Tri.V1 == VRemove) Tri.V1 = VKeep;
        else if (Tri.V2 == VRemove) Tri.V2 = VKeep;

        if (Tri.V0 == Tri.V1 || Tri.V1 == Tri.V2 || Tri.V0 == Tri.V2)
        {
            Tri.bIsValid = false;
        }
    }
}
