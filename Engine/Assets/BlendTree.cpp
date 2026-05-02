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

int KBlendTree::AddChild2D(std::shared_ptr<KAnimation> Anim, float ParameterValueX, float ParameterValueY)
{
    if (!Anim)
    {
        return -1;
    }

    FBlendTreeChild Child;
    Child.Animation = Anim;
    Child.ParameterValue = 0.0f;
    Child.ParameterValueX = ParameterValueX;
    Child.ParameterValueY = ParameterValueY;
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

void KBlendTree::ComputeWeights2D(float ParameterValueX, float ParameterValueY)
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

    struct F2DPoint
    {
        int Index;
        float X, Y;
    };

    std::vector<F2DPoint> Points;
    Points.reserve(Children.size());
    for (int i = 0; i < static_cast<int>(Children.size()); ++i)
    {
        Points.push_back({ i, Children[i].ParameterValueX, Children[i].ParameterValueY });
    }

    float MinWeight = std::numeric_limits<float>::max();
    for (const auto& P : Points)
    {
        float Dist = std::sqrt((P.X - ParameterValueX) * (P.X - ParameterValueX) +
                                (P.Y - ParameterValueY) * (P.Y - ParameterValueY));
        if (Dist < MinWeight)
        {
            MinWeight = Dist;
        }
    }

    if (MinWeight < 0.0001f)
    {
        for (const auto& P : Points)
        {
            float Dist = std::sqrt((P.X - ParameterValueX) * (P.X - ParameterValueX) +
                                    (P.Y - ParameterValueY) * (P.Y - ParameterValueY));
            if (Dist < 0.0001f)
            {
                Children[P.Index].BlendWeight = 1.0f;
                return;
            }
        }
    }

    if (Points.size() == 2)
    {
        float D0 = std::sqrt((Points[0].X - ParameterValueX) * (Points[0].X - ParameterValueX) +
                              (Points[0].Y - ParameterValueY) * (Points[0].Y - ParameterValueY));
        float D1 = std::sqrt((Points[1].X - ParameterValueX) * (Points[1].X - ParameterValueX) +
                              (Points[1].Y - ParameterValueY) * (Points[1].Y - ParameterValueY));
        float TotalDist = D0 + D1;
        if (TotalDist < 0.0001f)
        {
            Children[Points[0].Index].BlendWeight = 0.5f;
            Children[Points[1].Index].BlendWeight = 0.5f;
        }
        else
        {
            Children[Points[0].Index].BlendWeight = D1 / TotalDist;
            Children[Points[1].Index].BlendWeight = D0 / TotalDist;
        }
        return;
    }

    int BestTriangle[3] = { -1, -1, -1 };
    float BestArea = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(Points.size()); ++i)
    {
        for (int j = i + 1; j < static_cast<int>(Points.size()); ++j)
        {
            for (int k = j + 1; k < static_cast<int>(Points.size()); ++k)
            {
                float ax = Points[j].X - Points[i].X;
                float ay = Points[j].Y - Points[i].Y;
                float bx = Points[k].X - Points[i].X;
                float by = Points[k].Y - Points[i].Y;
                float Cross = ax * by - ay * bx;
                float Area = std::abs(Cross) * 0.5f;

                float d0 = std::sqrt((Points[i].X - ParameterValueX) * (Points[i].X - ParameterValueX) +
                                      (Points[i].Y - ParameterValueY) * (Points[i].Y - ParameterValueY));
                float d1 = std::sqrt((Points[j].X - ParameterValueX) * (Points[j].X - ParameterValueX) +
                                      (Points[j].Y - ParameterValueY) * (Points[j].Y - ParameterValueY));
                float d2 = std::sqrt((Points[k].X - ParameterValueX) * (Points[k].X - ParameterValueX) +
                                      (Points[k].Y - ParameterValueY) * (Points[k].Y - ParameterValueY));
                float Score = Area + d0 + d1 + d2;

                if (Score < BestArea)
                {
                    BestArea = Score;
                    BestTriangle[0] = Points[i].Index;
                    BestTriangle[1] = Points[j].Index;
                    BestTriangle[2] = Points[k].Index;
                }
            }
        }
    }

    if (BestTriangle[0] >= 0 && BestTriangle[1] >= 0 && BestTriangle[2] >= 0)
    {
        float x1 = Children[BestTriangle[0]].ParameterValueX;
        float y1 = Children[BestTriangle[0]].ParameterValueY;
        float x2 = Children[BestTriangle[1]].ParameterValueX;
        float y2 = Children[BestTriangle[1]].ParameterValueY;
        float x3 = Children[BestTriangle[2]].ParameterValueX;
        float y3 = Children[BestTriangle[2]].ParameterValueY;

        float detT = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);
        if (std::abs(detT) > 0.0001f)
        {
            float w1 = ((y2 - y3) * (ParameterValueX - x3) + (x3 - x2) * (ParameterValueY - y3)) / detT;
            float w2 = ((y3 - y1) * (ParameterValueX - x3) + (x1 - x3) * (ParameterValueY - y3)) / detT;
            float w3 = 1.0f - w1 - w2;

            Children[BestTriangle[0]].BlendWeight = Clamp(w1, 0.0f, 1.0f);
            Children[BestTriangle[1]].BlendWeight = Clamp(w2, 0.0f, 1.0f);
            Children[BestTriangle[2]].BlendWeight = Clamp(w3, 0.0f, 1.0f);
        }
        else
        {
            float D0 = std::sqrt((x1 - ParameterValueX) * (x1 - ParameterValueX) + (y1 - ParameterValueY) * (y1 - ParameterValueY));
            float D1 = std::sqrt((x2 - ParameterValueX) * (x2 - ParameterValueX) + (y2 - ParameterValueY) * (y2 - ParameterValueY));
            float D2 = std::sqrt((x3 - ParameterValueX) * (x3 - ParameterValueX) + (y3 - ParameterValueY) * (y3 - ParameterValueY));
            float TotalDist = D0 + D1 + D2;
            if (TotalDist > 0.0001f)
            {
                Children[BestTriangle[0]].BlendWeight = D1 / TotalDist + D2 / TotalDist;
                Children[BestTriangle[1]].BlendWeight = D0 / TotalDist + D2 / TotalDist;
                Children[BestTriangle[2]].BlendWeight = D0 / TotalDist + D1 / TotalDist;
            }
        }
    }
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
            XMMATRIX invBindPose = XMLoadFloat4x4(&Bone->InverseBindPose);
            OutMatrices[BoneIdx] = invBindPose * OutMatrices[BoneIdx];
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
        XMVECTOR AccRot = XMQuaternionIdentity();
        XMVECTOR AccTrans = XMVectorSet(0, 0, 0, 0);
        float TotalWeight = 0.0f;

        bool bFirst = true;
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

            float rotDot4 = XMVectorGetX(XMVector4Dot(AccRot, R));
            if (rotDot4 < 0.0f)
            {
                R = XMVectorNegate(R);
            }

            if (bFirst)
            {
                AccRot = XMVectorScale(R, W);
                bFirst = false;
            }
            else
            {
                AccRot = XMVectorAdd(AccRot, XMVectorScale(R, W));
            }

            TotalWeight += W;
        }

        if (TotalWeight > 0.0001f)
        {
            AccScale = XMVectorScale(AccScale, 1.0f / TotalWeight);
            AccTrans = XMVectorScale(AccTrans, 1.0f / TotalWeight);
            AccRot = XMQuaternionNormalize(AccRot);

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

void KBlendTree::Update2D(float DeltaTime, float ParameterValueX, float ParameterValueY)
{
    if (!Skeleton || Children.empty())
    {
        return;
    }

    ComputeWeights2D(ParameterValueX, ParameterValueY);

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
