#include "AnimationStateMachine.h"
#include <algorithm>

KAnimationState::KAnimationState()
{
}

KAnimationState::KAnimationState(const std::string& InName)
    : Name(InName)
{
}

KAnimationState::~KAnimationState()
{
    Transitions.clear();
}

void KAnimationState::AddTransition(std::unique_ptr<KAnimationTransition> Transition)
{
    if (Transition)
    {
        Transitions.push_back(std::move(Transition));
    }
}

void KAnimationState::RemoveTransition(const std::string& TargetStateName)
{
    auto It = Transitions.begin();
    while (It != Transitions.end())
    {
        if (*It && (*It)->GetTargetState() == TargetStateName)
        {
            It = Transitions.erase(It);
        }
        else
        {
            ++It;
        }
    }
}

void KAnimationState::AddNotify(const FAnimNotify& Notify)
{
    Notifies.push_back(Notify);
}

void KAnimationState::CheckNotifies(float CurrentTime, float PreviousTime, float Duration)
{
    for (auto& Notify : Notifies)
    {
        Notify.bTriggered = false;

        float NotifyTime = Notify.TriggerTime * Duration;
        
        if (PreviousTime < CurrentTime)
        {
            if (PreviousTime <= NotifyTime && NotifyTime < CurrentTime)
            {
                Notify.bTriggered = true;
                if (Notify.Callback)
                {
                    Notify.Callback();
                }
            }
        }
        else if (bIsLooping)
        {
            if (PreviousTime <= NotifyTime || NotifyTime < CurrentTime)
            {
                Notify.bTriggered = true;
                if (Notify.Callback)
                {
                    Notify.Callback();
                }
            }
        }
    }
}

void KAnimationState::Update(float DeltaTime)
{
    if (!Animation || Status == EAnimStateStatus::Inactive)
    {
        return;
    }

    float Duration = Animation->GetDurationInSeconds();
    if (Duration <= 0.0f)
    {
        return;
    }

    float PreviousTime = CurrentTime;
    CurrentTime += DeltaTime * Speed;

    if (bIsLooping)
    {
        while (CurrentTime >= Duration)
        {
            CurrentTime -= Duration;
        }
        while (CurrentTime < 0.0f)
        {
            CurrentTime += Duration;
        }
    }
    else
    {
        if (CurrentTime >= Duration)
        {
            CurrentTime = Duration;
            Status = EAnimStateStatus::Inactive;
        }
    }

    CheckNotifies(CurrentTime, PreviousTime, Duration);
}

void KAnimationState::Reset()
{
    CurrentTime = 0.0f;
    Status = EAnimStateStatus::Inactive;
    StateWeight = 1.0f;
    for (auto& Notify : Notifies)
    {
        Notify.bTriggered = false;
    }
}

KAnimationTransition::KAnimationTransition()
{
}

KAnimationTransition::KAnimationTransition(const std::string& InSourceState, const std::string& InTargetState)
    : SourceStateName(InSourceState)
    , TargetStateName(InTargetState)
{
}

void KAnimationTransition::AddCondition(const FAnimTransitionCondition& Condition)
{
    Conditions.push_back(Condition);
}

bool KAnimationTransition::CheckConditions(const std::unordered_map<std::string, float>& FloatParams,
                                            const std::unordered_map<std::string, bool>& BoolParams) const
{
    if (Conditions.empty())
    {
        return true;
    }

    bool anyEvaluated = false;

    for (const auto& Condition : Conditions)
    {
        if (Condition.bIsBool)
        {
            auto It = BoolParams.find(Condition.ParameterName);
            if (It == BoolParams.end())
            {
                return false;
            }

            anyEvaluated = true;
            bool Value = It->second;
            bool Expected = Condition.CompareValue > 0.5f;

            switch (Condition.ComparisonType)
            {
            case FAnimTransitionCondition::EComparisonType::Equals:
                if (Value != Expected) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::NotEquals:
                if (Value == Expected) return false;
                break;
            default:
                break;
            }
        }
        else
        {
            auto It = FloatParams.find(Condition.ParameterName);
            if (It == FloatParams.end())
            {
                return false;
            }

            anyEvaluated = true;
            float Value = It->second;

            switch (Condition.ComparisonType)
            {
            case FAnimTransitionCondition::EComparisonType::Equals:
                if (std::abs(Value - Condition.CompareValue) > 0.001f) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::NotEquals:
                if (std::abs(Value - Condition.CompareValue) <= 0.001f) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::Greater:
                if (Value <= Condition.CompareValue) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::GreaterOrEquals:
                if (Value < Condition.CompareValue) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::Less:
                if (Value >= Condition.CompareValue) return false;
                break;
            case FAnimTransitionCondition::EComparisonType::LessOrEquals:
                if (Value > Condition.CompareValue) return false;
                break;
            }
        }
    }

    return anyEvaluated;
}

