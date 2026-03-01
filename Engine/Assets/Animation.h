#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include "../Serialization/BinaryArchive.h"
#include "Skeleton.h"
#include <vector>
#include <string>
#include <unordered_map>

constexpr uint32 ANIMATION_VERSION = 1;

struct FTransformKey
{
    float Time;
    XMFLOAT3 Position;
    XMFLOAT4 Rotation;
    XMFLOAT3 Scale;

    FTransformKey()
        : Time(0.0f)
        , Position(0, 0, 0)
        , Rotation(0, 0, 0, 1)
        , Scale(1, 1, 1)
    {}

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};

struct FAnimationChannel
{
    uint32 BoneIndex;
    std::string BoneName;
    std::vector<FTransformKey> PositionKeys;
    std::vector<FTransformKey> RotationKeys;
    std::vector<FTransformKey> ScaleKeys;

    FAnimationChannel() : BoneIndex(INVALID_BONE_INDEX) {}

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
    
    size_t GetPositionKeyIndex(float Time) const;
    size_t GetRotationKeyIndex(float Time) const;
    size_t GetScaleKeyIndex(float Time) const;
    
    XMFLOAT3 InterpolatePosition(float Time) const;
    XMFLOAT4 InterpolateRotation(float Time) const;
    XMFLOAT3 InterpolateScale(float Time) const;
};

class KAnimation
{
public:
    KAnimation() = default;
    ~KAnimation() = default;

    KAnimation(const KAnimation&) = delete;
    KAnimation& operator=(const KAnimation&) = delete;

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void SetDuration(float InDuration) { Duration = InDuration; }
    float GetDuration() const { return Duration; }

    void SetTicksPerSecond(float InTicksPerSecond) { TicksPerSecond = InTicksPerSecond; }
    float GetTicksPerSecond() const { return TicksPerSecond; }

    float GetDurationInSeconds() const 
    { 
        return TicksPerSecond > 0.0f ? Duration / TicksPerSecond : Duration; 
    }

    uint32 AddChannel(const FAnimationChannel& Channel);
    uint32 GetChannelCount() const { return static_cast<uint32>(Channels.size()); }
    const FAnimationChannel* GetChannel(uint32 Index) const;
    const FAnimationChannel* GetChannelByBoneIndex(uint32 BoneIndex) const;

    void BuildBoneIndexMap(const KSkeleton* Skeleton);

    HRESULT LoadFromFile(const std::wstring& Path);
    HRESULT SaveToFile(const std::wstring& Path);

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);

    bool IsValid() const { return !Channels.empty() && Duration > 0.0f; }
    void Clear() { Channels.clear(); BoneIndexToChannel.clear(); }

private:
    std::string Name;
    float Duration = 0.0f;
    float TicksPerSecond = 24.0f;
    std::vector<FAnimationChannel> Channels;
    std::unordered_map<uint32, uint32> BoneIndexToChannel;
};
