#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include "../Serialization/BinaryArchive.h"
#include <vector>
#include <string>
#include <unordered_map>

constexpr uint32 MAX_BONES = 256;
constexpr int32 INVALID_BONE_INDEX = -1;

struct FBone
{
    std::string Name;
    int32 ParentIndex;
    XMMATRIX BindPose;
    XMMATRIX InverseBindPose;
    XMFLOAT3 LocalPosition;
    XMFLOAT4 LocalRotation;
    XMFLOAT3 LocalScale;

    FBone()
        : ParentIndex(INVALID_BONE_INDEX)
        , BindPose(XMMatrixIdentity())
        , InverseBindPose(XMMatrixIdentity())
        , LocalPosition(0, 0, 0)
        , LocalRotation(0, 0, 0, 1)
        , LocalScale(1, 1, 1)
    {}

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};

class KSkeleton
{
public:
    KSkeleton() = default;
    ~KSkeleton() = default;

    KSkeleton(const KSkeleton&) = delete;
    KSkeleton& operator=(const KSkeleton&) = delete;

    uint32 AddBone(const FBone& Bone);
    static constexpr uint32 INVALID_BONE_INDEX_U32 = static_cast<uint32>(-1);
    uint32 GetBoneCount() const { return static_cast<uint32>(Bones.size()); }
    
    int32 GetBoneIndex(const std::string& Name) const;
    const FBone* GetBone(uint32 Index) const;
    FBone* GetBoneMutable(uint32 Index);
    const FBone* GetBoneByName(const std::string& Name) const;

    const std::vector<FBone>& GetBones() const { return Bones; }
    std::vector<FBone>& GetBonesMutable() { return Bones; }

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void CalculateBindPoses();
    void CalculateInverseBindPoses();

    XMMATRIX GetBoneMatrix(uint32 Index) const;
    void SetBoneMatrix(uint32 Index, const XMMATRIX& Matrix);

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);

    bool IsValid() const { return !Bones.empty(); }
    void Clear() { Bones.clear(); BoneNameToIndex.clear(); }

private:
    void CalculateBoneBindPoseRecursive(uint32 BoneIndex);

private:
    std::string Name;
    std::vector<FBone> Bones;
    std::unordered_map<std::string, uint32> BoneNameToIndex;
};
