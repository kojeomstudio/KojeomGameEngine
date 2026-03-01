#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Utils/Math.h"
#include <string>
#include <unordered_map>

struct FOcclusionQueryResult
{
    UINT64 PixelsVisible;
    bool WasVisible;
    bool QueryFinished;
};

class KOcclusionQuery
{
public:
    KOcclusionQuery() = default;
    ~KOcclusionQuery() = default;

    KOcclusionQuery(const KOcclusionQuery&) = delete;
    KOcclusionQuery& operator=(const KOcclusionQuery&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 MaxQueries = 256);
    void Cleanup();

    INT32 BeginQuery(ID3D11DeviceContext* Context);
    void EndQuery(ID3D11DeviceContext* Context, INT32 QueryIndex);

    FOcclusionQueryResult GetQueryResult(ID3D11DeviceContext* Context, INT32 QueryIndex, bool Flush = false);

    void ResetFrame();
    INT32 GetActiveQueryCount() const { return ActiveQueryCount; }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateQueries(ID3D11Device* Device);

private:
    ComPtr<ID3D11Query> OcclusionQuery;
    ComPtr<ID3D11Query> DisjointQuery;
    ComPtr<ID3D11Predicate> OcclusionPredicate;

    std::vector<ComPtr<ID3D11Query>> QueryPool;
    std::vector<ComPtr<ID3D11Predicate>> PredicatePool;
    std::vector<bool> QueryInUse;
    std::vector<FOcclusionQueryResult> QueryResults;

    UINT32 MaxQueries = 256;
    INT32 ActiveQueryCount = 0;
    bool bInitialized = false;
};

struct FOccluderVolume
{
    XMMATRIX WorldMatrix;
    FBoundingBox Bounds;
    bool bIsOccluder;
};

class KOcclusionCuller
{
public:
    KOcclusionCuller() = default;
    ~KOcclusionCuller() = default;

    KOcclusionCuller(const KOcclusionCuller&) = delete;
    KOcclusionCuller& operator=(const KOcclusionCuller&) = delete;

    HRESULT Initialize(ID3D11Device* Device);
    void Cleanup();

    void BeginFrame(ID3D11DeviceContext* Context);
    void EndFrame(ID3D11DeviceContext* Context);

    INT32 BeginOcclusionQuery(ID3D11DeviceContext* Context, const std::string& Name);
    void EndOcclusionQuery(ID3D11DeviceContext* Context, const std::string& Name);

    bool IsVisible(ID3D11DeviceContext* Context, const std::string& Name);

    void SetOccluder(const std::string& Name, const FOccluderVolume& Occluder);
    void RemoveOccluder(const std::string& Name);
    void ClearOccluders();

    void SetThresholdPixels(UINT64 Threshold) { ThresholdPixels = Threshold; }
    UINT64 GetThresholdPixels() const { return ThresholdPixels; }

    void SetEnabled(bool bEnabled) { bEnabled = bEnabled; }
    bool IsEnabled() const { return bEnabled; }

    KOcclusionQuery* GetQuerySystem() { return &QuerySystem; }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateDebugMesh(ID3D11Device* Device);

private:
    KOcclusionQuery QuerySystem;

    std::unordered_map<std::string, INT32> NamedQueries;
    std::unordered_map<std::string, FOccluderVolume> Occluders;
    std::unordered_map<std::string, bool> VisibilityCache;

    UINT64 ThresholdPixels = 1;
    bool bEnabled = true;
    bool bInitialized = false;
};
