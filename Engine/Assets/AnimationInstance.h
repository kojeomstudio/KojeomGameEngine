#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include "Skeleton.h"
#include "Animation.h"
#include <vector>
#include <memory>
#include <unordered_map>

enum class EAnimationPlayMode
{
    Once,
    Loop,
    PingPong
};

enum class EAnimationState
{
    Stopped,
    Playing,
    Paused
};

enum class ERootMotionMode
{
    NoRootMotion,
    RootMotionFromEverything,
    RootMotionFromRootBoneOnly
};

struct FRootMotionData
{
    XMFLOAT3 PositionDelta;
    XMFLOAT4 RotationDelta;
    bool bHasRootMotion;

    FRootMotionData()
        : PositionDelta(0, 0, 0)
        , RotationDelta(0, 0, 0, 1)
        , bHasRootMotion(false)
    {}
};

struct FAnimationInstanceState
{
    std::shared_ptr<KAnimation> Animation;
    float CurrentTime;
    float PlaybackSpeed;
    EAnimationPlayMode PlayMode;
    EAnimationState State;
    bool bReverse;

    FAnimationInstanceState()
        : CurrentTime(0.0f)
        , PlaybackSpeed(1.0f)
        , PlayMode(EAnimationPlayMode::Loop)
        , State(EAnimationState::Stopped)
        , bReverse(false)
    {}
};

class KAnimationInstance
{
public:
    KAnimationInstance();
    ~KAnimationInstance() = default;

    KAnimationInstance(const KAnimationInstance&) = delete;
    KAnimationInstance& operator=(const KAnimationInstance&) = delete;

    void SetSkeleton(KSkeleton* InSkeleton) { Skeleton = InSkeleton; }
    KSkeleton* GetSkeleton() const { return Skeleton; }

    void PlayAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim, 
                       EAnimationPlayMode Mode = EAnimationPlayMode::Loop);
    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

    void SetPlaybackSpeed(float Speed) { CurrentState.PlaybackSpeed = Speed; }
    float GetPlaybackSpeed() const { return CurrentState.PlaybackSpeed; }

    void SetPlayMode(EAnimationPlayMode Mode) { CurrentState.PlayMode = Mode; }
    EAnimationPlayMode GetPlayMode() const { return CurrentState.PlayMode; }

    void SetCurrentTime(float Time);
    float GetCurrentTime() const { return CurrentState.CurrentTime; }
    float GetNormalizedTime() const;

    EAnimationState GetState() const { return CurrentState.State; }
    bool IsPlaying() const { return CurrentState.State == EAnimationState::Playing; }

    void Update(float DeltaTime);

    const std::vector<XMMATRIX>& GetBoneMatrices() const { return BoneMatrices; }
    const std::vector<XMMATRIX>& GetFinalBoneMatrices() const { return FinalBoneMatrices; }
    const XMMATRIX* GetBoneMatrixData() const { return BoneMatrices.empty() ? nullptr : BoneMatrices.data(); }
    uint32 GetBoneMatrixCount() const { return static_cast<uint32>(BoneMatrices.size()); }

    void UpdateBoneMatrices();

    void SetBlendWeight(float Weight) { BlendWeight = Weight; }
    float GetBlendWeight() const { return BlendWeight; }

    const std::string& GetCurrentAnimationName() const { return CurrentAnimationName; }
    std::shared_ptr<KAnimation> GetCurrentAnimation() const { return CurrentState.Animation; }

    void AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim);
    void RemoveAnimation(const std::string& Name);
    std::shared_ptr<KAnimation> GetAnimation(const std::string& Name) const;

    void ClearAnimations();

    void SetRootMotionBoneIndex(int32 InBoneIndex) { RootMotionBoneIndex = InBoneIndex; }
    int32 GetRootMotionBoneIndex() const { return RootMotionBoneIndex; }

    void SetRootMotionMode(ERootMotionMode InMode) { RootMotionMode = InMode; }
    ERootMotionMode GetRootMotionMode() const { return RootMotionMode; }

    const FRootMotionData& ExtractRootMotion();
    void ResetRootMotion();

private:
    void EvaluateAnimation();
    void CalculateFinalBoneTransforms();

private:
    KSkeleton* Skeleton = nullptr;
    FAnimationInstanceState CurrentState;
    std::string CurrentAnimationName;
    std::vector<XMMATRIX> BoneMatrices;
    std::vector<XMMATRIX> FinalBoneMatrices;
    float BlendWeight = 1.0f;

    std::unordered_map<std::string, std::shared_ptr<KAnimation>> Animations;

    int32 RootMotionBoneIndex = 0;
    ERootMotionMode RootMotionMode = ERootMotionMode::NoRootMotion;
    FRootMotionData LastRootMotion;
    XMFLOAT3 PreviousRootPosition;
    XMFLOAT4 PreviousRootRotation;
    bool bRootMotionInitialized = false;
};
