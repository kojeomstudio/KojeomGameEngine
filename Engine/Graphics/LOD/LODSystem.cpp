#include "LODSystem.h"
#include <algorithm>
#include <cmath>

KLODSystem::KLODSystem()
    : bInitialized(false)
{
}

void KLODSystem::Initialize(const FLODSystemParams& InParams)
{
    Params = InParams;
    
    LODLevels.clear();
    LODLevels.reserve(Params.MaxLODLevels);

    float ScreenCoverageStep = 1.0f / static_cast<float>(Params.MaxLODLevels);
    float CurrentScreenCoverage = 1.0f;

    for (uint32 i = 0; i < Params.MaxLODLevels; ++i)
    {
        FLODLevel Level;
        Level.LODIndex = i;
        Level.ScreenCoverageMin = CurrentScreenCoverage - ScreenCoverageStep;
        Level.ScreenCoverageMax = CurrentScreenCoverage;
        Level.TransitionDistance = (i < Params.LODDistances.size()) 
            ? Params.LODDistances[i] 
            : 100.0f * (i + 1);
        Level.BlendDuration = Params.BlendDuration;
        
        LODLevels.push_back(Level);
        CurrentScreenCoverage -= ScreenCoverageStep;
    }

    bInitialized = true;
    LOG_INFO("LODSystem initialized with 4 LOD levels");
}

void KLODSystem::Update(float DeltaTime)
{
    for (auto& Pair : BlendStates)
    {
        FLODBlendState& State = Pair.second;
        
        if (State.bIsBlending)
        {
            State.BlendTime += DeltaTime;
            float TotalBlendTime = Params.BlendDuration;
            
            if (State.BlendTime >= TotalBlendTime)
            {
                State.CurrentLOD = State.TargetLOD;
                State.BlendFactor = 1.0f;
                State.bIsBlending = false;
            }
            else
            {
                State.BlendFactor = SmoothStep(0.0f, TotalBlendTime, State.BlendTime);
            }
        }
    }
}

uint32 KLODSystem::SelectLOD(
    const FBoundingSphere& Bounds,
    const XMFLOAT3& ObjectPosition,
    const XMMATRIX& WorldMatrix,
    const XMMATRIX& ViewProjectionMatrix,
    float ScreenWidth, float ScreenHeight)
{
    if (Params.bUseScreenCoverage)
    {
        float ScreenCoverage = CalculateScreenCoverage(
            Bounds, ObjectPosition, ViewProjectionMatrix, ScreenWidth, ScreenHeight);
        return CalculateLODFromScreenCoverage(ScreenCoverage);
    }
    else
    {
        return CalculateLODFromScreenCoverage(1.0f);
    }
}

uint32 KLODSystem::SelectLODByDistance(
    const XMFLOAT3& ObjectPosition,
    const XMFLOAT3& CameraPosition)
{
    XMVECTOR ObjPos = XMLoadFloat3(&ObjectPosition);
    XMVECTOR CamPos = XMLoadFloat3(&CameraPosition);
    float Distance;
    XMStoreFloat(&Distance, XMVector3Length(XMVectorSubtract(ObjPos, CamPos)));
    
    return CalculateLODFromDistance(Distance);
}

FLODBlendState KLODSystem::GetBlendState(uint32 ObjectID) const
{
    auto It = BlendStates.find(ObjectID);
    if (It != BlendStates.end())
    {
        return It->second;
    }
    
    FLODBlendState DefaultState;
    DefaultState.CurrentLOD = 0;
    DefaultState.TargetLOD = 0;
    DefaultState.BlendFactor = 1.0f;
    DefaultState.BlendTime = 0.0f;
    DefaultState.bIsBlending = false;
    return DefaultState;
}

void KLODSystem::UpdateBlendState(uint32 ObjectID, uint32 TargetLOD, float DeltaTime)
{
    auto& State = BlendStates[ObjectID];
    
    if (TargetLOD != State.CurrentLOD)
    {
        if (Params.bEnableLODBlending)
        {
            State.TargetLOD = TargetLOD;
            State.BlendTime = 0.0f;
            State.BlendFactor = 0.0f;
            State.bIsBlending = true;
            
            if (OnLODSelection)
            {
                OnLODSelection(State.CurrentLOD, TargetLOD);
            }
        }
        else
        {
            State.CurrentLOD = TargetLOD;
            State.TargetLOD = TargetLOD;
            State.BlendFactor = 1.0f;
            State.bIsBlending = false;
        }
    }
}

void KLODSystem::SetLODDistance(uint32 LODIndex, float Distance)
{
    if (LODIndex < LODLevels.size())
    {
        LODLevels[LODIndex].TransitionDistance = Distance;
    }
}

float KLODSystem::GetLODDistance(uint32 LODIndex) const
{
    if (LODIndex < LODLevels.size())
    {
        return LODLevels[LODIndex].TransitionDistance;
    }
    return FLT_MAX;
}

void KLODSystem::RegisterObject(uint32 ObjectID)
{
    FLODBlendState State;
    State.CurrentLOD = 0;
    State.TargetLOD = 0;
    State.BlendFactor = 1.0f;
    State.BlendTime = 0.0f;
    State.bIsBlending = false;
    
    BlendStates[ObjectID] = State;
}

void KLODSystem::UnregisterObject(uint32 ObjectID)
{
    BlendStates.erase(ObjectID);
}

void KLODSystem::ClearAllObjects()
{
    BlendStates.clear();
}

float KLODSystem::CalculateScreenCoverage(
    const FBoundingSphere& Bounds,
    const XMFLOAT3& CameraPosition,
    const XMMATRIX& ViewProjectionMatrix,
    float ScreenWidth, float ScreenHeight)
{
    XMVECTOR Center = XMLoadFloat3(&Bounds.Center);
    float Radius = Bounds.Radius;

    XMVECTOR Projected = XMVector3Project(
        Center,
        0.0f, 0.0f,
        ScreenWidth, ScreenHeight,
        0.0f, 1.0f,
        XMMatrixIdentity(),
        ViewProjectionMatrix,
        XMMatrixIdentity());

    XMFLOAT3 ProjectedPos;
    XMStoreFloat3(&ProjectedPos, Projected);

    float ProjectedRadius = Radius * ScreenHeight / (2.0f * Radius);
    
    float Coverage = (ProjectedRadius * 2.0f) / ScreenHeight;
    Coverage = std::max(0.0f, std::min(1.0f, Coverage));
    
    return Coverage;
}

uint32 KLODSystem::CalculateLODFromScreenCoverage(float ScreenCoverage) const
{
    for (size_t i = 0; i < LODLevels.size(); ++i)
    {
        if (ScreenCoverage >= LODLevels[i].ScreenCoverageMin &&
            ScreenCoverage < LODLevels[i].ScreenCoverageMax)
        {
            return static_cast<uint32>(i);
        }
    }
    
    return static_cast<uint32>(LODLevels.size() - 1);
}

uint32 KLODSystem::CalculateLODFromDistance(float Distance) const
{
    for (size_t i = 0; i < LODLevels.size(); ++i)
    {
        if (Distance < LODLevels[i].TransitionDistance * Params.LODBias)
        {
            return static_cast<uint32>(i);
        }
    }
    
    return static_cast<uint32>(LODLevels.size() - 1);
}

float KLODSystem::SmoothStep(float Edge0, float Edge1, float X) const
{
    float T = (X - Edge0) / (Edge1 - Edge0);
    T = std::max(0.0f, std::min(1.0f, T));
    return T * T * (3.0f - 2.0f * T);
}
