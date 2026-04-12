#include "Animation.h"
#include <algorithm>

namespace
{
    template<typename T>
    T Clamp(T Value, T Min, T Max)
    {
        return Value < Min ? Min : (Value > Max ? Max : Value);
    }
}

void FTransformKey::Serialize(KBinaryArchive& Archive)
{
    Archive << Time;
    Archive.WriteRaw(&Position, sizeof(Position));
    Archive.WriteRaw(&Rotation, sizeof(Rotation));
    Archive.WriteRaw(&Scale, sizeof(Scale));
}

void FTransformKey::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Time;
    Archive.ReadRaw(&Position, sizeof(Position));
    Archive.ReadRaw(&Rotation, sizeof(Rotation));
    Archive.ReadRaw(&Scale, sizeof(Scale));
}

void FAnimationChannel::Serialize(KBinaryArchive& Archive)
{
    Archive << BoneIndex;
    Archive << BoneName;
    
    uint32 posKeyCount = static_cast<uint32>(PositionKeys.size());
    uint32 rotKeyCount = static_cast<uint32>(RotationKeys.size());
    uint32 scaleKeyCount = static_cast<uint32>(ScaleKeys.size());
    
    Archive << posKeyCount << rotKeyCount << scaleKeyCount;
    
    for (auto& key : PositionKeys) key.Serialize(Archive);
    for (auto& key : RotationKeys) key.Serialize(Archive);
    for (auto& key : ScaleKeys) key.Serialize(Archive);
}

void FAnimationChannel::Deserialize(KBinaryArchive& Archive)
{
    Archive >> BoneIndex;
    Archive >> BoneName;
    
    uint32 posKeyCount, rotKeyCount, scaleKeyCount;
    Archive >> posKeyCount >> rotKeyCount >> scaleKeyCount;
    
    PositionKeys.resize(posKeyCount);
    RotationKeys.resize(rotKeyCount);
    ScaleKeys.resize(scaleKeyCount);
    
    for (auto& key : PositionKeys) key.Deserialize(Archive);
    for (auto& key : RotationKeys) key.Deserialize(Archive);
    for (auto& key : ScaleKeys) key.Deserialize(Archive);
}

size_t FAnimationChannel::GetPositionKeyIndex(float Time) const
{
    if (PositionKeys.empty()) return 0;
    if (PositionKeys.size() == 1) return 0;
    
    auto it = std::upper_bound(PositionKeys.begin(), PositionKeys.end(), Time,
        [](float t, const FTransformKey& key) { return t < key.Time; });
    if (it == PositionKeys.end()) return PositionKeys.size() - 1;
    size_t idx = static_cast<size_t>(std::distance(PositionKeys.begin(), it));
    return idx > 0 ? idx - 1 : 0;
}

size_t FAnimationChannel::GetRotationKeyIndex(float Time) const
{
    if (RotationKeys.empty()) return 0;
    if (RotationKeys.size() == 1) return 0;
    
    auto it = std::upper_bound(RotationKeys.begin(), RotationKeys.end(), Time,
        [](float t, const FTransformKey& key) { return t < key.Time; });
    if (it == RotationKeys.end()) return RotationKeys.size() - 1;
    size_t idx = static_cast<size_t>(std::distance(RotationKeys.begin(), it));
    return idx > 0 ? idx - 1 : 0;
}

size_t FAnimationChannel::GetScaleKeyIndex(float Time) const
{
    if (ScaleKeys.empty()) return 0;
    if (ScaleKeys.size() == 1) return 0;
    
    auto it = std::upper_bound(ScaleKeys.begin(), ScaleKeys.end(), Time,
        [](float t, const FTransformKey& key) { return t < key.Time; });
    if (it == ScaleKeys.end()) return ScaleKeys.size() - 1;
    size_t idx = static_cast<size_t>(std::distance(ScaleKeys.begin(), it));
    return idx > 0 ? idx - 1 : 0;
}

XMFLOAT3 FAnimationChannel::InterpolatePosition(float Time) const
{
    if (PositionKeys.empty()) return XMFLOAT3(0, 0, 0);
    if (PositionKeys.size() == 1) return PositionKeys[0].Position;
    
    size_t index = GetPositionKeyIndex(Time);
    size_t nextIndex = index + 1;
    
    if (nextIndex >= PositionKeys.size())
        return PositionKeys[index].Position;
    
    float keyDuration = PositionKeys[nextIndex].Time - PositionKeys[index].Time;
    float factor = (keyDuration > 0.0f) ? 
        (Time - PositionKeys[index].Time) / keyDuration : 0.0f;
    factor = Clamp(factor, 0.0f, 1.0f);
    
    XMVECTOR pos1 = XMLoadFloat3(&PositionKeys[index].Position);
    XMVECTOR pos2 = XMLoadFloat3(&PositionKeys[nextIndex].Position);
    XMVECTOR result = XMVectorLerp(pos1, pos2, factor);
    
    XMFLOAT3 outPos;
    XMStoreFloat3(&outPos, result);
    return outPos;
}

