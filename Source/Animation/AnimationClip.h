#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Animation/Skeleton.h"
#include "Core/Log.h"

namespace Kojeom
{
struct VectorKey
{
    float time = 0.0f;
    glm::vec3 value = glm::vec3(0.0f);
};

struct QuatKey
{
    float time = 0.0f;
    glm::quat value = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct AnimationChannel
{
    int32_t boneIndex = -1;
    std::string boneName;
    std::vector<VectorKey> positionKeys;
    std::vector<QuatKey> rotationKeys;
    std::vector<VectorKey> scaleKeys;
};

struct AnimationEvent
{
    float time = 0.0f;
    std::string name;
    int32_t intValue = 0;
    float floatValue = 0.0f;
    std::string stringValue;
};

struct RootMotionData
{
    glm::vec3 deltaPosition = glm::vec3(0.0f);
    glm::quat deltaRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool hasRootMotion = false;
};

class AnimationClip
{
public:
    AnimationClip() = default;

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    void SetDuration(float duration) { m_duration = duration; }
    float GetDuration() const { return m_duration; }

    void SetTicksPerSecond(float tps) { m_ticksPerSecond = tps; }
    float GetTicksPerSecond() const { return m_ticksPerSecond; }

    float GetDurationSeconds() const
    {
        return (m_ticksPerSecond > 0.0f) ? m_duration / m_ticksPerSecond : m_duration;
    }

    void AddChannel(const AnimationChannel& channel) { m_channels.push_back(channel); }
    const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }
    size_t GetChannelCount() const { return m_channels.size(); }

    void AddEvent(const AnimationEvent& event) { m_events.push_back(event); }
    const std::vector<AnimationEvent>& GetEvents() const { return m_events; }

    Pose Sample(float timeSeconds, const Skeleton& skeleton) const
    {
        Pose pose(skeleton.GetBoneCount());
        float clipTime = GetDurationSeconds();

        float sampleTime = timeSeconds;
        if (sampleTime > clipTime)
        {
            sampleTime = std::fmod(sampleTime, clipTime);
        }

        for (const auto& channel : m_channels)
        {
            if (channel.boneIndex < 0 ||
                channel.boneIndex >= static_cast<int32_t>(skeleton.GetBoneCount()))
                continue;

            BoneTransform transform;

            transform.position = SampleVectorKeys(channel.positionKeys, sampleTime);
            transform.rotation = SampleQuatKeys(channel.rotationKeys, sampleTime);
            transform.scale = SampleVectorKeys(channel.scaleKeys, sampleTime);

            pose.GetLocalTransform(channel.boneIndex) = transform;
        }

        pose.ComputeGlobalTransforms(skeleton);
        return pose;
    }

    std::vector<AnimationEvent> GetEventsInRange(float prevTime, float currentTime) const
    {
        std::vector<AnimationEvent> triggered;
        for (const auto& event : m_events)
        {
            if (prevTime <= event.time && event.time < currentTime)
            {
                triggered.push_back(event);
            }
            else if (currentTime < prevTime && (event.time >= prevTime || event.time < currentTime))
            {
                triggered.push_back(event);
            }
        }
        return triggered;
    }

private:
    static glm::vec3 SampleVectorKeys(const std::vector<VectorKey>& keys, float time)
    {
        if (keys.empty()) return glm::vec3(0.0f);
        if (keys.size() == 1) return keys[0].value;

        size_t idx = 0;
        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (time >= keys[i].time && time < keys[i + 1].time)
            {
                idx = i;
                break;
            }
        }

        const VectorKey& a = keys[idx];
        const VectorKey& b = keys[std::min(idx + 1, keys.size() - 1)];

        float t = 0.0f;
        if (b.time - a.time > 0.0001f)
        {
            t = (time - a.time) / (b.time - a.time);
            t = glm::clamp(t, 0.0f, 1.0f);
        }

        return glm::mix(a.value, b.value, t);
    }

    static glm::quat SampleQuatKeys(const std::vector<QuatKey>& keys, float time)
    {
        if (keys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (keys.size() == 1) return keys[0].value;

        size_t idx = 0;
        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (time >= keys[i].time && time < keys[i + 1].time)
            {
                idx = i;
                break;
            }
        }

        const QuatKey& a = keys[idx];
        const QuatKey& b = keys[std::min(idx + 1, keys.size() - 1)];

        float t = 0.0f;
        if (b.time - a.time > 0.0001f)
        {
            t = (time - a.time) / (b.time - a.time);
            t = glm::clamp(t, 0.0f, 1.0f);
        }

        return glm::slerp(a.value, b.value, t);
    }

    std::string m_name;
    float m_duration = 0.0f;
    float m_ticksPerSecond = 30.0f;
    std::vector<AnimationChannel> m_channels;
    std::vector<AnimationEvent> m_events;
};

class Animator
{
public:
    using EventCallback = std::function<void(const AnimationEvent&)>;

    void SetSkeleton(const Skeleton* skeleton) { m_skeleton = skeleton; }
    void SetClip(const AnimationClip* clip)
    {
        m_currentClip = clip;
        m_playbackTime = 0.0f;
    }

    void Play() { m_playing = true; }
    void Pause() { m_playing = false; }
    void Stop() { m_playing = false; m_playbackTime = 0.0f; }
    void SetLoop(bool loop) { m_loop = loop; }
    void SetSpeed(float speed) { m_speed = speed; }
    void Seek(float time) { m_playbackTime = time; }
    void SetEventCallback(EventCallback callback) { m_eventCallback = callback; }
    void SetRootMotionBone(int32_t boneIndex) { m_rootMotionBone = boneIndex; }
    void SetRootMotionEnabled(bool enabled) { m_rootMotionEnabled = enabled; }

