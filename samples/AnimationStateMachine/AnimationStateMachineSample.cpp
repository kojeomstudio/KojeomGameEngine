/**
 * @file AnimationStateMachineSample.cpp
 * @brief Animation State Machine Sample demonstrating state-based animation
 * 
 * This sample demonstrates:
 * - Multiple animation states (Idle, Walk, Run)
 * - State transitions with conditions
 * - Parameter-based state changes
 * - Smooth animation blending
 */

#include "../../Engine/Core/Engine.h"
#include "../../Engine/Assets/SkeletalMeshComponent.h"
#include "../../Engine/Assets/Skeleton.h"
#include "../../Engine/Assets/Animation.h"
#include "../../Engine/Assets/AnimationInstance.h"
#include "../../Engine/Assets/AnimationStateMachine.h"
#include "../../Engine/Input/InputManager.h"
#include <cmath>

class AnimationStateMachineSample : public KEngine
{
public:
    AnimationStateMachineSample() = default;
    ~AnimationStateMachineSample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        if (!renderer)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        auto graphicsDevice = GetGraphicsDevice();
        if (!graphicsDevice)
        {
            LOG_ERROR("GraphicsDevice not initialized");
            return E_FAIL;
        }

        m_cubeMesh = renderer->CreateCubeMesh();
        m_sphereMesh = renderer->CreateSphereMesh(16, 8);

        if (!m_cubeMesh || !m_sphereMesh)
        {
            LOG_ERROR("Failed to create meshes");
            return E_FAIL;
        }

        FDirectionalLight dirLight;
        dirLight.Direction = XMFLOAT3(0.5f, -1.0f, 0.3f);
        dirLight.Color = XMFLOAT4(1.0f, 1.0f, 0.95f, 1.0f);
        dirLight.AmbientColor = XMFLOAT4(0.15f, 0.15f, 0.2f, 1.0f);
        renderer->SetDirectionalLight(dirLight);

        FPointLight pointLight;
        pointLight.Position = XMFLOAT3(5.0f, 5.0f, -5.0f);
        pointLight.Color = XMFLOAT4(0.8f, 0.8f, 1.0f, 1.0f);
        pointLight.Intensity = 3.0f;
        pointLight.Radius = 20.0f;
        renderer->AddPointLight(pointLight);

        renderer->SetShadowEnabled(true);
        renderer->SetShadowSceneBounds(XMFLOAT3(0.0f, 2.0f, 0.0f), 15.0f);

