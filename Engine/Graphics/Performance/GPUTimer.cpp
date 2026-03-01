#include "GPUTimer.h"

HRESULT KGPUTimer::Initialize(ID3D11Device* Device, UINT32 InMaxTimers)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for GPU timer");
        return E_INVALIDARG;
    }

    MaxTimers = InMaxTimers;

    D3D11_QUERY_DESC disjointDesc = {};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    disjointDesc.MiscFlags = 0;

    HRESULT hr = Device->CreateQuery(&disjointDesc, &FrameDisjointQuery);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create frame disjoint query");
        return hr;
    }

    D3D11_QUERY_DESC timestampDesc = {};
    timestampDesc.Query = D3D11_QUERY_TIMESTAMP;
    timestampDesc.MiscFlags = 0;

    hr = Device->CreateQuery(&timestampDesc, &FrameStartQuery);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create frame start query");
        return hr;
    }

    hr = Device->CreateQuery(&timestampDesc, &FrameEndQuery);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create frame end query");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("GPU timer initialized");
    return S_OK;
}

void KGPUTimer::Cleanup()
{
    Timers.clear();
    PendingTimers.clear();
    FrameDisjointQuery.Reset();
    FrameStartQuery.Reset();
    FrameEndQuery.Reset();
    bInitialized = false;
}

HRESULT KGPUTimer::CreateTimer(const std::string& Name)
{
    if (!bInitialized)
    {
        return E_FAIL;
    }

    if (Timers.size() >= MaxTimers)
    {
        LOG_WARNING("Maximum GPU timers reached");
        return E_FAIL;
    }

    if (Timers.find(Name) != Timers.end())
    {
        LOG_WARNING("Timer already exists");
        return S_FALSE;
    }

    Timers[Name] = FGPUTimerQuery();
    Timers[Name].Name = Name;

    return S_OK;
}

void KGPUTimer::DestroyTimer(const std::string& Name)
{
    auto it = Timers.find(Name);
    if (it != Timers.end())
    {
        Timers.erase(it);
    }
}

HRESULT KGPUTimer::CreateQueriesForTimer(ID3D11Device* Device, FGPUTimerQuery& Timer)
{
    D3D11_QUERY_DESC disjointDesc = {};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    HRESULT hr = Device->CreateQuery(&disjointDesc, &Timer.DisjointQuery);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_QUERY_DESC timestampDesc = {};
    timestampDesc.Query = D3D11_QUERY_TIMESTAMP;

    hr = Device->CreateQuery(&timestampDesc, &Timer.StartQuery);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = Device->CreateQuery(&timestampDesc, &Timer.EndQuery);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

void KGPUTimer::BeginFrame(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !Context)
    {
        return;
    }

    bFrameStarted = true;
    FrameStats.TimerResults.clear();

    if (FrameDisjointQuery && bDisjointQueryIssued)
    {
        UINT64 startTime = 0, endTime = 0;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;

        if (SUCCEEDED(Context->GetData(FrameStartQuery.Get(), &startTime, sizeof(startTime), 0)) &&
            SUCCEEDED(Context->GetData(FrameEndQuery.Get(), &endTime, sizeof(endTime), 0)) &&
            SUCCEEDED(Context->GetData(FrameDisjointQuery.Get(), &disjointData, sizeof(disjointData), 0)))
        {
            if (!disjointData.Disjoint)
            {
                UINT64 delta = endTime - startTime;
                float frequency = static_cast<float>(disjointData.Frequency);
                FrameStats.FrameTimeMs = (static_cast<float>(delta) / frequency) * 1000.0f;
            }
        }
    }

    Context->Begin(FrameDisjointQuery.Get());
    Context->End(FrameStartQuery.Get());
    bDisjointQueryIssued = true;

    for (auto& pair : Timers)
    {
        if (pair.second.bResultReady)
        {
            TryGetTimerResult(Context, pair.second);
            if (pair.second.bResultReady)
            {
                FrameStats.TimerResults[pair.first] = pair.second.LastTimeMs;
            }
        }
    }
}

void KGPUTimer::EndFrame(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !Context || !bFrameStarted)
    {
        return;
    }

    Context->End(FrameEndQuery.Get());
    Context->End(FrameDisjointQuery.Get());

    FrameStats.FrameCount++;
    bFrameStarted = false;
}

void KGPUTimer::StartTimer(ID3D11DeviceContext* Context, const std::string& Name)
{
    if (!bInitialized || !Context || !bFrameStarted)
    {
        return;
    }

    auto it = Timers.find(Name);
    if (it == Timers.end())
    {
        return;
    }

    FGPUTimerQuery& timer = it->second;

    if (!timer.DisjointQuery)
    {
        ID3D11Device* device = nullptr;
        Context->GetDevice(&device);
        if (device)
        {
            CreateQueriesForTimer(device, timer);
            device->Release();
        }
    }

    if (!timer.DisjointQuery)
    {
        return;
    }

    timer.bInProgress = true;
    timer.bResultReady = false;

    Context->Begin(timer.DisjointQuery.Get());
    Context->End(timer.StartQuery.Get());
}

void KGPUTimer::EndTimer(ID3D11DeviceContext* Context, const std::string& Name)
{
    if (!bInitialized || !Context || !bFrameStarted)
    {
        return;
    }

    auto it = Timers.find(Name);
    if (it == Timers.end())
    {
        return;
    }

    FGPUTimerQuery& timer = it->second;

    if (!timer.bInProgress || !timer.EndQuery)
    {
        return;
    }

    Context->End(timer.EndQuery.Get());
    Context->End(timer.DisjointQuery.Get());

    timer.bInProgress = false;
    timer.bResultReady = true;
}

bool KGPUTimer::TryGetTimerResult(ID3D11DeviceContext* Context, FGPUTimerQuery& Timer)
{
    if (!Timer.bResultReady || !Timer.DisjointQuery)
    {
        return false;
    }

    UINT64 startTime = 0, endTime = 0;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;

    HRESULT hrStart = Context->GetData(Timer.StartQuery.Get(), &startTime, sizeof(startTime), 0);
    HRESULT hrEnd = Context->GetData(Timer.EndQuery.Get(), &endTime, sizeof(endTime), 0);
    HRESULT hrDisjoint = Context->GetData(Timer.DisjointQuery.Get(), &disjointData, sizeof(disjointData), 0);

    if (hrStart == S_OK && hrEnd == S_OK && hrDisjoint == S_OK)
    {
        if (!disjointData.Disjoint)
        {
            UINT64 delta = endTime - startTime;
            float frequency = static_cast<float>(disjointData.Frequency);
            Timer.LastTimeMs = (static_cast<float>(delta) / frequency) * 1000.0f;
            Timer.bResultReady = false;
            return true;
        }
    }

    return false;
}

float KGPUTimer::GetTimerResult(const std::string& Name) const
{
    auto it = Timers.find(Name);
    if (it != Timers.end())
    {
        return it->second.LastTimeMs;
    }

    auto resultIt = FrameStats.TimerResults.find(Name);
    if (resultIt != FrameStats.TimerResults.end())
    {
        return resultIt->second;
    }

    return 0.0f;
}

void KGPUTimer::GetTimerNames(std::vector<std::string>& OutNames) const
{
    OutNames.clear();
    OutNames.reserve(Timers.size());
    for (const auto& pair : Timers)
    {
        OutNames.push_back(pair.first);
    }
}

bool KGPUTimer::HasTimer(const std::string& Name) const
{
    return Timers.find(Name) != Timers.end();
}