void KAnimationTransition::Reset()
{
    BlendProgress = 0.0f;
    bCanTransition = false;
}

KAnimationStateMachine::KAnimationStateMachine()
{
}

KAnimationState* KAnimationStateMachine::AddState(const std::string& Name, std::shared_ptr<KAnimation> Animation)
{
    if (Name.empty())
    {
        return nullptr;
    }

    auto State = std::make_unique<KAnimationState>(Name);
    State->SetAnimation(Animation);
    
    KAnimationState* StatePtr = State.get();
    States[Name] = std::move(State);

    if (States.size() == 1)
    {
        DefaultStateName = Name;
    }

    return StatePtr;
}

void KAnimationStateMachine::RemoveState(const std::string& Name)
{
    if (BlendState.bIsActive)
    {
        if ((CurrentStateName == Name && BlendState.TargetState && BlendState.TargetState->GetName() == Name) ||
            (BlendState.SourceState && BlendState.SourceState->GetName() == Name))
        {
            BlendState.bIsActive = false;
            BlendState.SourceState = nullptr;
            BlendState.TargetState = nullptr;
            BlendState.Transition = nullptr;
            BlendState.BlendProgress = 0.0f;
        }
    }

    if (CurrentStateName == Name)
    {
        if (!DefaultStateName.empty() && States.count(DefaultStateName))
        {
            CurrentState = States[DefaultStateName].get();
            CurrentStateName = DefaultStateName;
        }
        else
        {
            CurrentState = nullptr;
            CurrentStateName.clear();
        }
    }

    auto It = States.find(Name);
    if (It != States.end())
    {
        for (auto& [StateName, State] : States)
        {
            State->RemoveTransition(Name);
        }
        States.erase(It);
    }
}

KAnimationState* KAnimationStateMachine::GetState(const std::string& Name)
{
    auto It = States.find(Name);
    return It != States.end() ? It->second.get() : nullptr;
}

const KAnimationState* KAnimationStateMachine::GetState(const std::string& Name) const
{
    auto It = States.find(Name);
    return It != States.end() ? It->second.get() : nullptr;
}

bool KAnimationStateMachine::HasState(const std::string& Name) const
{
    return States.count(Name) > 0;
}

KAnimationTransition* KAnimationStateMachine::AddTransition(const std::string& FromState, const std::string& ToState,
                                                            float BlendDuration, bool bHasExitTime, float ExitTime)
{
    KAnimationState* SourceState = GetState(FromState);
    KAnimationState* TargetState = GetState(ToState);

    if (!SourceState || !TargetState)
    {
        return nullptr;
    }

    auto Transition = std::make_unique<KAnimationTransition>(FromState, ToState);
    Transition->SetBlendDuration(BlendDuration);
    Transition->SetHasExitTime(bHasExitTime);
    Transition->SetExitTime(ExitTime);

    KAnimationTransition* TransitionPtr = Transition.get();
    SourceState->AddTransition(std::move(Transition));

    return TransitionPtr;
}

void KAnimationStateMachine::RemoveTransition(const std::string& FromState, const std::string& ToState)
{
    KAnimationState* SourceState = GetState(FromState);
    if (SourceState)
    {
        SourceState->RemoveTransition(ToState);
    }
}

KAnimationTransition* KAnimationStateMachine::GetTransition(const std::string& FromState, const std::string& ToState)
{
    KAnimationState* SourceState = GetState(FromState);
    if (!SourceState)
    {
        return nullptr;
    }

    for (const auto& Transition : SourceState->GetTransitions())
    {
        if (Transition && Transition->GetTargetState() == ToState)
        {
            return Transition.get();
        }
    }

    return nullptr;
}

void KAnimationStateMachine::SetDefaultState(const std::string& Name)
{
    if (States.count(Name) > 0)
    {
        DefaultStateName = Name;
        
        if (!CurrentState)
        {
            CurrentState = States[Name].get();
            CurrentStateName = Name;
            CurrentState->SetStatus(EAnimStateStatus::Active);
            CurrentState->SetStateWeight(1.0f);
        }
    }
}

void KAnimationStateMachine::SetFloatParameter(const std::string& Name, float Value)
{
    FloatParameters[Name] = Value;
}

float KAnimationStateMachine::GetFloatParameter(const std::string& Name) const
{
    auto It = FloatParameters.find(Name);
    return It != FloatParameters.end() ? It->second : 0.0f;
}

