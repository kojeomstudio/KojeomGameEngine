#include "AnimationInstance.h"
#include <algorithm>

KAnimationInstance::KAnimationInstance()
{
}

void KAnimationInstance::PlayAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim,
                                       EAnimationPlayMode Mode)
{
    if (!Anim) return;

    CurrentAnimationName = Name;
    CurrentState.Animation = Anim;
    CurrentState.PlayMode = Mode;
    CurrentState.State = EAnimationState::Playing;
    CurrentState.CurrentTime = 0.0f;
    CurrentState.bReverse = false;

    if (Anim->IsValid() && Skeleton)
    {
        Anim->BuildBoneIndexMap(Skeleton);
        
        uint32 boneCount = Skeleton->GetBoneCount();
        BoneMatrices.resize(boneCount, XMMatrixIdentity());
        FinalBoneMatrices.resize(boneCount, XMMatrixIdentity());
    }
}

void KAnimationInstance::StopAnimation()
{
    CurrentState.State = EAnimationState::Stopped;
    CurrentState.CurrentTime = 0.0f;
}

void KAnimationInstance::PauseAnimation()
{
    if (CurrentState.State == EAnimationState::Playing)
    {
        CurrentState.State = EAnimationState::Paused;
    }
}

void KAnimationInstance::ResumeAnimation()
{
    if (CurrentState.State == EAnimationState::Paused)
    {
        CurrentState.State = EAnimationState::Playing;
    }
}

void KAnimationInstance::SetCurrentTime(float Time)
{
    if (!CurrentState.Animation) return;

    float duration = CurrentState.Animation->GetDurationInSeconds();
    if (duration <= 0.0f) return;
    CurrentState.CurrentTime = std::max(0.0f, std::min(Time, duration));
}

float KAnimationInstance::GetNormalizedTime() const
{
    if (!CurrentState.Animation) return 0.0f;

    float duration = CurrentState.Animation->GetDurationInSeconds();
    if (duration <= 0.0f) return 0.0f;

    return CurrentState.CurrentTime / duration;
}

void KAnimationInstance::Update(float DeltaTime)
{
    if (!CurrentState.Animation || CurrentState.State != EAnimationState::Playing)
    {
        return;
    }

    float duration = CurrentState.Animation->GetDurationInSeconds();
    if (duration <= 0.0f)
    {
        return;
    }

    float speedMultiplier = CurrentState.bReverse ? -1.0f : 1.0f;
    CurrentState.CurrentTime += DeltaTime * CurrentState.PlaybackSpeed * speedMultiplier;

    switch (CurrentState.PlayMode)
    {
    case EAnimationPlayMode::Once:
        if (CurrentState.CurrentTime >= duration)
        {
            CurrentState.CurrentTime = duration;
            CurrentState.State = EAnimationState::Stopped;
        }
        else if (CurrentState.CurrentTime < 0.0f)
        {
            CurrentState.CurrentTime = 0.0f;
            CurrentState.State = EAnimationState::Stopped;
        }
        break;

    case EAnimationPlayMode::Loop:
        if (CurrentState.CurrentTime >= duration)
        {
            CurrentState.CurrentTime = fmodf(CurrentState.CurrentTime, duration);
        }
        else if (CurrentState.CurrentTime < 0.0f)
        {
            CurrentState.CurrentTime = duration + fmodf(CurrentState.CurrentTime, duration);
        }
        break;

    case EAnimationPlayMode::PingPong:
        if (CurrentState.CurrentTime >= duration)
        {
            float overflow = CurrentState.CurrentTime - duration;
            float fullCycle = duration * 2.0f;
            if (fullCycle > 0.0f)
            {
                overflow = fmodf(overflow, fullCycle);
            }
            if (overflow <= duration)
            {
                CurrentState.CurrentTime = duration - overflow;
                CurrentState.bReverse = true;
            }
            else
            {
                CurrentState.CurrentTime = overflow - duration;
                CurrentState.bReverse = false;
            }
        }
        else if (CurrentState.CurrentTime < 0.0f)
        {
            float underflow = -CurrentState.CurrentTime;
            float fullCycle = duration * 2.0f;
            if (fullCycle > 0.0f)
            {
                underflow = fmodf(underflow, fullCycle);
            }
            if (underflow <= duration)
            {
                CurrentState.CurrentTime = underflow;
                CurrentState.bReverse = true;
            }
            else
            {
                CurrentState.CurrentTime = duration - (underflow - duration);
                CurrentState.bReverse = false;
            }
        }
        break;
    }

    UpdateBoneMatrices();
}

void KAnimationInstance::UpdateBoneMatrices()
{
    if (!CurrentState.Animation || !Skeleton)
    {
        return;
    }

    EvaluateAnimation();
    CalculateFinalBoneTransforms();
}

