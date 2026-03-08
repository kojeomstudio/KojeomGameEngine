#include "../../Engine/Core/Engine.h"
#include "../../Engine/Assets/SkeletalMeshComponent.h"
#include "../../Engine/Assets/Skeleton.h"
#include "../../Engine/Assets/Animation.h"
#include "../../Engine/Assets/AnimationInstance.h"
#include <cmath>

class SkeletalMeshSample : public KEngine
{
public:
    SkeletalMeshSample() = default;
    ~SkeletalMeshSample() = default;

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

        LOG_INFO("SkeletalMesh Sample initialized");
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

        FBone chestBone;
        chestBone.Name = "Chest";
        chestBone.ParentIndex = spineIdx;
        chestBone.LocalPosition = XMFLOAT3(0.0f, 1.0f, 0.0f);
        chestBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        chestBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 chestIdx = m_skeleton->AddBone(chestBone);

        FBone headBone;
        headBone.Name = "Head";
        headBone.ParentIndex = chestIdx;
        headBone.LocalPosition = XMFLOAT3(0.0f, 0.8f, 0.0f);
        headBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        headBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_skeleton->AddBone(headBone);

        FBone leftShoulderBone;
        leftShoulderBone.Name = "LeftShoulder";
        leftShoulderBone.ParentIndex = chestIdx;
        leftShoulderBone.LocalPosition = XMFLOAT3(-0.5f, 0.5f, 0.0f);
        leftShoulderBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftShoulderBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftShoulderIdx = m_skeleton->AddBone(leftShoulderBone);

        FBone leftUpperArmBone;
        leftUpperArmBone.Name = "LeftUpperArm";
        leftUpperArmBone.ParentIndex = leftShoulderIdx;
        leftUpperArmBone.LocalPosition = XMFLOAT3(-0.3f, 0.0f, 0.0f);
        leftUpperArmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftUpperArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftUpperArmIdx = m_skeleton->AddBone(leftUpperArmBone);

        FBone leftLowerArmBone;
        leftLowerArmBone.Name = "LeftLowerArm";
        leftLowerArmBone.ParentIndex = leftUpperArmIdx;
        leftLowerArmBone.LocalPosition = XMFLOAT3(-0.8f, 0.0f, 0.0f);
        leftLowerArmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftLowerArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 leftLowerArmIdx = m_skeleton->AddBone(leftLowerArmBone);

        FBone leftHandBone;
        leftHandBone.Name = "LeftHand";
        leftHandBone.ParentIndex = leftLowerArmIdx;
        leftHandBone.LocalPosition = XMFLOAT3(-0.6f, 0.0f, 0.0f);
        leftHandBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        leftHandBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_skeleton->AddBone(leftHandBone);

        FBone rightShoulderBone;
        rightShoulderBone.Name = "RightShoulder";
        rightShoulderBone.ParentIndex = chestIdx;
        rightShoulderBone.LocalPosition = XMFLOAT3(0.5f, 0.5f, 0.0f);
        rightShoulderBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightShoulderBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightShoulderIdx = m_skeleton->AddBone(rightShoulderBone);

        FBone rightUpperArmBone;
        rightUpperArmBone.Name = "RightUpperArm";
        rightUpperArmBone.ParentIndex = rightShoulderIdx;
        rightUpperArmBone.LocalPosition = XMFLOAT3(0.3f, 0.0f, 0.0f);
        rightUpperArmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightUpperArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightUpperArmIdx = m_skeleton->AddBone(rightUpperArmBone);

        FBone rightLowerArmBone;
        rightLowerArmBone.Name = "RightLowerArm";
        rightLowerArmBone.ParentIndex = rightUpperArmIdx;
        rightLowerArmBone.LocalPosition = XMFLOAT3(0.8f, 0.0f, 0.0f);
        rightLowerArmBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightLowerArmBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        uint32 rightLowerArmIdx = m_skeleton->AddBone(rightLowerArmBone);

        FBone rightHandBone;
        rightHandBone.Name = "RightHand";
        rightHandBone.ParentIndex = rightLowerArmIdx;
        rightHandBone.LocalPosition = XMFLOAT3(0.6f, 0.0f, 0.0f);
        rightHandBone.LocalRotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        rightHandBone.LocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        m_skeleton->AddBone(rightHandBone);

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

        m_animation = CreateWaveAnimation();

        m_animationInstance = std::make_shared<KAnimationInstance>();
        m_animationInstance->SetSkeleton(m_skeleton.get());
        m_animationInstance->AddAnimation("Wave", m_animation);
        m_animationInstance->PlayAnimation("Wave", m_animation, EAnimationPlayMode::Loop);

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

        XMFLOAT4 bodyColor(0.6f, 0.4f, 0.3f, 1.0f);
        XMFLOAT4 headColor(0.7f, 0.5f, 0.4f, 1.0f);
        XMFLOAT4 armColor(0.65f, 0.45f, 0.35f, 1.0f);

