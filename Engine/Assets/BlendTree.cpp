#include "BlendTree.h"
#include <limits>

namespace
{
    float Clamp(float Value, float Min, float Max)
    {
        return Value < Min ? Min : (Value > Max ? Max : Value);
    }
}

KBlendTree::KBlendTree()
{
}

int KBlendTree::AddChild(std::shared_ptr<KAnimation> Anim, float ParameterValue)
{
    if (!Anim)
    {
        return -1;
    }

    FBlendTreeChild Child;
    Child.Animation = Anim;
    Child.ParameterValue = ParameterValue;
    Child.CurrentTime = 0.0f;
    Child.Speed = 1.0f;
    Child.bLooping = true;
    Child.BlendWeight = 0.0f;

    if (Skeleton)
    {
        Anim->BuildBoneIndexMap(Skeleton);
        uint32 BoneCount = Skeleton->GetBoneCount();
        Child.BoneMatrices.resize(BoneCount, XMMatrixIdentity());
    }

    Children.push_back(std::move(Child));
    return static_cast<int>(Children.size()) - 1;
}

void KBlendTree::RemoveChild(int Index)
{
    if (Index >= 0 && Index < static_cast<int>(Children.size()))
    {
        Children.erase(Children.begin() + Index);
    }
}

const FBlendTreeChild* KBlendTree::GetChild(int Index) const
{
    if (Index >= 0 && Index < static_cast<int>(Children.size()))
    {
        return &Children[Index];
    }
    return nullptr;
}

FBlendTreeChild* KBlendTree::GetChildMutable(int Index)
{
    if (Index >= 0 && Index < static_cast<int>(Children.size()))
    {
        return &Children[Index];
    }
    return nullptr;
}

void KBlendTree::ComputeWeights(float ParameterValue)
{
    if (Children.empty())
    {
        return;
    }

    for (auto& Child : Children)
    {
        Child.BlendWeight = 0.0f;
    }

    if (Children.size() == 1)
    {
        Children[0].BlendWeight = 1.0f;
        return;
    }

    std::vector<int> sortedIndices(static_cast<size_t>(Children.size()));
    for (int i = 0; i < static_cast<int>(Children.size()); ++i)
    {
        sortedIndices[static_cast<size_t>(i)] = i;
    }
    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [this](int A, int B) { return Children[A].ParameterValue < Children[B].ParameterValue; });

    float MinParam = Children[sortedIndices[0]].ParameterValue;
    float MaxParam = Children[sortedIndices.back()].ParameterValue;

    if (ParameterValue <= MinParam)
    {
        Children[sortedIndices[0]].BlendWeight = 1.0f;
        return;
    }

    if (ParameterValue >= MaxParam)
    {
        Children[sortedIndices.back()].BlendWeight = 1.0f;
        return;
    }

    int LowerIdx = -1;
    int UpperIdx = -1;

    for (size_t i = 0; i < sortedIndices.size(); ++i)
    {
        int Idx = sortedIndices[i];
        if (Children[Idx].ParameterValue <= ParameterValue)
        {
            LowerIdx = Idx;
        }
    }
    for (size_t i = 0; i < sortedIndices.size(); ++i)
    {
        int Idx = sortedIndices[i];
        if (Children[Idx].ParameterValue >= ParameterValue && UpperIdx == -1)
        {
            UpperIdx = Idx;
        }
    }

    if (LowerIdx == -1 || UpperIdx == -1 || LowerIdx == UpperIdx)
    {
        int TargetIdx = LowerIdx != -1 ? LowerIdx : UpperIdx;
        if (TargetIdx >= 0)
        {
            Children[TargetIdx].BlendWeight = 1.0f;
        }
        return;
    }

    float Range = Children[UpperIdx].ParameterValue - Children[LowerIdx].ParameterValue;
    float Alpha = (Range > 0.0001f) ? (ParameterValue - Children[LowerIdx].ParameterValue) / Range : 0.0f;
    Alpha = Clamp(Alpha, 0.0f, 1.0f);

    Children[LowerIdx].BlendWeight = 1.0f - Alpha;
    Children[UpperIdx].BlendWeight = Alpha;
}

void KBlendTree::EvaluateChild(FBlendTreeChild& Child, std::vector<XMMATRIX>& OutMatrices)
{
    if (!Child.Animation || !Skeleton)
    {
        return;
    }

    auto* Anim = Child.Animation.get();
    float TimeInTicks = Child.CurrentTime * Anim->GetTicksPerSecond();

    uint32 BoneCount = Skeleton->GetBoneCount();
    OutMatrices.resize(BoneCount);

    std::vector<XMMATRIX> LocalTransforms(BoneCount, XMMatrixIdentity());

    for (uint32 i = 0; i < BoneCount; ++i)
    {
        const FAnimationChannel* Channel = Anim->GetChannelByBoneIndex(i);

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
            const FBone* Bone = Skeleton->GetBone(i);
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

        LocalTransforms[i] = XMMatrixScalingFromVector(ScaleVec) *
                             XMMatrixRotationQuaternion(RotQuat) *
                             XMMatrixTranslationFromVector(PosVec);
    }

    std::vector<bool> Processed(BoneCount, false);
    std::vector<uint32> EvalOrder;
    EvalOrder.reserve(BoneCount);

    bool bProgress = true;
    while (bProgress && EvalOrder.size() < BoneCount)
    {
        bProgress = false;
        for (uint32 i = 0; i < BoneCount; ++i)
        {
            if (Processed[i]) continue;

            const FBone* Bone = Skeleton->GetBone(i);
            if (!Bone) { Processed[i] = true; EvalOrder.push_back(i); bProgress = true; continue; }

            if (Bone->ParentIndex == INVALID_BONE_INDEX ||
                (Bone->ParentIndex >= 0 && static_cast<uint32>(Bone->ParentIndex) < BoneCount && Processed[Bone->ParentIndex]))
            {
                EvalOrder.push_back(i);
                Processed[i] = true;
                bProgress = true;
            }
        }
    }

    for (uint32 i = 0; i < BoneCount; ++i)
    {
        if (!Processed[i]) EvalOrder.push_back(i);
    }

    for (uint32 BoneIdx : EvalOrder)
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
            OutMatrices[BoneIdx] = Bone->InverseBindPose * OutMatrices[BoneIdx];
        }
    }
}

