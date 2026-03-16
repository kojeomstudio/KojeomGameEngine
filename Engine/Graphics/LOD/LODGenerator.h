#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Assets/StaticMesh.h"
#include <vector>
#include <unordered_map>
#include <memory>

struct FEdgeCollapse
{
    uint32 V1;
    uint32 V2;
    float Cost;
    XMFLOAT3 NewPosition;

    bool operator<(const FEdgeCollapse& Other) const
    {
        return Cost > Other.Cost;
    }
};

struct FQuadricMatrix
{
    float M[10];

    FQuadricMatrix()
    {
        for (int i = 0; i < 10; ++i)
            M[i] = 0.0f;
    }

    FQuadricMatrix(float a, float b, float c, float d)
    {
        M[0] = a * a; M[1] = a * b; M[2] = a * c; M[3] = a * d;
        M[4] = b * b; M[5] = b * c; M[6] = b * d;
        M[7] = c * c; M[8] = c * d;
        M[9] = d * d;
    }

    FQuadricMatrix operator+(const FQuadricMatrix& Other) const
    {
        FQuadricMatrix Result;
        for (int i = 0; i < 10; ++i)
            Result.M[i] = M[i] + Other.M[i];
        return Result;
    }

    FQuadricMatrix& operator+=(const FQuadricMatrix& Other)
    {
        for (int i = 0; i < 10; ++i)
            M[i] += Other.M[i];
        return *this;
    }

    float Evaluate(const XMFLOAT3& Pos) const
    {
        float x = Pos.x, y = Pos.y, z = Pos.z;
        return M[0] * x * x + 2 * M[1] * x * y + 2 * M[2] * x * z + 2 * M[3] * x
             + M[4] * y * y + 2 * M[5] * y * z + 2 * M[6] * y
             + M[7] * z * z + 2 * M[8] * z
             + M[9];
    }
};

struct FLODGenerationParams
{
    float ReductionFactor = 0.5f;
    uint32 TargetTriangleCount = 0;
    float MaxEdgeLength = FLT_MAX;
    bool bPreserveBoundary = true;
    bool bPreserveUVSeams = true;
    float BoundaryWeight = 1000.0f;
};

class KLODGenerator
{
public:
    KLODGenerator() = default;
    ~KLODGenerator() = default;

    KLODGenerator(const KLODGenerator&) = delete;
    KLODGenerator& operator=(const KLODGenerator&) = delete;

    static std::vector<FVertex> SimplifyMesh(
        const std::vector<FVertex>& InVertices,
        const std::vector<uint32>& InIndices,
        const FLODGenerationParams& Params);

    static bool GenerateLODs(
        KStaticMesh* StaticMesh,
        uint32 LODCount,
        const FLODGenerationParams& BaseParams);

    static std::vector<FMeshLOD> GenerateLODChain(
        const std::vector<FVertex>& BaseVertices,
        const std::vector<uint32>& BaseIndices,
        uint32 LODCount,
        float ReductionFactor = 0.5f);

private:
    struct FEdge
    {
        uint32 V0, V1;
        bool operator==(const FEdge& Other) const
        {
            return (V0 == Other.V0 && V1 == Other.V1) ||
                   (V0 == Other.V1 && V1 == Other.V0);
        }
    };

    struct FEdgeHash
    {
        size_t operator()(const FEdge& Edge) const
        {
            uint32 MinV = std::min(Edge.V0, Edge.V1);
            uint32 MaxV = std::max(Edge.V0, Edge.V1);
            return std::hash<uint64>()((uint64)MinV << 32 | MaxV);
        }
    };

    struct FTriangle
    {
        uint32 V0, V1, V2;
        XMFLOAT3 Normal;
        bool bIsValid = true;
    };

    static void ComputeQuadrics(
        const std::vector<FVertex>& Vertices,
        const std::vector<uint32>& Indices,
        std::vector<FQuadricMatrix>& OutQuadrics);

    static void BuildEdgeList(
        const std::vector<uint32>& Indices,
        std::unordered_map<FEdge, std::vector<uint32>, FEdgeHash>& EdgeTriangles);

    static float ComputeEdgeCollapseCost(
        const XMFLOAT3& P1, const XMFLOAT3& P2,
        const FQuadricMatrix& Q1, const FQuadricMatrix& Q2,
        XMFLOAT3& OutNewPosition);

    static void CollapseEdge(
        std::vector<FVertex>& Vertices,
        std::vector<FTriangle>& Triangles,
        uint32 VKeep, uint32 VRemove,
        const XMFLOAT3& NewPosition);
};
