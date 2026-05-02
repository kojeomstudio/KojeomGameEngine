#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include "Skeleton.h"
#include "Animation.h"
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>

enum class EBlendTreeType
{
    BlendTree1D,
    BlendTree2D
};

struct FBlendTreeChild
{
    std::shared_ptr<KAnimation> Animation;
    float ParameterValue;
    float ParameterValueX;
    float ParameterValueY;
    float CurrentTime;
    float Speed;
    bool bLooping;
    float BlendWeight;
    std::vector<XMMATRIX> BoneMatrices;

    FBlendTreeChild()
        : ParameterValue(0.0f)
        , ParameterValueX(0.0f)
        , ParameterValueY(0.0f)
        , CurrentTime(0.0f)
        , Speed(1.0f)
        , bLooping(true)
        , BlendWeight(0.0f)
    {}
};

class KBlendTree
{
public:
    KBlendTree();
    ~KBlendTree() = default;

    KBlendTree(const KBlendTree&) = delete;
    KBlendTree& operator=(const KBlendTree&) = delete;

    void SetSkeleton(KSkeleton* InSkeleton) { Skeleton = InSkeleton; }
    KSkeleton* GetSkeleton() const { return Skeleton; }

    void SetParameterName(const std::string& InName) { ParameterName = InName; }
    const std::string& GetParameterName() const { return ParameterName; }

    void SetParameterNameY(const std::string& InName) { ParameterNameY = InName; }
    const std::string& GetParameterNameY() const { return ParameterNameY; }

    void SetType(EBlendTreeType InType) { Type = InType; }
    EBlendTreeType GetType() const { return Type; }

    int AddChild(std::shared_ptr<KAnimation> Anim, float ParameterValue);
    int AddChild2D(std::shared_ptr<KAnimation> Anim, float ParameterValueX, float ParameterValueY);
    void RemoveChild(int Index);
    int GetChildCount() const { return static_cast<int>(Children.size()); }
    const FBlendTreeChild* GetChild(int Index) const;
    FBlendTreeChild* GetChildMutable(int Index);

    void Update(float DeltaTime, float ParameterValue);
    void Update2D(float DeltaTime, float ParameterValueX, float ParameterValueY);

    const std::vector<XMMATRIX>& GetBoneMatrices() const { return BlendedBoneMatrices; }
    const XMMATRIX* GetBoneMatrixData() const { return BlendedBoneMatrices.empty() ? nullptr : BlendedBoneMatrices.data(); }
    uint32 GetBoneMatrixCount() const { return static_cast<uint32>(BlendedBoneMatrices.size()); }

    float GetChildWeight(int Index) const;

    void Reset();

private:
    void ComputeWeights(float ParameterValue);
    void ComputeWeights2D(float ParameterValueX, float ParameterValueY);
    void EvaluateChild(FBlendTreeChild& Child, std::vector<XMMATRIX>& OutMatrices);
    void BlendChildMatrices(const std::vector<std::vector<XMMATRIX>>& AllMatrices,
                            const std::vector<float>& Weights,
                            std::vector<XMMATRIX>& OutMatrices);

private:
    KSkeleton* Skeleton = nullptr;
    std::string ParameterName;
    std::string ParameterNameY;
    EBlendTreeType Type = EBlendTreeType::BlendTree1D;
    std::vector<FBlendTreeChild> Children;
    std::vector<XMMATRIX> BlendedBoneMatrices;
};