        uint32 rootIdx = m_skeleton->GetBoneIndex("Root");
        uint32 spineIdx = m_skeleton->GetBoneIndex("Spine");
        uint32 chestIdx = m_skeleton->GetBoneIndex("Chest");
        uint32 headIdx = m_skeleton->GetBoneIndex("Head");
        uint32 leftUpperArmIdx = m_skeleton->GetBoneIndex("LeftUpperArm");
        uint32 leftLowerArmIdx = m_skeleton->GetBoneIndex("LeftLowerArm");
        uint32 leftHandIdx = m_skeleton->GetBoneIndex("LeftHand");
        uint32 rightUpperArmIdx = m_skeleton->GetBoneIndex("RightUpperArm");
        uint32 rightLowerArmIdx = m_skeleton->GetBoneIndex("RightLowerArm");
        uint32 rightHandIdx = m_skeleton->GetBoneIndex("RightHand");

        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.3f, 0.5f, 0.2f), rootIdx, bodyColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT3(0.4f, 0.8f, 0.25f), spineIdx, bodyColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.6f, 0.8f, 0.3f), chestIdx, bodyColor);
        createBoneInfluencedCube(XMFLOAT3(0.0f, 0.4f, 0.0f), XMFLOAT3(0.4f, 0.4f, 0.4f), headIdx, headColor);

        createBoneInfluencedCube(XMFLOAT3(-0.4f, 0.0f, 0.0f), XMFLOAT3(0.2f, 0.6f, 0.2f), leftUpperArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(-0.4f, 0.0f, 0.0f), XMFLOAT3(0.18f, 0.6f, 0.18f), leftLowerArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(-0.3f, 0.0f, 0.0f), XMFLOAT3(0.15f, 0.2f, 0.1f), leftHandIdx, armColor);

        createBoneInfluencedCube(XMFLOAT3(0.4f, 0.0f, 0.0f), XMFLOAT3(0.2f, 0.6f, 0.2f), rightUpperArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(0.4f, 0.0f, 0.0f), XMFLOAT3(0.18f, 0.6f, 0.18f), rightLowerArmIdx, armColor);
        createBoneInfluencedCube(XMFLOAT3(0.3f, 0.0f, 0.0f), XMFLOAT3(0.15f, 0.2f, 0.1f), rightHandIdx, armColor);
    }

    std::shared_ptr<KAnimation> CreateWaveAnimation()
    {
        auto animation = std::make_shared<KAnimation>();
        animation->SetName("Wave");
        animation->SetDuration(3.0f);
        animation->SetTicksPerSecond(1.0f);

        int32 rightUpperArmIdx = m_skeleton->GetBoneIndex("RightUpperArm");
        int32 rightLowerArmIdx = m_skeleton->GetBoneIndex("RightLowerArm");
        int32 headIdx = m_skeleton->GetBoneIndex("Head");

        if (rightUpperArmIdx != INVALID_BONE_INDEX)
        {
            FAnimationChannel channel;
            channel.BoneName = "RightUpperArm";
            channel.BoneIndex = rightUpperArmIdx;

            FTransformKey key1, key2, key3, key4;
            key1.Time = 0.0f;
            key1.Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

            key2.Time = 0.5f;
            XMFLOAT3 axis2(0, 0, 1);
            XMStoreFloat4(&key2.Rotation, XMQuaternionRotationAxis(XMLoadFloat3(&axis2), XM_PIDIV2));

            key3.Time = 1.5f;
            XMFLOAT3 axis3(0, 0, 1);
            XMStoreFloat4(&key3.Rotation, XMQuaternionRotationAxis(XMLoadFloat3(&axis3), XM_PIDIV2 + 0.3f));

            key4.Time = 2.5f;
            key4.Rotation = key2.Rotation;

            channel.RotationKeys.push_back(key1);
            channel.RotationKeys.push_back(key2);
            channel.RotationKeys.push_back(key3);
            channel.RotationKeys.push_back(key4);

            animation->AddChannel(channel);
        }

        if (rightLowerArmIdx != INVALID_BONE_INDEX)
        {
            FAnimationChannel channel;
            channel.BoneName = "RightLowerArm";
            channel.BoneIndex = rightLowerArmIdx;

            FTransformKey key1, key2;
            key1.Time = 0.0f;
            key1.Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

            key2.Time = 0.5f;
            XMFLOAT3 axis2(0, 1, 0);
            XMStoreFloat4(&key2.Rotation, XMQuaternionRotationAxis(XMLoadFloat3(&axis2), -XM_PIDIV4));

            channel.RotationKeys.push_back(key1);
            channel.RotationKeys.push_back(key2);

            animation->AddChannel(channel);
        }

        if (headIdx != INVALID_BONE_INDEX)
        {
            FAnimationChannel channel;
            channel.BoneName = "Head";
            channel.BoneIndex = headIdx;

            FTransformKey key1, key2, key3;
            key1.Time = 0.0f;
            key1.Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

            key2.Time = 1.5f;
            XMFLOAT3 axis2(0, 1, 0);
            XMStoreFloat4(&key2.Rotation, XMQuaternionRotationAxis(XMLoadFloat3(&axis2), 0.2f));

            key3.Time = 3.0f;
            key3.Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

            channel.RotationKeys.push_back(key1);
            channel.RotationKeys.push_back(key2);
            channel.RotationKeys.push_back(key3);

            animation->AddChannel(channel);
        }

        animation->BuildBoneIndexMap(m_skeleton.get());

        return animation;
    }

    void Update(float deltaTime) override
    {
        KEngine::Update(deltaTime);

        m_time += deltaTime;

        if (m_animationInstance)
        {
            m_animationInstance->Update(deltaTime);
        }

        m_cameraAngle += deltaTime * 10.0f;
        float camRadius = 10.0f;
        float camX = camRadius * sinf(XMConvertToRadians(m_cameraAngle));
        float camZ = camRadius * cosf(XMConvertToRadians(m_cameraAngle));

        auto camera = GetCamera();
        if (camera)
        {
            camera->SetPosition(XMFLOAT3(camX, 4.0f, camZ));
            camera->LookAt(XMFLOAT3(0.0f, 1.5f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

protected:
    virtual void RenderFrame_Internal() override
    {
        auto renderer = GetRenderer();
        auto context = GetGraphicsDevice()->GetContext();

        XMMATRIX floorWorld = XMMatrixScaling(12.0f, 0.1f, 12.0f) *
            XMMatrixTranslation(0.0f, -0.05f, 0.0f);
        auto checkerTexture = renderer->GetTextureManager()->GetCheckerboardTexture();
        renderer->RenderMeshLit(m_cubeMesh, floorWorld, checkerTexture);

        if (m_skeletalMesh && m_skeleton && m_animationInstance && m_skinnedShader)
        {
            m_animationInstance->UpdateBoneMatrices();

            const std::vector<XMMATRIX>& boneMatrices = m_animationInstance->GetBoneMatrices();
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

            XMMATRIX worldMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
            renderer->RenderSkeletalMesh(m_skeletalMesh.get(), worldMatrix, m_boneBuffer.Get());
        }

        RenderSkeletonDebug();
    }

    void RenderSkeletonDebug()
    {
        auto renderer = GetRenderer();
        if (!m_skeleton || !renderer)
            return;

        const std::vector<FBone>& bones = m_skeleton->GetBones();
        std::vector<XMMATRIX> boneWorldMatrices(bones.size());

        const std::vector<XMMATRIX>& animMatrices = m_animationInstance->GetBoneMatrices();

        for (size_t i = 0; i < bones.size(); ++i)
        {
            if (animMatrices.size() > i)
            {
                boneWorldMatrices[i] = animMatrices[i];
            }
            else
            {
                boneWorldMatrices[i] = m_skeleton->GetBoneMatrix(static_cast<uint32>(i));
            }
        }

        for (size_t i = 0; i < bones.size(); ++i)
        {
            const FBone& bone = bones[i];

            XMFLOAT3 bonePos;
            XMStoreFloat3(&bonePos, boneWorldMatrices[i].r[3]);

            XMMATRIX sphereWorld = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(bonePos.x, bonePos.y, bonePos.z);
            renderer->RenderMeshLit(m_sphereMesh, sphereWorld);

            if (bone.ParentIndex != INVALID_BONE_INDEX)
            {
                XMFLOAT3 parentPos;
                XMStoreFloat3(&parentPos, boneWorldMatrices[bone.ParentIndex].r[3]);

                XMFLOAT3 midPoint(
                    (bonePos.x + parentPos.x) * 0.5f,
                    (bonePos.y + parentPos.y) * 0.5f,
                    (bonePos.z + parentPos.z) * 0.5f
                );

                float dx = bonePos.x - parentPos.x;
                float dy = bonePos.y - parentPos.y;
                float dz = bonePos.z - parentPos.z;
                float length = sqrtf(dx * dx + dy * dy + dz * dz);

                XMMATRIX boneLineWorld = XMMatrixScaling(0.05f, length * 0.5f, 0.05f) *
                    XMMatrixTranslation(midPoint.x, midPoint.y, midPoint.z);
                renderer->RenderMeshLit(m_cubeMesh, boneLineWorld);
            }
        }
    }

private:
    std::shared_ptr<KMesh> m_cubeMesh;
    std::shared_ptr<KMesh> m_sphereMesh;
    std::shared_ptr<KSkeleton> m_skeleton;
    std::shared_ptr<KSkeletalMesh> m_skeletalMesh;
    std::shared_ptr<KAnimation> m_animation;
    std::shared_ptr<KAnimationInstance> m_animationInstance;
    KShaderProgram* m_skinnedShader = nullptr;
    ComPtr<ID3D11Buffer> m_boneBuffer;

    float m_time = 0.0f;
    float m_cameraAngle = 45.0f;
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

    return KEngine::RunApplication<SkeletalMeshSample>(
        hInstance,
        L"SkeletalMesh Sample - KojeomEngine",
        1280, 720,
        [](SkeletalMeshSample* app) -> HRESULT {
            return app->InitializeSample();
        }
    );
}