void KAnimationInstance::EvaluateAnimation()
{
    uint32 boneCount = Skeleton->GetBoneCount();
    
    std::vector<XMMATRIX> localTransforms(boneCount, XMMatrixIdentity());

    for (uint32 i = 0; i < boneCount; ++i)
    {
        const FBone* bone = Skeleton->GetBone(i);
        if (!bone) continue;

        const FAnimationChannel* channel = CurrentState.Animation->GetChannelByBoneIndex(i);
        
        if (channel)
        {
            float ticksPerSecond = CurrentState.Animation->GetTicksPerSecond();
            float timeInTicks = CurrentState.CurrentTime * ticksPerSecond;
            XMFLOAT3 pos = channel->InterpolatePosition(timeInTicks);
            XMFLOAT4 rot = channel->InterpolateRotation(timeInTicks);
            XMFLOAT3 scale = channel->InterpolateScale(timeInTicks);

            XMVECTOR posVec = XMLoadFloat3(&pos);
            XMVECTOR rotVec = XMLoadFloat4(&rot);
            XMVECTOR scaleVec = XMLoadFloat3(&scale);

            localTransforms[i] = XMMatrixScalingFromVector(scaleVec) *
                                 XMMatrixRotationQuaternion(rotVec) *
                                 XMMatrixTranslationFromVector(posVec);
        }
        else
        {
            localTransforms[i] = XMMatrixScalingFromVector(XMLoadFloat3(&bone->LocalScale)) *
                                 XMMatrixRotationQuaternion(XMLoadFloat4(&bone->LocalRotation)) *
                                 XMMatrixTranslationFromVector(XMLoadFloat3(&bone->LocalPosition));
        }
    }

    std::vector<uint32> sortedBones;
    sortedBones.reserve(boneCount);
    std::vector<uint32> unprocessedParents(boneCount, 0);
    for (uint32 i = 0; i < boneCount; ++i)
    {
        const FBone* bone = Skeleton->GetBone(i);
        if (bone && bone->ParentIndex >= 0)
        {
            unprocessedParents[i]++;
        }
    }
    std::vector<uint32> queue;
    for (uint32 i = 0; i < boneCount; ++i)
    {
        if (unprocessedParents[i] == 0)
        {
            queue.push_back(i);
        }
    }
    while (!queue.empty())
    {
        uint32 boneIdx = queue.back();
        queue.pop_back();
        sortedBones.push_back(boneIdx);

        for (uint32 i = 0; i < boneCount; ++i)
        {
            const FBone* bone = Skeleton->GetBone(i);
            if (bone && bone->ParentIndex == static_cast<int32>(boneIdx))
            {
                unprocessedParents[i]--;
                if (unprocessedParents[i] == 0)
                {
                    queue.push_back(i);
                }
            }
        }
    }

    for (uint32 boneIdx : sortedBones)
    {
        const FBone* bone = Skeleton->GetBone(boneIdx);
        if (!bone) continue;

        if (bone->ParentIndex >= 0)
        {
            BoneMatrices[boneIdx] = localTransforms[boneIdx] * BoneMatrices[bone->ParentIndex];
        }
        else
        {
            BoneMatrices[boneIdx] = localTransforms[boneIdx];
        }
    }
}

void KAnimationInstance::CalculateBoneTransformRecursive(uint32 BoneIndex, const std::vector<XMMATRIX>& LocalTransforms)
{
    const FBone* bone = Skeleton->GetBone(BoneIndex);
    if (!bone) return;

    if (bone->ParentIndex >= 0)
    {
        BoneMatrices[BoneIndex] = LocalTransforms[BoneIndex] * BoneMatrices[bone->ParentIndex];
    }
    else
    {
        BoneMatrices[BoneIndex] = LocalTransforms[BoneIndex];
    }
}

void KAnimationInstance::CalculateFinalBoneTransforms()
{
    uint32 boneCount = Skeleton->GetBoneCount();
    FinalBoneMatrices.resize(boneCount);

    for (uint32 i = 0; i < boneCount; ++i)
    {
        const FBone* bone = Skeleton->GetBone(i);
        if (bone)
        {
            FinalBoneMatrices[i] = BoneMatrices[i] * bone->InverseBindPose;
        }
        else
        {
            FinalBoneMatrices[i] = XMMatrixIdentity();
        }
    }
}

void KAnimationInstance::AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Anim)
{
    if (Anim)
    {
        Animations[Name] = Anim;
    }
}

void KAnimationInstance::RemoveAnimation(const std::string& Name)
{
    Animations.erase(Name);
}

std::shared_ptr<KAnimation> KAnimationInstance::GetAnimation(const std::string& Name) const
{
    auto it = Animations.find(Name);
    if (it != Animations.end())
    {
        return it->second;
    }
    return nullptr;
}

void KAnimationInstance::ClearAnimations()
{
    Animations.clear();
    CurrentState.Animation = nullptr;
    CurrentState.State = EAnimationState::Stopped;
}
