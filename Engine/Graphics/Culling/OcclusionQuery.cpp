#include "OcclusionQuery.h"
#include <sstream>

HRESULT KOcclusionQuery::Initialize(ID3D11Device* Device, UINT32 InMaxQueries)
{
    if (bInitialized)
    {
        return S_OK;
    }

    if (!Device)
    {
        LOG_ERROR("OcclusionQuery: Invalid device");
        return E_INVALIDARG;
    }

    MaxQueries = InMaxQueries;

    HRESULT hr = CreateQueries(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("OcclusionQuery: Failed to create queries");
        return hr;
    }

    bInitialized = true;
    std::ostringstream oss;
    oss << "OcclusionQuery: Initialized with " << MaxQueries << " queries";
    LOG_INFO(oss.str().c_str());
    return S_OK;
}

void KOcclusionQuery::Cleanup()
{
    QueryPool.clear();
    PredicatePool.clear();
    QueryInUse.clear();
    QueryResults.clear();
    OcclusionQuery.Reset();
    DisjointQuery.Reset();
    OcclusionPredicate.Reset();
    bInitialized = false;
}

HRESULT KOcclusionQuery::CreateQueries(ID3D11Device* Device)
{
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_OCCLUSION;
    queryDesc.MiscFlags = 0;

    QueryPool.resize(MaxQueries);
    for (UINT32 i = 0; i < MaxQueries; ++i)
    {
        HRESULT hr = Device->CreateQuery(&queryDesc, &QueryPool[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("OcclusionQuery: Failed to create occlusion query");
            return hr;
        }
    }

    D3D11_QUERY_DESC predicateDesc = {};
    predicateDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
    predicateDesc.MiscFlags = 0;

    PredicatePool.resize(MaxQueries);
    for (UINT32 i = 0; i < MaxQueries; ++i)
    {
        HRESULT hr = Device->CreatePredicate(&predicateDesc, &PredicatePool[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("OcclusionQuery: Failed to create occlusion predicate");
            return hr;
        }
    }

    QueryInUse.resize(MaxQueries, false);
    QueryResults.resize(MaxQueries);

    return S_OK;
}

INT32 KOcclusionQuery::BeginQuery(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !Context)
    {
        return -1;
    }

    for (UINT32 i = 0; i < MaxQueries; ++i)
    {
        if (!QueryInUse[i])
        {
            QueryInUse[i] = true;
            QueryResults[i].QueryFinished = false;
            QueryResults[i].PixelsVisible = 0;
            QueryResults[i].WasVisible = false;
            Context->Begin(QueryPool[i].Get());
            ActiveQueryCount++;
            return static_cast<INT32>(i);
        }
    }

    return -1;
}

void KOcclusionQuery::EndQuery(ID3D11DeviceContext* Context, INT32 QueryIndex)
{
    if (!bInitialized || !Context || QueryIndex < 0 || QueryIndex >= static_cast<INT32>(MaxQueries))
    {
        return;
    }

    Context->End(QueryPool[QueryIndex].Get());
    QueryResults[QueryIndex].QueryFinished = true;
}

FOcclusionQueryResult KOcclusionQuery::GetQueryResult(ID3D11DeviceContext* Context, INT32 QueryIndex, bool Flush)
{
    FOcclusionQueryResult result = {};

    if (!bInitialized || !Context || QueryIndex < 0 || QueryIndex >= static_cast<INT32>(MaxQueries))
    {
        return result;
    }

    if (!QueryResults[QueryIndex].QueryFinished)
    {
        return result;
    }

    UINT64 pixelsVisible = 0;
    HRESULT hr = Context->GetData(QueryPool[QueryIndex].Get(), &pixelsVisible, sizeof(UINT64), 
                                   Flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);

    if (hr == S_OK)
    {
        result.PixelsVisible = pixelsVisible;
        result.WasVisible = pixelsVisible > 0;
        result.QueryFinished = true;
        QueryInUse[QueryIndex] = false;
        QueryResults[QueryIndex] = result;
        ActiveQueryCount--;
    }
    else if (hr == S_FALSE)
    {
        result = QueryResults[QueryIndex];
    }

    return result;
}

void KOcclusionQuery::ResetFrame()
{
    for (UINT32 i = 0; i < MaxQueries; ++i)
    {
        QueryInUse[i] = false;
        QueryResults[i].QueryFinished = false;
    }
    ActiveQueryCount = 0;
}

HRESULT KOcclusionCuller::Initialize(ID3D11Device* Device)
{
    if (bInitialized)
    {
        return S_OK;
    }

    if (!Device)
    {
        LOG_ERROR("OcclusionCuller: Invalid device");
        return E_INVALIDARG;
    }

    HRESULT hr = QuerySystem.Initialize(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("OcclusionCuller: Failed to initialize query system");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("OcclusionCuller: Initialized");
    return S_OK;
}

void KOcclusionCuller::Cleanup()
{
    QuerySystem.Cleanup();
    NamedQueries.clear();
    Occluders.clear();
    VisibilityCache.clear();
    bInitialized = false;
}

void KOcclusionCuller::BeginFrame(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !bEnabled)
    {
        return;
    }

    QuerySystem.ResetFrame();
    NamedQueries.clear();
    VisibilityCache.clear();
}

void KOcclusionCuller::EndFrame(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !bEnabled || !Context)
    {
        return;
    }

    for (auto& pair : NamedQueries)
    {
        auto result = QuerySystem.GetQueryResult(Context, pair.second, false);
        if (result.QueryFinished)
        {
            VisibilityCache[pair.first] = result.WasVisible;
        }
    }
}

INT32 KOcclusionCuller::BeginOcclusionQuery(ID3D11DeviceContext* Context, const std::string& Name)
{
    if (!bInitialized || !bEnabled || !Context)
    {
        return -1;
    }

    INT32 queryIndex = QuerySystem.BeginQuery(Context);
    if (queryIndex >= 0)
    {
        NamedQueries[Name] = queryIndex;
    }

    return queryIndex;
}

void KOcclusionCuller::EndOcclusionQuery(ID3D11DeviceContext* Context, const std::string& Name)
{
    if (!bInitialized || !bEnabled || !Context)
    {
        return;
    }

    auto it = NamedQueries.find(Name);
    if (it != NamedQueries.end())
    {
        QuerySystem.EndQuery(Context, it->second);
    }
}

bool KOcclusionCuller::IsVisible(ID3D11DeviceContext* Context, const std::string& Name)
{
    if (!bInitialized || !bEnabled)
    {
        return true;
    }

    auto cacheIt = VisibilityCache.find(Name);
    if (cacheIt != VisibilityCache.end())
    {
        return cacheIt->second;
    }

    auto queryIt = NamedQueries.find(Name);
    if (queryIt != NamedQueries.end())
    {
        auto result = QuerySystem.GetQueryResult(Context, queryIt->second, false);
        return result.QueryFinished ? result.WasVisible : true;
    }

    return true;
}

void KOcclusionCuller::SetOccluder(const std::string& Name, const FOccluderVolume& Occluder)
{
    Occluders[Name] = Occluder;
}

void KOcclusionCuller::RemoveOccluder(const std::string& Name)
{
    Occluders.erase(Name);
}

void KOcclusionCuller::ClearOccluders()
{
    Occluders.clear();
}
