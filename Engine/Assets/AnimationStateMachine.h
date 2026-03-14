#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include "Skeleton.h"
#include "Animation.h"
#include "AnimationInstance.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

enum class EAnimStateStatus
{
    Inactive,
    Active,
    TransitioningOut,
    TransitioningIn
};

struct FAnimNotify
{
    std::string Name;
    float TriggerTime;
    float Duration;
    std::function<void()> Callback;
    bool bTriggered;

    FAnimNotify()
        : TriggerTime(0.0f)
        , Duration(0.0f)
        , bTriggered(false)
    {}
};

struct FAnimTransitionCondition
{
    std::string ParameterName;
    enum class EComparisonType
    {
        Equals,
        NotEquals,
        Greater,
        GreaterOrEquals,
        Less,
        LessOrEquals
    } ComparisonType;

    float CompareValue;
    bool bIsBool;

    FAnimTransitionCondition()
        : ComparisonType(EComparisonType::Equals)
        , CompareValue(0.0f)
        , bIsBool(false)
    {}
};

class KAnimationTransition;

class KAnimationState
{
public:
    KAnimationState();
    KAnimationState(const std::string& InName);
    ~KAnimationState() = default;

    KAnimationState(const KAnimationState&) = delete;
    KAnimationState& operator=(const KAnimationState&) = delete;

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void SetAnimation(std::shared_ptr<KAnimation> InAnimation) { Animation = InAnimation; }
    std::shared_ptr<KAnimation> GetAnimation() const { return Animation; }

    void SetLooping(bool bLoop) { bIsLooping = bLoop; }
    bool IsLooping() const { return bIsLooping; }

    void SetSpeed(float InSpeed) { Speed = InSpeed; }
    float GetSpeed() const { return Speed; }

    void SetBlendDuration(float Duration) { BlendDuration = Duration; }
    float GetBlendDuration() const { return BlendDuration; }

    void AddTransition(KAnimationTransition* Transition);
    void RemoveTransition(const std::string& TargetStateName);
    const std::vector<KAnimationTransition*>& GetTransitions() const { return Transitions; }

    void AddNotify(const FAnimNotify& Notify);
    void ClearNotifies() { Notifies.clear(); }
    const std::vector<FAnimNotify>& GetNotifies() const { return Notifies; }

    void CheckNotifies(float CurrentTime, float PreviousTime, float Duration);

    float GetCurrentTime() const { return CurrentTime; }
    void SetCurrentTime(float Time) { CurrentTime = Time; }

    EAnimStateStatus GetStatus() const { return Status; }
    void SetStatus(EAnimStateStatus InStatus) { Status = InStatus; }

    float GetStateWeight() const { return StateWeight; }
    void SetStateWeight(float Weight) { StateWeight = Weight; }

    void Update(float DeltaTime);
    void Reset();

private:
    std::string Name;
    std::shared_ptr<KAnimation> Animation;
    std::vector<KAnimationTransition*> Transitions;
    std::vector<FAnimNotify> Notifies;

    float CurrentTime = 0.0f;
    float Speed = 1.0f;
    float BlendDuration = 0.25f;
    float StateWeight = 1.0f;
    bool bIsLooping = true;

    EAnimStateStatus Status = EAnimStateStatus::Inactive;
};

class KAnimationTransition
{
public:
    KAnimationTransition();
    KAnimationTransition(const std::string& InSourceState, const std::string& InTargetState);
    ~KAnimationTransition() = default;

    void SetSourceState(const std::string& Name) { SourceStateName = Name; }
    const std::string& GetSourceState() const { return SourceStateName; }

    void SetTargetState(const std::string& Name) { TargetStateName = Name; }
    const std::string& GetTargetState() const { return TargetStateName; }

    void SetBlendDuration(float Duration) { BlendDuration = Duration; }
    float GetBlendDuration() const { return BlendDuration; }

    void AddCondition(const FAnimTransitionCondition& Condition);
    void ClearConditions() { Conditions.clear(); }
    const std::vector<FAnimTransitionCondition>& GetConditions() const { return Conditions; }

    void SetCanTransition(bool bCan) { bCanTransition = bCan; }
    bool CanTransition() const { return bCanTransition; }

    void SetExitTime(float Time) { ExitTime = Time; }
    float GetExitTime() const { return ExitTime; }

    void SetHasExitTime(bool bHas) { bHasExitTime = bHas; }
    bool HasExitTime() const { return bHasExitTime; }

    bool CheckConditions(const std::unordered_map<std::string, float>& FloatParams,
                         const std::unordered_map<std::string, bool>& BoolParams) const;

