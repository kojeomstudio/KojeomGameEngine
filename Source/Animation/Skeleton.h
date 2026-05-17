#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Core/Log.h"

namespace Kojeom
{
struct Bone
{
    std::string name;
    int32_t parentIndex = -1;
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
};

class Skeleton
{
public:
    bool IsValid() const { return !m_bones.empty(); }
    size_t GetBoneCount() const { return m_bones.size(); }
    const Bone& GetBone(size_t index) const { return m_bones[index]; }
    const std::vector<Bone>& GetBones() const { return m_bones; }

    int32_t FindBoneIndex(const std::string& name) const
    {
        for (size_t i = 0; i < m_bones.size(); ++i)
        {
            if (m_bones[i].name == name) return static_cast<int32_t>(i);
        }
        return -1;
    }

    void AddBone(const Bone& bone)
    {
        m_bones.push_back(bone);
    }

    bool Validate() const
    {
        for (size_t i = 0; i < m_bones.size(); ++i)
        {
            if (m_bones[i].parentIndex >= static_cast<int32_t>(i) && m_bones[i].parentIndex != -1)
            {
                KE_LOG_ERROR("Bone {} has invalid parent index {}", i, m_bones[i].parentIndex);
                return false;
            }
        }
        return true;
    }

private:
    std::vector<Bone> m_bones;
};

struct BoneTransform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 ToMatrix() const
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::toMat4(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return T * R * S;
    }

    static BoneTransform Lerp(const BoneTransform& a, const BoneTransform& b, float t)
    {
        BoneTransform result;
        result.position = glm::mix(a.position, b.position, t);
        result.rotation = glm::slerp(a.rotation, b.rotation, t);
        result.scale = glm::mix(a.scale, b.scale, t);
        return result;
    }
};

class Pose
{
public:
    Pose() = default;
    explicit Pose(size_t boneCount) : m_localTransforms(boneCount), m_globalMatrices(boneCount, glm::mat4(1.0f)) {}

    void Resize(size_t boneCount)
    {
        m_localTransforms.resize(boneCount);
        m_globalMatrices.resize(boneCount, glm::mat4(1.0f));
    }

    size_t GetBoneCount() const { return m_localTransforms.size(); }

    BoneTransform& GetLocalTransform(size_t index) { return m_localTransforms[index]; }
    const BoneTransform& GetLocalTransform(size_t index) const { return m_localTransforms[index]; }
    BoneTransform& GetGlobalTransform(size_t index) { return m_globalTransformsTRS[index]; }
    const BoneTransform& GetGlobalTransform(size_t index) const { return m_globalTransformsTRS[index]; }

    void ComputeGlobalTransforms(const Skeleton& skeleton)
    {
        if (m_globalMatrices.size() != m_localTransforms.size())
            m_globalMatrices.resize(m_localTransforms.size(), glm::mat4(1.0f));

        for (size_t i = 0; i < m_localTransforms.size(); ++i)
        {
            const Bone& bone = skeleton.GetBone(i);
            glm::mat4 localMat = m_localTransforms[i].ToMatrix();
            if (bone.parentIndex == -1)
            {
                m_globalMatrices[i] = localMat;
            }
            else
            {
                m_globalMatrices[i] = m_globalMatrices[static_cast<size_t>(bone.parentIndex)] * localMat;
            }
        }

        m_globalTransformsTRS.resize(m_localTransforms.size());
        for (size_t i = 0; i < m_globalMatrices.size(); ++i)
        {
            glm::mat4& m = m_globalMatrices[i];
            m_globalTransformsTRS[i].position = glm::vec3(m[3]);
            glm::vec3 scaleX(m[0][0], m[0][1], m[0][2]);
            glm::vec3 scaleY(m[1][0], m[1][1], m[1][2]);
            glm::vec3 scaleZ(m[2][0], m[2][1], m[2][2]);
            m_globalTransformsTRS[i].scale = glm::vec3(glm::length(scaleX), glm::length(scaleY), glm::length(scaleZ));
        }
    }

    std::vector<glm::mat4> ComputeSkinMatrices(const Skeleton& skeleton) const
    {
        std::vector<glm::mat4> matrices(m_localTransforms.size());
        for (size_t i = 0; i < m_localTransforms.size(); ++i)
        {
            matrices[i] = m_globalMatrices[i] * skeleton.GetBone(i).inverseBindMatrix;
        }
        return matrices;
    }

private:
    std::vector<BoneTransform> m_localTransforms;
    std::vector<glm::mat4> m_globalMatrices;
    std::vector<BoneTransform> m_globalTransformsTRS;
};
}
