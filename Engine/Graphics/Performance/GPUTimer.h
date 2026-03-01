#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include <string>
#include <vector>
#include <unordered_map>

struct FGPUTimerQuery
{
    ComPtr<ID3D11Query> DisjointQuery;
    ComPtr<ID3D11Query> StartQuery;
    ComPtr<ID3D11Query> EndQuery;
    std::string Name;
    float LastTimeMs;
    bool bInProgress;
    bool bResultReady;

    FGPUTimerQuery() : LastTimeMs(0.0f), bInProgress(false), bResultReady(false) {}
};

struct FFrameStats
{
    float GPUTimeMs = 0.0f;
    float FrameTimeMs = 0.0f;
    UINT64 FrameCount = 0;
    std::unordered_map<std::string, float> TimerResults;
};

class KGPUTimer
{
public:
    KGPUTimer() = default;
    ~KGPUTimer() = default;

    KGPUTimer(const KGPUTimer&) = delete;
    KGPUTimer& operator=(const KGPUTimer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 MaxTimers = 16);
    void Cleanup();

    HRESULT CreateTimer(const std::string& Name);
    void DestroyTimer(const std::string& Name);

    void BeginFrame(ID3D11DeviceContext* Context);
    void EndFrame(ID3D11DeviceContext* Context);

    void StartTimer(ID3D11DeviceContext* Context, const std::string& Name);
    void EndTimer(ID3D11DeviceContext* Context, const std::string& Name);

    float GetTimerResult(const std::string& Name) const;
    const FFrameStats& GetFrameStats() const { return FrameStats; }

    void GetTimerNames(std::vector<std::string>& OutNames) const;

    bool IsInitialized() const { return bInitialized; }
    bool HasTimer(const std::string& Name) const;

private:
    HRESULT CreateQueriesForTimer(ID3D11Device* Device, FGPUTimerQuery& Timer);
    bool TryGetTimerResult(ID3D11DeviceContext* Context, FGPUTimerQuery& Timer);

private:
    ComPtr<ID3D11Query> FrameDisjointQuery;
    ComPtr<ID3D11Query> FrameStartQuery;
    ComPtr<ID3D11Query> FrameEndQuery;

    std::unordered_map<std::string, FGPUTimerQuery> Timers;
    std::vector<std::string> PendingTimers;

    FFrameStats FrameStats;
    UINT32 MaxTimers = 16;
    bool bInitialized = false;
    bool bFrameStarted = false;
    bool bDisjointQueryIssued = false;
};