void KAnimationStateMachine::SetBoolParameter(const std::string& Name, bool Value)
{
    BoolParameters[Name] = Value;
}

bool KAnimationStateMachine::GetBoolParameter(const std::string& Name) const
{
    auto It = BoolParameters.find(Name);
    return It != BoolParameters.end() ? It->second : false;
}

void KAnimationStateMachine::TriggerTransition(const std::string& TargetStateName)
{
    if (BlendState.bIsActive)
    {
        return;
    }

    KAnimationState* TargetState = GetState(TargetStateName);
    if (!TargetState || TargetState == CurrentState)
    {
        return;
    }

    KAnimationTransition* Transition = GetTransition(CurrentStateName, TargetStateName);
    if (Transition)
    {
        ProcessTransition(Transition);
    }
    else
    {
        ForceState(TargetStateName, 0.25f);
    }
}

void KAnimationStateMachine::ForceState(const std::string& StateName, float BlendDuration)
{
    KAnimationState* TargetState = GetState(StateName);
    if (!TargetState)
    {
        return;
    }

    if (BlendState.bIsActive && BlendState.TargetState)
    {
        BlendState.SourceState = BlendState.TargetState;
        BlendState.TargetState = TargetState;
        BlendState.BlendDuration = BlendDuration;
        BlendState.BlendProgress = 0.0f;
        BlendState.bIsActive = true;

        if (BlendState.SourceState)
        {
            BlendState.SourceState->SetStatus(EAnimStateStatus::TransitioningOut);
        }

        PreviousStateName = CurrentStateName;
        CurrentStateName = StateName;
        CurrentState = TargetState;

        TargetState->Reset();
        TargetState->SetStatus(EAnimStateStatus::TransitioningIn);
    }
    else if (CurrentState)
    {
        BlendState.SourceState = CurrentState;
        BlendState.TargetState = TargetState;
        BlendState.BlendDuration = BlendDuration;
        BlendState.BlendProgress = 0.0f;
        BlendState.bIsActive = true;
        
        PreviousStateName = CurrentStateName;
        CurrentStateName = StateName;
        CurrentState = TargetState;
        
        CurrentState->SetStatus(EAnimStateStatus::TransitioningOut);
        TargetState->Reset();
        TargetState->SetStatus(EAnimStateStatus::TransitioningIn);
    }
    else
    {
        CurrentState = TargetState;
        CurrentStateName = StateName;
        TargetState->Reset();
        TargetState->SetStatus(EAnimStateStatus::Active);
        TargetState->SetStateWeight(1.0f);
    }
}

void KAnimationStateMachine::Update(float DeltaTime)
{
    if (!Skeleton)
    {
        return;
    }

    if (!CurrentState && !DefaultStateName.empty())
    {
        SetDefaultState(DefaultStateName);
    }

    if (!CurrentState)
    {
        return;
    }

    if (!BlendState.bIsActive)
    {
        CurrentState->Update(DeltaTime);
        CheckTransitions();
    }
    else
    {
        UpdateBlending(DeltaTime);
    }

    if (CurrentState && CurrentState->GetAnimation())
    {
        if (BlendState.bIsActive && BlendState.SourceState && BlendState.SourceState->GetAnimation())
        {
            EvaluateState(BlendState.SourceState, 1.0f - BlendState.BlendProgress, TempBoneMatricesA);
            EvaluateState(BlendState.TargetState, BlendState.BlendProgress, TempBoneMatricesB);
            BlendBoneMatrices(TempBoneMatricesA, TempBoneMatricesB, BlendState.BlendProgress, BlendedBoneMatrices);
        }
        else
        {
            EvaluateState(CurrentState, 1.0f, BlendedBoneMatrices);
        }
    }
}