    float GetBlendProgress() const { return BlendProgress; }
    void SetBlendProgress(float Progress) { BlendProgress = Progress; }

    void Reset();

private:
    std::string SourceStateName;
    std::string TargetStateName;
    std::vector<FAnimTransitionCondition> Conditions;

    float BlendDuration = 0.25f;
    float ExitTime = 1.0f;
    float BlendProgress = 0.0f;
    bool bCanTransition = false;
    bool bHasExitTime = true;
};

struct FBlendState
{
    KAnimationState* SourceState = nullptr;
    KAnimationState* TargetState = nullptr;
    KAnimationTransition* Transition = nullptr;
    float BlendProgress = 0.0f;
    float BlendDuration = 0.25f;
    bool bIsActive = false;
};

class KAnimationStateMachine
{
public:
    KAnimationStateMachine();
    ~KAnimationStateMachine() = default;

    KAnimationStateMachine(const KAnimationStateMachine&) = delete;
    KAnimationStateMachine& operator=(const KAnimationStateMachine&) = delete;

    void SetSkeleton(KSkeleton* InSkeleton) { Skeleton = InSkeleton; }
    KSkeleton* GetSkeleton() const { return Skeleton; }

    KAnimationState* AddState(const std::string& Name, std::shared_ptr<KAnimation> Animation);
    void RemoveState(const std::string& Name);
    KAnimationState* GetState(const std::string& Name);
    const KAnimationState* GetState(const std::string& Name) const;
    bool HasState(const std::string& Name) const;

    KAnimationTransition* AddTransition(const std::string& FromState, const std::string& ToState,
                                        float BlendDuration = 0.25f, bool bHasExitTime = true, float ExitTime = 1.0f);
    void RemoveTransition(const std::string& FromState, const std::string& ToState);
    KAnimationTransition* GetTransition(const std::string& FromState, const std::string& ToState);

    void SetDefaultState(const std::string& Name);
    const std::string& GetCurrentStateName() const { return CurrentStateName; }
    KAnimationState* GetCurrentState() { return CurrentState; }
    const KAnimationState* GetCurrentState() const { return CurrentState; }

    void SetFloatParameter(const std::string& Name, float Value);
    float GetFloatParameter(const std::string& Name) const;
    void SetBoolParameter(const std::string& Name, bool Value);
    bool GetBoolParameter(const std::string& Name) const;

    void TriggerTransition(const std::string& TargetStateName);
    void ForceState(const std::string& StateName, float BlendDuration = 0.25f);

    void Update(float DeltaTime);

    const std::vector<XMMATRIX>& GetBoneMatrices() const { return BlendedBoneMatrices; }
    const XMMATRIX* GetBoneMatrixData() const { return BlendedBoneMatrices.empty() ? nullptr : BlendedBoneMatrices.data(); }
    uint32 GetBoneMatrixCount() const { return static_cast<uint32>(BlendedBoneMatrices.size()); }

    bool IsTransitioning() const { return BlendState.bIsActive; }
    float GetBlendProgress() const { return BlendState.BlendProgress; }
    const std::string& GetPreviousStateName() const { return PreviousStateName; }

    void Reset();

    const std::unordered_map<std::string, std::unique_ptr<KAnimationState>>& GetStates() const { return States; }

    size_t GetStateCount() const { return States.size(); }
    size_t GetTransitionCount() const;

private:
    void EvaluateState(KAnimationState* State, float Weight, std::vector<XMMATRIX>& OutMatrices);
    void BlendBoneMatrices(const std::vector<XMMATRIX>& Source, const std::vector<XMMATRIX>& Target,
                           float BlendFactor, std::vector<XMMATRIX>& Out);
    void CheckTransitions();
    void ProcessTransition(KAnimationTransition* Transition);
    void UpdateBlending(float DeltaTime);
    void FinalizeTransition();

private:
    KSkeleton* Skeleton = nullptr;

    std::unordered_map<std::string, std::unique_ptr<KAnimationState>> States;
    std::unordered_map<std::string, float> FloatParameters;
    std::unordered_map<std::string, bool> BoolParameters;

    std::string DefaultStateName;
    std::string CurrentStateName;
    std::string PreviousStateName;
    KAnimationState* CurrentState = nullptr;

    FBlendState BlendState;
    std::vector<XMMATRIX> BlendedBoneMatrices;
    std::vector<XMMATRIX> TempBoneMatricesA;
    std::vector<XMMATRIX> TempBoneMatricesB;
};
