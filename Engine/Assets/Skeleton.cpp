#include "Skeleton.h"

void FBone::Serialize(KBinaryArchive& Archive)
{
    Archive << Name;
    Archive << ParentIndex;
    
    XMFLOAT4X4 bindPoseFloat4x4;
    XMStoreFloat4x4(&bindPoseFloat4x4, BindPose);
    Archive.WriteRaw(&bindPoseFloat4x4, sizeof(bindPoseFloat4x4));
    
    XMFLOAT4X4 invBindPoseFloat4x4;
    XMStoreFloat4x4(&invBindPoseFloat4x4, InverseBindPose);
    Archive.WriteRaw(&invBindPoseFloat4x4, sizeof(invBindPoseFloat4x4));
    
    Archive.WriteRaw(&LocalPosition, sizeof(LocalPosition));
    Archive.WriteRaw(&LocalRotation, sizeof(LocalRotation));
    Archive.WriteRaw(&LocalScale, sizeof(LocalScale));
}

void FBone::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Name;
    Archive >> ParentIndex;
    
    XMFLOAT4X4 bindPoseFloat4x4;
    Archive.ReadRaw(&bindPoseFloat4x4, sizeof(bindPoseFloat4x4));
    BindPose = XMLoadFloat4x4(&bindPoseFloat4x4);
    
    XMFLOAT4X4 invBindPoseFloat4x4;
    Archive.ReadRaw(&invBindPoseFloat4x4, sizeof(invBindPoseFloat4x4));
    InverseBindPose = XMLoadFloat4x4(&invBindPoseFloat4x4);
    
    Archive.ReadRaw(&LocalPosition, sizeof(LocalPosition));
    Archive.ReadRaw(&LocalRotation, sizeof(LocalRotation));
    Archive.ReadRaw(&LocalScale, sizeof(LocalScale));
}

uint32 KSkeleton::AddBone(const FBone& Bone)
{
    uint32 index = static_cast<uint32>(Bones.size());
    Bones.push_back(Bone);
    BoneNameToIndex[Bone.Name] = index;
    return index;
}

int32 KSkeleton::GetBoneIndex(const std::string& Name) const
{
    auto it = BoneNameToIndex.find(Name);
    if (it != BoneNameToIndex.end())
    {
        return static_cast<int32>(it->second);
    }
    return INVALID_BONE_INDEX;
}

const FBone* KSkeleton::GetBone(uint32 Index) const
{
    if (Index < Bones.size())
    {
        return &Bones[Index];
    }
    return nullptr;
}

FBone* KSkeleton::GetBoneMutable(uint32 Index)
{
    if (Index < Bones.size())
    {
        return &Bones[Index];
    }
    return nullptr;
}

const FBone* KSkeleton::GetBoneByName(const std::string& Name) const
{
    int32 index = GetBoneIndex(Name);
    if (index >= 0)
    {
        return &Bones[index];
    }
    return nullptr;
}

void KSkeleton::CalculateBindPoses()
{
    // Process bones in hierarchical order: roots first, then children
    // This ensures parent bind poses are computed before their children depend on them
    std::vector<bool> processed(Bones.size(), false);

    // Process root bones first (ParentIndex == INVALID_BONE_INDEX)
    for (uint32 i = 0; i < Bones.size(); ++i)
    {
        if (Bones[i].ParentIndex < 0)
        {
            CalculateBoneBindPoseRecursive(i);
            processed[i] = true;
        }
    }

    // Iteratively process child bones until all are processed
    bool bProgress = true;
    while (bProgress)
    {
        bProgress = false;
        for (uint32 i = 0; i < Bones.size(); ++i)
        {
            if (!processed[i])
            {
                int32 parentIdx = Bones[i].ParentIndex;
                if (parentIdx >= 0 && static_cast<uint32>(parentIdx) < Bones.size() && processed[parentIdx])
                {
                    CalculateBoneBindPoseRecursive(i);
                    processed[i] = true;
                    bProgress = true;
                }
            }
        }
    }
}

void KSkeleton::CalculateBoneBindPoseRecursive(uint32 BoneIndex)
{
    FBone& bone = Bones[BoneIndex];
    
    XMMATRIX localMatrix = XMMatrixScalingFromVector(XMLoadFloat3(&bone.LocalScale)) *
                           XMMatrixRotationQuaternion(XMLoadFloat4(&bone.LocalRotation)) *
                           XMMatrixTranslationFromVector(XMLoadFloat3(&bone.LocalPosition));
    
    if (bone.ParentIndex >= 0)
    {
        const FBone& parent = Bones[bone.ParentIndex];
        bone.BindPose = localMatrix * parent.BindPose;
    }
    else
    {
        bone.BindPose = localMatrix;
    }
}

void KSkeleton::CalculateInverseBindPoses()
{
    for (auto& bone : Bones)
    {
        bone.InverseBindPose = XMMatrixInverse(nullptr, bone.BindPose);
    }
}

XMMATRIX KSkeleton::GetBoneMatrix(uint32 Index) const
{
    if (Index < Bones.size())
    {
        return Bones[Index].BindPose;
    }
    return XMMatrixIdentity();
}

void KSkeleton::SetBoneMatrix(uint32 Index, const XMMATRIX& Matrix)
{
    if (Index < Bones.size())
    {
        Bones[Index].BindPose = Matrix;
    }
}

void KSkeleton::Serialize(KBinaryArchive& Archive)
{
    Archive << Name;
    uint32 boneCount = static_cast<uint32>(Bones.size());
    Archive << boneCount;
    
    for (auto& bone : Bones)
    {
        bone.Serialize(Archive);
    }
}

void KSkeleton::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Name;
    uint32 boneCount;
    Archive >> boneCount;
    
    Bones.resize(boneCount);
    BoneNameToIndex.clear();
    
    for (uint32 i = 0; i < boneCount; ++i)
    {
        Bones[i].Deserialize(Archive);
        BoneNameToIndex[Bones[i].Name] = i;
    }
}