        HRESULT hr = CreateBoneBuffer(graphicsDevice->GetDevice());
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create bone buffer");
            return hr;
        }

        hr = CreateProceduralSkeletalMesh();
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create procedural skeletal mesh");
            return hr;
        }

        m_skinnedShader = renderer->GetSkinnedShader();
        if (!m_skinnedShader)
        {
            LOG_ERROR("Failed to get skinned shader");
            return E_FAIL;
        }

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(0.0f, 3.0f, -8.0f));
            camera->LookAt(XMFLOAT3(0.0f, 1.5f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        LOG_INFO("Animation State Machine Sample initialized");
        LOG_INFO("Controls: WASD to move, Shift to run, 1/2/3 to force states");
        return S_OK;
    }

    HRESULT CreateBoneBuffer(ID3D11Device* Device)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(FBoneMatrixBuffer);
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = Device->CreateBuffer(&desc, nullptr, &m_boneBuffer);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create bone matrix buffer");
            return hr;
        }
        return S_OK;
    }

    HRESULT CreateProceduralSkeletalMesh()
    {
        auto renderer = GetRenderer();
        auto device = GetGraphicsDevice()->GetDevice();

        m_skeleton = std::make_shared<KSkeleton>();

        FBone rootBone;
        rootBone.Name = "Root";
        rootBone.ParentIndex = INVALID_BONE_INDEX;
        rootBone.LocalPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
        rootBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rootBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rootIdx = m_skeleton->AddBone(rootBone);

        FBone spineBone;
        spineBone.Name = "Spine";
        spineBone.ParentIndex = rootIdx;
        spineBone.LocalPosition = XMFLOAT3(0.0f, 1.0f, 0.0f);
        spineBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        spineBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 spineIdx = m_skeleton->AddBone(spineBone);

        FBone headBone;
        headBone.Name = "Head";
        headBone.ParentIndex = spineIdx;
        headBone.LocalPosition = XMFLOAT3(0.0f, 0.5f, 0.0f);
        headBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        headBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 headIdx = m_skeleton->AddBone(headBone);

        FBone leftArmBone;
        leftArmBone.Name = "LeftArm";
        leftArmBone.ParentIndex = spineIdx;
        leftArmBone.LocalPosition = XMFLOAT3(-0.3f, 0.3f, 0.0f);
        XMFLOAT3 leftArmAxis(0, 0, 1);
        XMStoreFloat4(&leftArmBone.LocalRotation, XMQuaternionRotationAxis(XMLoadFloat3(&leftArmAxis), 0.4f));
        leftArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftArmIdx = m_skeleton->AddBone(leftArmBone);

        FBone leftForearmBone;
        leftForearmBone.Name = "LeftForearm";
        leftForearmBone.ParentIndex = leftArmIdx;
        leftForearmBone.LocalPosition = XMFLOAT3(-0.4f, 0.0f, 0.0f);
        leftForearmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftForearmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftForearmIdx = m_skeleton->AddBone(leftForearmBone);

        FBone rightArmBone;
        rightArmBone.Name = "RightArm";
        rightArmBone.ParentIndex = spineIdx;
        rightArmBone.LocalPosition = XMFLOAT3(0.3f, 0.3f, 0.0f);
        XMFLOAT3 rightArmAxis(0, 0, -1);
        XMStoreFloat4(&rightArmBone.LocalRotation, XMQuaternionRotationAxis(XMLoadFloat3(&rightArmAxis), 0.4f));
        rightArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightArmIdx = m_skeleton->AddBone(rightArmBone);

        FBone rightForearmBone;
        rightForearmBone.Name = "RightForearm";
        rightForearmBone.ParentIndex = rightArmIdx;
        rightForearmBone.LocalPosition = XMFLOAT3(0.4f, 0.0f, 0.0f);
        rightForearmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightForearmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightForearmIdx = m_skeleton->AddBone(rightForearmBone);

        FBone leftLegBone;
        leftLegBone.Name = "LeftLeg";
        leftLegBone.ParentIndex = rootIdx;
        leftLegBone.LocalPosition = XMFLOAT3(-0.15f, 0.0f, 0.0f);
        leftLegBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftLegBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftLegIdx = m_skeleton->AddBone(leftLegBone);

        FBone leftCalfBone;
        leftCalfBone.Name = "LeftCalf";
        leftCalfBone.ParentIndex = leftLegIdx;
        leftCalfBone.LocalPosition = XMFLOAT3(0.0f, -0.5f, 0.0f);
        leftCalfBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftCalfBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftCalfIdx = m_skeleton->AddBone(leftCalfBone);

        FBone rightLegBone;
        rightLegBone.Name = "RightLeg";
        rightLegBone.ParentIndex = rootIdx;
        rightLegBone.LocalPosition = XMFLOAT3(0.15f, 0.0f, 0.0f);
        rightLegBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightLegBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightLegIdx = m_skeleton->AddBone(rightLegBone);

        FBone rightCalfBone;
        rightCalfBone.Name = "RightCalf";
        rightCalfBone.ParentIndex = rightLegIdx;
        rightCalfBone.LocalPosition = XMFLOAT3(0.0f, -0.5f, 0.0f);
        rightCalfBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightCalfBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_skeleton->AddBone(rightCalfBone);

        m_skeleton->CalculateBindPoses();
        m_skeleton->CalculateInverseBindPoses();

        std::vector<FSkinnedVertex> vertices;
        std::vector<uint32> indices;
        CreateBodyVertices(vertices, indices);

        m_skeletalMesh = std::make_shared<KSkeletalMesh>();
        HRESULT hr = m_skeletalMesh->CreateFromData(device, vertices, indices);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create skeletal mesh from data");
            return hr;
        }

        m_idleAnimation = CreateIdleAnimation();
        m_walkAnimation = CreateWalkAnimation();
        m_runAnimation = CreateRunAnimation();

        SetupStateMachine();

        return S_OK;
    }

    void CreateBodyVertices(std::vector<FSkinnedVertex>& vertices, std::vector<uint32>& indices)
    {
        auto createBoneInfluencedCube = [&](XMFLOAT3 center, XMFLOAT3 size, uint32 boneIndex, XMFLOAT4 color) {
            uint32 baseIndex = static_cast<uint32>(vertices.size());

            float hw = size.x * 0.5f;
            float hh = size.y * 0.5f;
            float hd = size.z * 0.5f;

            FSkinnedVertex v;
            v.Color = color;
            for (uint32 i = 0; i < MAX_BONE_INFLUENCES; ++i)
            {
                v.BoneIndices[i] = boneIndex;
                v.BoneWeights[i] = (i == 0) ? 1.0f : 0.0f;
            }

            v.Position = XMFLOAT3(center.x - hw, center.y - hh, center.z + hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
            v.TexCoord = XMFLOAT2(0.0f, 1.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x + hw, center.y - hh, center.z + hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
            v.TexCoord = XMFLOAT2(1.0f, 1.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x + hw, center.y + hh, center.z + hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
            v.TexCoord = XMFLOAT2(1.0f, 0.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x - hw, center.y + hh, center.z + hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
            v.TexCoord = XMFLOAT2(0.0f, 0.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x - hw, center.y - hh, center.z - hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            v.TexCoord = XMFLOAT2(0.0f, 1.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x + hw, center.y - hh, center.z - hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            v.TexCoord = XMFLOAT2(1.0f, 1.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x + hw, center.y + hh, center.z - hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            v.TexCoord = XMFLOAT2(1.0f, 0.0f);
            vertices.push_back(v);

            v.Position = XMFLOAT3(center.x - hw, center.y + hh, center.z - hd);
            v.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            v.TexCoord = XMFLOAT2(0.0f, 0.0f);
            vertices.push_back(v);

            uint32 cubeIndices[] = {
                0, 1, 2, 0, 2, 3,
                1, 5, 6, 1, 6, 2,
                5, 4, 7, 5, 7, 6,
                4, 0, 3, 4, 3, 7,
                3, 2, 6, 3, 6, 7,
                4, 5, 1, 4, 1, 0
            };
            for (uint32 idx : cubeIndices)
            {
                indices.push_back(baseIndex + idx);
            }
        };

        XMFLOAT4 bodyColor(0.8f, 0.6f, 0.4f, 1.0f);
        XMFLOAT4 headColor(0.9f, 0.7f, 0.5f, 1.0f);
        XMFLOAT4 armColor(0.75f, 0.55f, 0.4f, 1.0f);
        XMFLOAT4 legColor(0.7f, 0.5f, 0.35f, 1.0f);

        uint32 rootIdx = m_skeleton->GetBoneIndex("Root");
        uint32 spineIdx = m_skeleton->GetBoneIndex("Spine");
        uint32 headIdx = m_skeleton->GetBoneIndex("Head");
        uint32 leftArmIdx = m_skeleton->GetBoneIndex("LeftArm");
        uint32 leftForearmIdx = m_skeleton->GetBoneIndex("LeftForearm");
        uint32 rightArmIdx = m_skeleton->GetBoneIndex("RightArm");
        uint32 rightForearmIdx = m_skeleton->GetBoneIndex("RightForearm");
        uint32 leftLegIdx = m_skeleton->GetBoneIndex("LeftLeg");
        uint32 leftCalfIdx = m_skeleton->GetBoneIndex("LeftCalf");
        uint32 rightLegIdx = m_skeleton->GetBoneIndex("RightLeg");

        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.2f, 0.2f, 0.2f), rootIdx, bodyColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.25f, 0.0f), XMFLOAT3(0.3f, 0.5f, 0.2f), spineIdx, bodyColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.25f, 0.0f), XMFLOAT3(0.2f, 0.2f, 0.2f), headIdx, headColor);

        createBoneInfluencedCube(XMFLOAT3(-0.2f, 0.0f, 0.0f), XMFLOAT3(0.12f, 0.35f, 0.12f), leftArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(-0.2f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.35f, 0.1f), leftForearmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(0.2f, 0.0f, 0.0f), XMFLOAT3(0.12f, 0.35f, 0.12f), rightArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(0.2f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.35f, 0.1f), rightForearmIdx, armColor);

        createBoneInfluencedCube(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.15f, 0.4f, 0.15f), leftLegIdx, legColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.12f, 0.4f, 0.12f), leftCalfIdx, legColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.15f, 0.4f, 0.15f), rightLegIdx, legColor);
    }

    std::shared_ptr<KAnimation> CreateIdleAnimation()
    {
        auto animation = std::make_shared<KAnimation>();
        animation->SetName("Idle");
        animation->SetDuration(2.0f);
        animation->SetTicksPerSecond(1.0f);

        for (uint32 i = 0; i < m_skeleton->GetBoneCount(); ++i)
        {
            const FBone* bone = m_skeleton->GetBone(i);
            if (!bone) continue;

            FAnimationChannel channel;
            channel.BoneName = bone->Name;
            channel.BoneIndex = i;

            FTransformKey key;
            key.Time = 0.0f;
            key.Position = bone->LocalPosition;
            key.Rotation = bone->LocalRotation;
            key.Scale = bone->LocalScale;
            channel.PositionKeys.push_back(key);
            channel.RotationKeys.push_back(key);
            channel.ScaleKeys.push_back(key);

            key.Time = 1.0f;
            if (bone->Name == "Spine" || bone->Name == "Head")
            {
                key.Position.y += 0.02f;
            }
            channel.PositionKeys.push_back(key);
            channel.RotationKeys.push_back(key);
            channel.ScaleKeys.push_back(key);

            key.Time = 2.0f;
            key.Position = bone->LocalPosition;
            channel.PositionKeys.push_back(key);
            channel.RotationKeys.push_back(key);
            channel.ScaleKeys.push_back(key);

            animation->AddChannel(channel);
        }

        animation->BuildBoneIndexMap(m_skeleton.get());
        return animation;
    }

    std::shared_ptr<KAnimation> CreateWalkAnimation()
    {
        auto animation = std::make_shared<KAnimation>();
        animation->SetName("Walk");
        animation->SetDuration(1.0f);
        animation->SetTicksPerSecond(1.0f);

        for (uint32 i = 0; i < m_skeleton->GetBoneCount(); ++i)
        {
            const FBone* bone = m_skeleton->GetBone(i);
            if (!bone) continue;

            FAnimationChannel channel;
            channel.BoneName = bone->Name;
            channel.BoneIndex = i;

            int numKeys = 5;
            for (int k = 0; k <= numKeys; ++k)
            {
                FTransformKey key;
                key.Time = static_cast<float>(k) / numKeys;
                key.Position = bone->LocalPosition;
                key.Rotation = bone->LocalRotation;
                key.Scale = bone->LocalScale;

                float phase = key.Time * XM_2PI;

                if (bone->Name == "LeftLeg" || bone->Name == "RightLeg")
                {
                    float legPhase = (bone->Name == "LeftLeg") ? phase : (phase + XM_PI);
                    XMVECTOR rot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), sinf(legPhase) * 0.3f);
                    XMStoreFloat4(&key.Rotation, rot);
                }
                else if (bone->Name == "LeftArm" || bone->Name == "RightArm")
                {
                    float armPhase = (bone->Name == "LeftArm") ? (phase + XM_PI) : phase;
                    float swing = sinf(armPhase) * 0.2f;
                    XMVECTOR baseRot = XMLoadFloat4(&bone->LocalRotation);
                    XMVECTOR swingRot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), swing);
                    XMStoreFloat4(&key.Rotation, XMQuaternionMultiply(swingRot, baseRot));
                }
                else if (bone->Name == "Spine")
                {
                    key.Position.y += sinf(phase * 2) * 0.02f;
                }

                channel.PositionKeys.push_back(key);
                channel.RotationKeys.push_back(key);
                channel.ScaleKeys.push_back(key);
            }

            animation->AddChannel(channel);
        }

        animation->BuildBoneIndexMap(m_skeleton.get());
        return animation;
    }

    std::shared_ptr<KAnimation> CreateRunAnimation()
    {
        auto animation = std::make_shared<KAnimation>();
        animation->SetName("Run");
        animation->SetDuration(0.5f);
        animation->SetTicksPerSecond(1.0f);

        for (uint32 i = 0; i < m_skeleton->GetBoneCount(); ++i)
        {
            const FBone* bone = m_skeleton->GetBone(i);
            if (!bone) continue;

            FAnimationChannel channel;
            channel.BoneName = bone->Name;
            channel.BoneIndex = i;

            int numKeys = 5;
            for (int k = 0; k <= numKeys; ++k)
            {
                FTransformKey key;
                key.Time = static_cast<float>(k) / numKeys;
                key.Position = bone->LocalPosition;
                key.Rotation = bone->LocalRotation;
                key.Scale = bone->LocalScale;

                float phase = key.Time * XM_2PI;

                if (bone->Name == "LeftLeg" || bone->Name == "RightLeg")
                {
                    float legPhase = (bone->Name == "LeftLeg") ? phase : (phase + XM_PI);
                    XMVECTOR rot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), sinf(legPhase) * 0.5f);
                    XMStoreFloat4(&key.Rotation, rot);
                }
                else if (bone->Name == "LeftArm" || bone->Name == "RightArm")
                {
                    float armPhase = (bone->Name == "LeftArm") ? (phase + XM_PI) : phase;
                    float swing = sinf(armPhase) * 0.4f;
                    XMVECTOR baseRot = XMLoadFloat4(&bone->LocalRotation);
                    XMVECTOR swingRot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), swing);
                    XMStoreFloat4(&key.Rotation, XMQuaternionMultiply(swingRot, baseRot));
                }
                else if (bone->Name == "Spine")
                {
                    key.Position.y += fabsf(sinf(phase * 2)) * 0.05f;
                    XMVECTOR leanRot = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), 0.15f);
                    XMStoreFloat4(&key.Rotation, leanRot);
                }

                channel.PositionKeys.push_back(key);
                channel.RotationKeys.push_back(key);
                channel.ScaleKeys.push_back(key);
            }

            animation->AddChannel(channel);
        }

        animation->BuildBoneIndexMap(m_skeleton.get());
        return animation;
    }

    void SetupStateMachine()
    {
        m_stateMachine = std::make_unique<KAnimationStateMachine>();
        m_stateMachine->SetSkeleton(m_skeleton.get());

        auto* idleState = m_stateMachine->AddState("Idle", m_idleAnimation);
        idleState->SetLooping(true);
        idleState->SetSpeed(1.0f);

        auto* walkState = m_stateMachine->AddState("Walk", m_walkAnimation);
        walkState->SetLooping(true);
        walkState->SetSpeed(1.0f);

        auto* runState = m_stateMachine->AddState("Run", m_runAnimation);
        runState->SetLooping(true);
        runState->SetSpeed(1.0f);

        FAnimTransitionCondition speedCondition;
        speedCondition.ParameterName = "Speed";
        speedCondition.bIsBool = false;

        auto* idleToWalk = m_stateMachine->AddTransition("Idle", "Walk", 0.3f, false);
        speedCondition.ComparisonType = FAnimTransitionCondition::EComparisonType::Greater;
        speedCondition.CompareValue = 0.1f;
        idleToWalk->AddCondition(speedCondition);

        auto* walkToIdle = m_stateMachine->AddTransition("Walk", "Idle", 0.3f, false);
        speedCondition.ComparisonType = FAnimTransitionCondition::EComparisonType::LessOrEquals;
        speedCondition.CompareValue = 0.1f;
        walkToIdle->AddCondition(speedCondition);

        auto* walkToRun = m_stateMachine->AddTransition("Walk", "Run", 0.2f, false);
        speedCondition.ComparisonType = FAnimTransitionCondition::EComparisonType::Greater;
        speedCondition.CompareValue = 2.0f;
        walkToRun->AddCondition(speedCondition);

        auto* runToWalk = m_stateMachine->AddTransition("Run", "Walk", 0.3f, false);
        speedCondition.ComparisonType = FAnimTransitionCondition::EComparisonType::LessOrEquals;
        speedCondition.CompareValue = 2.0f;
        runToWalk->AddCondition(speedCondition);

        m_stateMachine->SetDefaultState("Idle");
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        auto input = GetInputManager();
        if (input)
        {
            input->Update();

            float moveX = 0.0f;
            float moveZ = 0.0f;

            if (input->IsKeyHeld(EKeyCode::W)) moveZ = 1.0f;
            if (input->IsKeyHeld(EKeyCode::S)) moveZ = -1.0f;
            if (input->IsKeyHeld(EKeyCode::A)) moveX = -1.0f;
            if (input->IsKeyHeld(EKeyCode::D)) moveX = 1.0f;

            bool isRunning = input->IsKeyHeld(EKeyCode::LeftShift) || input->IsKeyHeld(EKeyCode::RightShift);

            float moveLength = sqrtf(moveX * moveX + moveZ * moveZ);
            if (moveLength > 0.0f)
            {
                m_characterYaw = atan2f(moveX, moveZ);
                m_targetSpeed = isRunning ? m_maxRunSpeed : m_maxWalkSpeed;
            }
            else
            {
                m_targetSpeed = 0.0f;
            }

            if (input->IsKeyJustPressed(EKeyCode::Num1))
            {
                m_stateMachine->TriggerTransition("Idle");
            }
            if (input->IsKeyJustPressed(EKeyCode::Num2))
            {
                m_stateMachine->TriggerTransition("Walk");
            }
            if (input->IsKeyJustPressed(EKeyCode::Num3))
            {
                m_stateMachine->TriggerTransition("Run");
            }
        }

        if (m_targetSpeed > m_currentSpeed)
        {
            m_currentSpeed += m_acceleration * deltaTime;
            if (m_currentSpeed > m_targetSpeed) m_currentSpeed = m_targetSpeed;
        }
        else if (m_targetSpeed < m_currentSpeed)
        {
            m_currentSpeed -= m_deceleration * deltaTime;
            if (m_currentSpeed < m_targetSpeed) m_currentSpeed = m_targetSpeed;
        }

        if (m_currentSpeed > 0.01f)
        {
            m_characterPos.x += sinf(m_characterYaw) * m_currentSpeed * deltaTime;
            m_characterPos.z += cosf(m_characterYaw) * m_currentSpeed * deltaTime;
        }

        if (m_stateMachine)
        {
            m_stateMachine->SetFloatParameter("Speed", m_currentSpeed);
            m_stateMachine->Update(deltaTime);
        }

        m_time += deltaTime;
        m_cameraAngle += deltaTime * 10.0f;
        float camRadius = 10.0f;
        float camX = camRadius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = camRadius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 4.0f, camZ));
            camera->LookAt(XMFLOAT3(m_characterPos.x, 1.5f, m_characterPos.z), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto context = GetGraphicsDevice()->GetContext();

        XMMATRIX floorWorld = XMMatrixScaling(20.0f, 0.1f, 20.0f) *
            XMMatrixTranslation(0.0f, -0.05f, 0.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        if (m_skeletalMesh && m_skeleton && m_stateMachine && m_skinnedShader)
        {
            const std::vector<XMMATRIX>& boneMatrices = m_stateMachine->GetBoneMatrices();
            if (!boneMatrices.empty())
            {
                FBoneMatrixBuffer boneBuffer;
                memset(&boneBuffer, 0, sizeof(boneBuffer));
                uint32 numBones = (std::min)(static_cast<uint32>(boneMatrices.size()), MAX_SKINNING_BONES);
                for (uint32 i = 0; i < numBones; ++i)
                {
                    boneBuffer.BoneMatrices[i] = XMMatrixTranspose(boneMatrices[i]);
                }

                D3D11_MAPPED_SUBRESOURCE mapped;
                HRESULT hr = context->Map(m_boneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                if (SUCCEEDED(hr))
                {
                    memcpy(mapped.pData, &boneBuffer, sizeof(boneBuffer));
                    context->Unmap(m_boneBuffer.Get(), 0);
                }
            }

            XMMATRIX worldMatrix = XMMatrixRotationY(m_characterYaw) * 
                XMMatrixTranslation(m_characterPos.x, m_characterPos.y, m_characterPos.z);
            renderer->RenderSkeletalMesh(m_skeletalMesh.get(), worldMatrix, m_boneBuffer.Get());
        }

        RenderStateMachineDebug();
    }

    void RenderStateMachineDebug()
    {
        static float lastPrintTime = 0.0f;
        if (m_time - lastPrintTime > 0.5f)
        {
            lastPrintTime = m_time;
            printf("State: %s | Speed: %.2f | Blending: %s\n",
                m_stateMachine ? m_stateMachine->GetCurrentStateName().c_str() : "None",
                m_currentSpeed,
                m_stateMachine && m_stateMachine->IsTransitioning() ? "Yes" : "No");
        }
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::shared_ptr<KSkeleton> m_skeleton;
    std::shared_ptr<KSkeletalMesh> m_skeletalMesh;
    std::shared_ptr<KAnimation> m_idleAnimation;
    std::shared_ptr<KAnimation> m_walkAnimation;
    std::shared_ptr<KAnimation> m_runAnimation;
    std::unique_ptr<KAnimationStateMachine> m_stateMachine;
    KShaderProgram* m_skinnedShader = nullptr;
    ComPtr<ID3D11Buffer> m_boneBuffer;

    float m_time = 0.0f;
    float m_cameraAngle = 45.0f;

    XMFLOAT3 m_characterPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float m_characterYaw = 0.0f;
    float m_currentSpeed = 0.0f;
    float m_targetSpeed = 0.0f;
    const float m_acceleration = 5.0f;
    const float m_deceleration = 8.0f;
    const float m_maxWalkSpeed = 1.5f;
    const float m_maxRunSpeed = 4.0f;
};

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    return KEngine::RunApplication<AnimationStateMachineSample>(
        hInstance,
        L"Animation State Machine Sample - KojeomEngine",
        1280, 720,
        [](AnimationStateMachineSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
