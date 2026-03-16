#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Assets/StaticMesh.h"
#include "../Camera.h"
#include <vector>
#include <memory>
#include <functional>

struct FLODLevel
{
    uint32 LODIndex;
    float ScreenCoverageMin;
    float ScreenCoverageMax;
    float TransitionDistance;
    float BlendDuration;
};

struct FLODBlendState
{
    uint32 CurrentLOD;
    uint32 TargetLOD;
    float BlendFactor;
    float BlendTime;
    bool bIsBlending;
};

struct FLODSystemParams
{
    uint32 MaxLODLevels = 4;
    float LODTransitionSpeed = 5.0f;
    float LODBias = 1.0f;
    bool bEnableLODBlending = true;
    float BlendDuration = 0.25f;
    bool bUseScreenCoverage = true;
    float BaseScreenSize = 1000.0f;
    std::vector<float> LODDistances = { 50.0f, 100.0f, 200.0f, 400.0f };
};

class KLODSystem
{
public:
    using LODSelectionCallback = std::function<void(uint32 OldLOD, uint32 NewLOD)>;

    KLODSystem();
    ~KLODSystem() = default;

    KLODSystem(const KLODSystem&) = delete;
    KLODSystem& operator=(const KLODSystem&) = delete;

    void Initialize(const FLODSystemParams& Params);
    void Update(float DeltaTime);

    uint32 SelectLOD(
        const FBoundingSphere& Bounds,
        const XMFLOAT3& ObjectPosition,
        const XMMATRIX& WorldMatrix,
        const XMMATRIX& ViewProjectionMatrix,
        float ScreenWidth, float ScreenHeight);

    uint32 SelectLODByDistance(
        const XMFLOAT3& ObjectPosition,
        const XMFLOAT3& CameraPosition);

    FLODBlendState GetBlendState(uint32 ObjectID) const;
    void UpdateBlendState(uint32 ObjectID, uint32 TargetLOD, float DeltaTime);

    void SetLODBias(float Bias) { Params.LODBias = Bias; }
    float GetLODBias() const { return Params.LODBias; }

    void SetTransitionSpeed(float Speed) { Params.LODTransitionSpeed = Speed; }
    float GetTransitionSpeed() const { return Params.LODTransitionSpeed; }

    void SetLODDistance(uint32 LODIndex, float Distance);
    float GetLODDistance(uint32 LODIndex) const;

    void EnableLODBlending(bool bEnable) { Params.bEnableLODBlending = bEnable; }
    bool IsLODBlendingEnabled() const { return Params.bEnableLODBlending; }

    void RegisterObject(uint32 ObjectID);
    void UnregisterObject(uint32 ObjectID);
    void ClearAllObjects();

    void SetLODSelectionCallback(LODSelectionCallback Callback) { OnLODSelection = Callback; }

    static float CalculateScreenCoverage(
        const FBoundingSphere& Bounds,
        const XMFLOAT3& CameraPosition,
        const XMMATRIX& ViewProjectionMatrix,
        float ScreenWidth, float ScreenHeight);

    const FLODSystemParams& GetParams() const { return Params; }

private:
    uint32 CalculateLODFromScreenCoverage(float ScreenCoverage) const;
    uint32 CalculateLODFromDistance(float Distance) const;
    float SmoothStep(float Edge0, float Edge1, float X) const;

private:
    FLODSystemParams Params;
    std::vector<FLODLevel> LODLevels;
    std::unordered_map<uint32, FLODBlendState> BlendStates;
    LODSelectionCallback OnLODSelection;
    bool bInitialized = false;
};
