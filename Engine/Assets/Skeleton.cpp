#include "Skeleton.h"

void FBone::Serialize(KBinaryArchive& Archive)
{
    Archive << Name;
    Archive << ParentIndex;
    Archive.WriteRaw(&BindPose, sizeof(BindPose));
    Archive.WriteRaw(&InverseBindPose, sizeof(InverseBindPose));
    Archive.WriteRaw(&LocalPosition, sizeof(LocalPosition));
    Archive.WriteRaw(&LocalRotation, sizeof(LocalRotation));
    Archive.WriteRaw(&LocalScale, sizeof(LocalScale));
}

void FBone::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Name;
    Archive >> ParentIndex;
    Archive.ReadRaw(&BindPose, sizeof(BindPose));
    Archive.ReadRaw(&InverseBindPose, sizeof(InverseBindPose));
    Archive.ReadRaw(&LocalPosition, sizeof(LocalPosition));
    Archive.ReadRaw(&LocalRotation, sizeof(LocalRotation));
    Archive.ReadRaw(&LocalScale, sizeof(LocalScale));
}

uint32 KSkeleton::AddBone(const FBone& Bone)
{
    if (Bones.size() >= MAX_BONES)
    {
        LOG_ERROR("Cannot add bone: maximum bone count (" + std::to_string(MAX_BONES) + ") reached");
        return INVALID_BONE_INDEX_U32;
    }
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
        XMMATRIX parentBindPose = XMLoadFloat4x4(&parent.BindPose);
        XMMATRIX result = localMatrix * parentBindPose;
        XMStoreFloat4x4(&bone.BindPose, result);
    }
    else
    {
        XMStoreFloat4x4(&bone.BindPose, localMatrix);
    }
}

void KSkeleton::CalculateInverseBindPoses()
{
    for (auto& bone : Bones)
    {
        XMMATRIX bindPose = XMLoadFloat4x4(&bone.BindPose);
        XMMATRIX inverseBindPose = XMMatrixInverse(nullptr, bindPose);
        XMStoreFloat4x4(&bone.InverseBindPose, inverseBindPose);
    }
}

XMMATRIX KSkeleton::GetBoneMatrix(uint32 Index) const
{
    if (Index < Bones.size())
    {
        return XMLoadFloat4x4(&Bones[Index].BindPose);
    }
    return XMMatrixIdentity();
}

void KSkeleton::SetBoneMatrix(uint32 Index, const XMMATRIX& Matrix)
{
    if (Index < Bones.size())
    {
        XMStoreFloat4x4(&Bones[Index].BindPose, Matrix);
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

    if (boneCount > MAX_BONES)
    {
        LOG_ERROR("Skeleton bone count exceeds maximum: " + std::to_string(boneCount) + " (max: " + std::to_string(MAX_BONES) + ")");
        return;
    }
    
    Bones.resize(boneCount);
    BoneNameToIndex.clear();
    
    for (uint32 i = 0; i < boneCount; ++i)
    {
        Bones[i].Deserialize(Archive);
        BoneNameToIndex[Bones[i].Name] = i;
    }
}