void KAnimationStateMachine::EvaluateState(KAnimationState* State, float Weight, std::vector<XMMATRIX>& OutMatrices)
{
    if (!State || !State->GetAnimation() || !Skeleton)
    {
        return;
    }

    auto* Animation = State->GetAnimation().get();
    float CurrentTime = State->GetCurrentTime();
    float TicksPerSecond = Animation->GetTicksPerSecond();
    float TimeInTicks = CurrentTime * TicksPerSecond;
    
    uint32 BoneCount = Skeleton->GetBoneCount();
    OutMatrices.resize(BoneCount);

    std::vector<XMMATRIX> LocalTransforms(BoneCount);
    for (uint32 i = 0; i < BoneCount; ++i)
    {
        LocalTransforms[i] = XMMatrixIdentity();
    }

    for (uint32 BoneIdx = 0; BoneIdx < BoneCount; ++BoneIdx)
    {
        const FAnimationChannel* Channel = Animation->GetChannelByBoneIndex(BoneIdx);
        
        XMFLOAT3 Position(0, 0, 0);
        XMFLOAT4 Rotation(0, 0, 0, 1);
        XMFLOAT3 Scale(1, 1, 1);

        if (Channel)
        {
            Position = Channel->InterpolatePosition(TimeInTicks);
            Rotation = Channel->InterpolateRotation(TimeInTicks);
            Scale = Channel->InterpolateScale(TimeInTicks);
        }
        else
        {
            const FBone* Bone = Skeleton->GetBone(BoneIdx);
            if (Bone)
            {
                Position = Bone->LocalPosition;
                Rotation = Bone->LocalRotation;
                Scale = Bone->LocalScale;
            }
        }

        XMVECTOR PosVec = XMLoadFloat3(&Position);
        XMVECTOR RotQuat = XMLoadFloat4(&Rotation);
        XMVECTOR ScaleVec = XMLoadFloat3(&Scale);

        XMMATRIX TranslationMatrix = XMMatrixTranslationFromVector(PosVec);
        XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(RotQuat);
        XMMATRIX ScaleMatrix = XMMatrixScalingFromVector(ScaleVec);

        LocalTransforms[BoneIdx] = ScaleMatrix * RotationMatrix * TranslationMatrix;
    }

    std::vector<bool> processed(BoneCount, false);
    std::vector<uint32> evalOrder;
    evalOrder.reserve(BoneCount);

    bool bProgress = true;
    while (bProgress && evalOrder.size() < BoneCount)
    {
        bProgress = false;
        for (uint32 i = 0; i < BoneCount; ++i)
        {
            if (processed[i]) continue;

            const FBone* Bone = Skeleton->GetBone(i);
            if (!Bone) { processed[i] = true; evalOrder.push_back(i); bProgress = true; continue; }

            if (Bone->ParentIndex == INVALID_BONE_INDEX ||
                (Bone->ParentIndex >= 0 && static_cast<uint32>(Bone->ParentIndex) < BoneCount && processed[Bone->ParentIndex]))
            {
                evalOrder.push_back(i);
                processed[i] = true;
                bProgress = true;
            }
        }
    }

    for (uint32 i = 0; i < BoneCount; ++i)
    {
        if (!processed[i]) evalOrder.push_back(i);
    }

    for (uint32 BoneIdx : evalOrder)
    {
        XMMATRIX GlobalTransform = LocalTransforms[BoneIdx];

        const FBone* Bone = Skeleton->GetBone(BoneIdx);
        if (Bone && Bone->ParentIndex != INVALID_BONE_INDEX && Bone->ParentIndex >= 0 && static_cast<uint32>(Bone->ParentIndex) < BoneCount)
        {
            GlobalTransform = LocalTransforms[BoneIdx] * OutMatrices[Bone->ParentIndex];
        }

        OutMatrices[BoneIdx] = GlobalTransform;
    }

    for (uint32 BoneIdx = 0; BoneIdx < BoneCount; ++BoneIdx)
    {
        const FBone* Bone = Skeleton->GetBone(BoneIdx);
        if (Bone)
        {
            XMMATRIX InverseBindPose = Bone->InverseBindPose;
            OutMatrices[BoneIdx] = InverseBindPose * OutMatrices[BoneIdx];
        }
    }
}

void KAnimationStateMachine::BlendBoneMatrices(const std::vector<XMMATRIX>& Source,
                                                const std::vector<XMMATRIX>& Target,
                                                float BlendFactor,
                                                std::vector<XMMATRIX>& Out)
{
    size_t Count = std::min(Source.size(), Target.size());
    Out.resize(Count);

    for (size_t i = 0; i < Count; ++i)
    {
        XMVECTOR SourceScale, SourceRot, SourceTrans;
        XMVECTOR TargetScale, TargetRot, TargetTrans;

        if (!XMMatrixDecompose(&SourceScale, &SourceRot, &SourceTrans, Source[i]))
        {
            SourceScale = XMVectorSet(1, 1, 1, 0);
            SourceRot = XMQuaternionIdentity();
            SourceTrans = XMVectorSet(0, 0, 0, 0);
        }
        if (!XMMatrixDecompose(&TargetScale, &TargetRot, &TargetTrans, Target[i]))
        {
            TargetScale = XMVectorSet(1, 1, 1, 0);
            TargetRot = XMQuaternionIdentity();
            TargetTrans = XMVectorSet(0, 0, 0, 0);
        }

        XMVECTOR BlendScale = XMVectorLerp(SourceScale, TargetScale, BlendFactor);
        XMVECTOR BlendRot = XMQuaternionSlerp(SourceRot, TargetRot, BlendFactor);
        XMVECTOR BlendTrans = XMVectorLerp(SourceTrans, TargetTrans, BlendFactor);

        XMMATRIX ScaleMatrix = XMMatrixScalingFromVector(BlendScale);
        XMMATRIX RotMatrix = XMMatrixRotationQuaternion(BlendRot);
        XMMATRIX TransMatrix = XMMatrixTranslationFromVector(BlendTrans);

        Out[i] = ScaleMatrix * RotMatrix * TransMatrix;
    }
}