void KBlendTree::BlendChildMatrices(const std::vector<std::vector<XMMATRIX>>& AllMatrices,
                                     const std::vector<float>& Weights,
                                     std::vector<XMMATRIX>& OutMatrices)
{
    if (AllMatrices.empty() || Weights.empty())
    {
        return;
    }

    uint32 BoneCount = static_cast<uint32>(AllMatrices[0].size());
    OutMatrices.resize(BoneCount);

    for (uint32 BoneIdx = 0; BoneIdx < BoneCount; ++BoneIdx)
    {
        XMVECTOR AccScale = XMVectorSet(0, 0, 0, 0);
        XMVECTOR AccRot = XMVectorSet(0, 0, 0, 0);
        XMVECTOR AccTrans = XMVectorSet(0, 0, 0, 0);
        float TotalWeight = 0.0f;

        for (size_t ChildIdx = 0; ChildIdx < AllMatrices.size(); ++ChildIdx)
        {
            if (ChildIdx >= Weights.size()) break;
            float W = Weights[ChildIdx];
            if (W < 0.0001f) continue;
            if (BoneIdx >= AllMatrices[ChildIdx].size()) continue;

            XMVECTOR S, R, T;
            if (!XMMatrixDecompose(&S, &R, &T, AllMatrices[ChildIdx][BoneIdx]))
            {
                S = XMVectorSet(1, 1, 1, 0);
                R = XMQuaternionIdentity();
                T = XMVectorSet(0, 0, 0, 0);
            }

            AccScale = XMVectorAdd(AccScale, XMVectorScale(S, W));
            AccTrans = XMVectorAdd(AccTrans, XMVectorScale(T, W));
            AccRot = XMVectorAdd(AccRot, XMVectorScale(R, W));
            TotalWeight += W;
        }

        if (TotalWeight > 0.0001f)
        {
            AccScale = XMVectorScale(AccScale, 1.0f / TotalWeight);
            AccTrans = XMVectorScale(AccTrans, 1.0f / TotalWeight);
            AccRot = XMVectorScale(AccRot, 1.0f / TotalWeight);

            float RotLength = XMVectorGetX(XMVector4Length(AccRot));
            if (RotLength > 0.0001f)
            {
                AccRot = XMVector4Normalize(AccRot);
            }
            else
            {
                AccRot = XMQuaternionIdentity();
            }

            OutMatrices[BoneIdx] = XMMatrixScalingFromVector(AccScale) *
                                   XMMatrixRotationQuaternion(AccRot) *
                                   XMMatrixTranslationFromVector(AccTrans);
        }
        else
        {
            OutMatrices[BoneIdx] = XMMatrixIdentity();
        }
    }
}

void KBlendTree::Update(float DeltaTime, float ParameterValue)
{
    if (!Skeleton || Children.empty())
    {
        return;
    }

    ComputeWeights(ParameterValue);

    for (auto& Child : Children)
    {
        if (Child.BlendWeight < 0.0001f || !Child.Animation)
        {
            continue;
        }

        float Duration = Child.Animation->GetDurationInSeconds();
        if (Duration <= 0.0f)
        {
            continue;
        }

        Child.CurrentTime += DeltaTime * Child.Speed;

        if (Child.bLooping)
        {
            while (Child.CurrentTime >= Duration)
            {
                Child.CurrentTime -= Duration;
            }
            while (Child.CurrentTime < 0.0f)
            {
                Child.CurrentTime += Duration;
            }
        }
        else
        {
            Child.CurrentTime = Clamp(Child.CurrentTime, 0.0f, Duration);
        }
    }

    std::vector<int> ActiveIndices;
    std::vector<float> ActiveWeights;
    std::vector<std::vector<XMMATRIX>> AllMatrices;

    for (int i = 0; i < static_cast<int>(Children.size()); ++i)
    {
        if (Children[i].BlendWeight < 0.0001f || !Children[i].Animation)
        {
            continue;
        }

        std::vector<XMMATRIX> ChildMatrices;
        EvaluateChild(Children[i], ChildMatrices);

        ActiveIndices.push_back(i);
        ActiveWeights.push_back(Children[i].BlendWeight);
        AllMatrices.push_back(std::move(ChildMatrices));
    }

    if (ActiveIndices.size() == 1)
    {
        BlendedBoneMatrices = std::move(AllMatrices[0]);
    }
    else if (ActiveIndices.size() > 1)
    {
        BlendChildMatrices(AllMatrices, ActiveWeights, BlendedBoneMatrices);
    }
}

float KBlendTree::GetChildWeight(int Index) const
{
    if (Index >= 0 && Index < static_cast<int>(Children.size()))
    {
        return Children[Index].BlendWeight;
    }
    return 0.0f;
}

void KBlendTree::Reset()
{
    for (auto& Child : Children)
    {
        Child.CurrentTime = 0.0f;
        Child.BlendWeight = 0.0f;
        Child.BoneMatrices.clear();
    }
    BlendedBoneMatrices.clear();
}