XMFLOAT4 FAnimationChannel::InterpolateRotation(float Time) const
{
    if (RotationKeys.empty()) return XMFLOAT4(0, 0, 0, 1);
    if (RotationKeys.size() == 1) return RotationKeys[0].Rotation;
    
    size_t index = GetRotationKeyIndex(Time);
    size_t nextIndex = index + 1;
    
    if (nextIndex >= RotationKeys.size())
        return RotationKeys[index].Rotation;
    
    float keyDuration = RotationKeys[nextIndex].Time - RotationKeys[index].Time;
    float factor = (keyDuration > 0.0f) ? 
        (Time - RotationKeys[index].Time) / keyDuration : 0.0f;
    factor = Clamp(factor, 0.0f, 1.0f);
    
    XMVECTOR rot1 = XMLoadFloat4(&RotationKeys[index].Rotation);
    XMVECTOR rot2 = XMLoadFloat4(&RotationKeys[nextIndex].Rotation);
    
    if (XMVectorGetX(XMVector4Dot(rot1, rot2)) < 0.0f)
    {
        rot2 = XMVectorNegate(rot2);
    }
    
    XMVECTOR result = XMQuaternionSlerp(rot1, rot2, factor);
    
    XMFLOAT4 outRot;
    XMStoreFloat4(&outRot, result);
    return outRot;
}

XMFLOAT3 FAnimationChannel::InterpolateScale(float Time) const
{
    if (ScaleKeys.empty()) return XMFLOAT3(1, 1, 1);
    if (ScaleKeys.size() == 1) return ScaleKeys[0].Scale;
    
    size_t index = GetScaleKeyIndex(Time);
    size_t nextIndex = index + 1;
    
    if (nextIndex >= ScaleKeys.size())
        return ScaleKeys[index].Scale;
    
    float keyDuration = ScaleKeys[nextIndex].Time - ScaleKeys[index].Time;
    float factor = (keyDuration > 0.0f) ? 
        (Time - ScaleKeys[index].Time) / keyDuration : 0.0f;
    factor = Clamp(factor, 0.0f, 1.0f);
    
    XMVECTOR scale1 = XMLoadFloat3(&ScaleKeys[index].Scale);
    XMVECTOR scale2 = XMLoadFloat3(&ScaleKeys[nextIndex].Scale);
    XMVECTOR result = XMVectorLerp(scale1, scale2, factor);
    
    XMFLOAT3 outScale;
    XMStoreFloat3(&outScale, result);
    return outScale;
}

uint32 KAnimation::AddChannel(const FAnimationChannel& Channel)
{
    uint32 index = static_cast<uint32>(Channels.size());
    Channels.push_back(Channel);
    return index;
}

const FAnimationChannel* KAnimation::GetChannel(uint32 Index) const
{
    if (Index < Channels.size())
    {
        return &Channels[Index];
    }
    return nullptr;
}

const FAnimationChannel* KAnimation::GetChannelByBoneIndex(uint32 BoneIndex) const
{
    auto it = BoneIndexToChannel.find(BoneIndex);
    if (it != BoneIndexToChannel.end())
    {
        return &Channels[it->second];
    }
    return nullptr;
}

void KAnimation::BuildBoneIndexMap(const KSkeleton* Skeleton)
{
    BoneIndexToChannel.clear();
    
    if (!Skeleton) return;
    
    for (uint32 i = 0; i < static_cast<uint32>(Channels.size()); ++i)
    {
        int32 boneIndex = Skeleton->GetBoneIndex(Channels[i].BoneName);
        if (boneIndex >= 0)
        {
            Channels[i].BoneIndex = static_cast<uint32>(boneIndex);
            BoneIndexToChannel[static_cast<uint32>(boneIndex)] = i;
        }
    }
}

HRESULT KAnimation::LoadFromFile(const std::wstring& Path)
{
    KBinaryArchive archive(KBinaryArchive::EMode::Read);
    if (!archive.Open(Path))
    {
        return E_FAIL;
    }
    
    Deserialize(archive);
    archive.Close();
    
    return S_OK;
}

HRESULT KAnimation::SaveToFile(const std::wstring& Path)
{
    KBinaryArchive archive(KBinaryArchive::EMode::Write);
    if (!archive.Open(Path))
    {
        return E_FAIL;
    }
    
    Serialize(archive);
    archive.Close();
    
    return S_OK;
}

void KAnimation::Serialize(KBinaryArchive& Archive)
{
    uint32 version = ANIMATION_VERSION;
    Archive << version;
    
    Archive << Name;
    Archive << Duration;
    Archive << TicksPerSecond;
    
    uint32 channelCount = static_cast<uint32>(Channels.size());
    Archive << channelCount;
    
    for (auto& channel : Channels)
    {
        channel.Serialize(Archive);
    }
}

void KAnimation::Deserialize(KBinaryArchive& Archive)
{
    uint32 version;
    Archive >> version;
    
    if (version != ANIMATION_VERSION)
    {
        return;
    }
    
    Archive >> Name;
    Archive >> Duration;
    Archive >> TicksPerSecond;
    
    uint32 channelCount;
    Archive >> channelCount;
    
    Channels.resize(channelCount);
    
    for (uint32 i = 0; i < channelCount; ++i)
    {
        Channels[i].Deserialize(Archive);
    }
}