    RootMotionData GetRootMotionDelta() const { return m_rootMotionDelta; }
    bool IsRootMotionEnabled() const { return m_rootMotionEnabled; }

    void Tick(float deltaSeconds)
    {
        if (!m_playing || !m_currentClip || !m_skeleton) return;

        float prevTime = m_playbackTime;
        m_playbackTime += deltaSeconds * m_speed;

        float clipDuration = m_currentClip->GetDurationSeconds();
        if (m_playbackTime >= clipDuration)
        {
            if (m_loop)
            {
                if (m_eventCallback)
                {
                    auto events = m_currentClip->GetEventsInRange(prevTime, clipDuration);
                    for (const auto& e : events) m_eventCallback(e);
                }
                m_playbackTime = std::fmod(m_playbackTime, clipDuration);
            }
            else
            {
                m_playbackTime = clipDuration;
                m_playing = false;
            }
        }
        else
        {
            if (m_eventCallback)
            {
                auto events = m_currentClip->GetEventsInRange(prevTime, m_playbackTime);
                for (const auto& e : events) m_eventCallback(e);
            }
        }

        m_currentPose = m_currentClip->Sample(m_playbackTime, *m_skeleton);

        if (m_rootMotionEnabled && m_rootMotionBone >= 0 &&
            m_rootMotionBone < static_cast<int32_t>(m_skeleton->GetBoneCount()))
        {
            auto& boneTransform = m_currentPose.GetGlobalTransform(m_rootMotionBone);
            m_rootMotionDelta.deltaPosition = boneTransform.position - m_prevRootPosition;
            m_rootMotionDelta.deltaRotation = boneTransform.rotation * glm::inverse(m_prevRootRotation);
            m_rootMotionDelta.hasRootMotion = true;
            m_prevRootPosition = boneTransform.position;
            m_prevRootRotation = boneTransform.rotation;
        }
    }

    Pose GetCurrentPose() const { return m_currentPose; }
    float GetPlaybackTime() const { return m_playbackTime; }
    bool IsPlaying() const { return m_playing; }

    std::vector<glm::mat4> GetSkinMatrices() const
    {
        if (!m_skeleton) return {};
        return m_currentPose.ComputeSkinMatrices(*m_skeleton);
    }

private:
    const Skeleton* m_skeleton = nullptr;
    const AnimationClip* m_currentClip = nullptr;
    Pose m_currentPose;
    float m_playbackTime = 0.0f;
    float m_speed = 1.0f;
    bool m_loop = true;
    bool m_playing = false;
    EventCallback m_eventCallback;
    bool m_rootMotionEnabled = false;
    int32_t m_rootMotionBone = -1;
    glm::vec3 m_prevRootPosition = glm::vec3(0.0f);
    glm::quat m_prevRootRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    RootMotionData m_rootMotionDelta;
};

class BlendTree
{
public:
    struct BlendNode
    {
        const AnimationClip* clip = nullptr;
        float position = 0.0f;
    };

    void SetSkeleton(const Skeleton* skeleton) { m_skeleton = skeleton; }

    void AddNode(const AnimationClip* clip, float position)
    {
        m_nodes.push_back({ clip, position });
    }

    void ClearNodes() { m_nodes.clear(); }

    void Tick(float deltaSeconds)
    {
        m_playbackTime += deltaSeconds * m_speed;
    }

    void SetSpeed(float speed) { m_speed = speed; }
    float GetPlaybackTime() const { return m_playbackTime; }

    Pose Sample(float blendParam) const
    {
        if (!m_skeleton || m_nodes.empty()) return Pose();
        if (m_nodes.size() == 1)
            return m_nodes[0].clip->Sample(m_playbackTime, *m_skeleton);

        size_t lowerIdx = 0;
        for (size_t i = 0; i < m_nodes.size() - 1; ++i)
        {
            if (blendParam >= m_nodes[i].position && blendParam <= m_nodes[i + 1].position)
            {
                lowerIdx = i;
                break;
            }
        }

        size_t upperIdx = std::min(lowerIdx + 1, m_nodes.size() - 1);
        const BlendNode& lower = m_nodes[lowerIdx];
        const BlendNode& upper = m_nodes[upperIdx];

        float range = upper.position - lower.position;
        float t = (range > 0.0001f) ? (blendParam - lower.position) / range : 0.0f;
        t = glm::clamp(t, 0.0f, 1.0f);

        Pose lowerPose = lower.clip->Sample(m_playbackTime, *m_skeleton);
        Pose upperPose = upper.clip->Sample(m_playbackTime, *m_skeleton);

        Pose result(m_skeleton->GetBoneCount());
        for (size_t i = 0; i < m_skeleton->GetBoneCount(); ++i)
        {
            result.GetLocalTransform(i) = BoneTransform::Lerp(
                lowerPose.GetLocalTransform(i), upperPose.GetLocalTransform(i), t);
        }
        result.ComputeGlobalTransforms(*m_skeleton);
        return result;
    }

private:
    const Skeleton* m_skeleton = nullptr;
    std::vector<BlendNode> m_nodes;
    float m_playbackTime = 0.0f;
    float m_speed = 1.0f;
};
}