void KAnimationStateMachine::CheckTransitions()
{
    if (!CurrentState || BlendState.bIsActive)
    {
        return;
    }

    for (const auto& Transition : CurrentState->GetTransitions())
    {
        if (!Transition)
        {
            continue;
        }

        bool bConditionsMet = Transition->CheckConditions(FloatParameters, BoolParameters);

        if (Transition->HasExitTime())
        {
            auto Animation = CurrentState->GetAnimation();
            if (!Animation)
            {
                continue;
            }

            float Duration = Animation->GetDurationInSeconds();
            float CurrentTime = CurrentState->GetCurrentTime();
            float NormalizedTime = Duration > 0.0f ? CurrentTime / Duration : 0.0f;

            if (bConditionsMet && NormalizedTime >= Transition->GetExitTime())
            {
                ProcessTransition(Transition.get());
                return;
            }
        }
        else
        {
            if (bConditionsMet)
            {
                ProcessTransition(Transition.get());
                return;
            }
        }
    }
}

void KAnimationStateMachine::ProcessTransition(KAnimationTransition* Transition)
{
    if (!Transition)
    {
        return;
    }

    KAnimationState* TargetState = GetState(Transition->GetTargetState());
    if (!TargetState)
    {
        return;
    }

    BlendState.SourceState = CurrentState;
    BlendState.TargetState = TargetState;
    BlendState.Transition = Transition;
    BlendState.BlendDuration = Transition->GetBlendDuration();
    BlendState.BlendProgress = 0.0f;
    BlendState.bIsActive = true;

    PreviousStateName = CurrentStateName;
    CurrentStateName = Transition->GetTargetState();
    CurrentState = TargetState;

    if (BlendState.SourceState)
    {
        BlendState.SourceState->SetStatus(EAnimStateStatus::TransitioningOut);
    }
    
    TargetState->Reset();
    TargetState->SetStatus(EAnimStateStatus::TransitioningIn);
}

void KAnimationStateMachine::UpdateBlending(float DeltaTime)
{
    if (!BlendState.bIsActive)
    {
        return;
    }

    if (BlendState.SourceState)
    {
        BlendState.SourceState->Update(DeltaTime);
    }

    if (BlendState.TargetState)
    {
        BlendState.TargetState->Update(DeltaTime);
    }

    if (BlendState.BlendDuration > 0.0f)
    {
        BlendState.BlendProgress += DeltaTime / BlendState.BlendDuration;
    }
    else
    {
        BlendState.BlendProgress = 1.0f;
    }

    if (BlendState.BlendProgress >= 1.0f)
    {
        FinalizeTransition();
    }
}

void KAnimationStateMachine::FinalizeTransition()
{
    if (BlendState.SourceState)
    {
        BlendState.SourceState->SetStatus(EAnimStateStatus::Inactive);
        BlendState.SourceState->SetStateWeight(0.0f);
    }

    if (BlendState.TargetState)
    {
        BlendState.TargetState->SetStatus(EAnimStateStatus::Active);
        BlendState.TargetState->SetStateWeight(1.0f);
    }

    BlendState.bIsActive = false;
    BlendState.BlendProgress = 0.0f;
    BlendState.SourceState = nullptr;
    BlendState.Transition = nullptr;
}

void KAnimationStateMachine::Reset()
{
    for (auto& [Name, State] : States)
    {
        State->Reset();
    }

    CurrentState = nullptr;
    CurrentStateName.clear();
    PreviousStateName.clear();
    
    BlendState = FBlendState();
    BlendedBoneMatrices.clear();
    TempBoneMatricesA.clear();
    TempBoneMatricesB.clear();

    FloatParameters.clear();
    BoolParameters.clear();

    if (!DefaultStateName.empty())
    {
        SetDefaultState(DefaultStateName);
    }
}

size_t KAnimationStateMachine::GetTransitionCount() const
{
    size_t Count = 0;
    for (const auto& [Name, State] : States)
    {
        Count += State->GetTransitions().size();
    }
    return Count;
}
