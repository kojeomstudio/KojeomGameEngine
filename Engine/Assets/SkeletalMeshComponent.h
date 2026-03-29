#pragma once

#include "../Scene/Actor.h"
#include "../Assets/Skeleton.h"
#include "../Assets/Animation.h"
#include "../Assets/AnimationInstance.h"
#include "../Graphics/Shader.h"
#include <memory>
#include <vector>

constexpr uint32 MAX_BONE_INFLUENCES = 4;
constexpr uint32 MAX_SKINNING_BONES = 256;

struct FSkinnedVertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Bitangent;
    uint32 BoneIndices[MAX_BONE_INFLUENCES];
    float BoneWeights[MAX_BONE_INFLUENCES];

    FSkinnedVertex()
        : Position(0, 0, 0)
        , Color(1, 1, 1, 1)
        , Normal(0, 1, 0)
        , TexCoord(0, 0)
        , Tangent(1, 0, 0)
        , Bitangent(0, 0, 1)
    {
        for (uint32 i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            BoneIndices[i] = 0;
            BoneWeights[i] = 0.0f;
        }
    }

    void AddBoneWeight(uint32 BoneIndex, float Weight)
    {
        for (uint32 i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            if (BoneWeights[i] == 0.0f)
            {
                BoneIndices[i] = BoneIndex;
                BoneWeights[i] = Weight;
                return;
            }
        }
    }

    void NormalizeWeights()
    {
        float TotalWeight = 0.0f;
        for (uint32 i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            TotalWeight += BoneWeights[i];
        }
        if (TotalWeight > 0.0f)
        {
            for (uint32 i = 0; i < MAX_BONE_INFLUENCES; ++i)
            {
                BoneWeights[i] /= TotalWeight;
            }
        }
    }
};

struct FBoneMatrixBuffer
{
    XMMATRIX BoneMatrices[MAX_SKINNING_BONES];
};

class KSkeletalMesh
{
public:
    KSkeletalMesh() = default;
    ~KSkeletalMesh();

    KSkeletalMesh(const KSkeletalMesh&) = delete;
    KSkeletalMesh& operator=(const KSkeletalMesh&) = delete;

    HRESULT CreateFromData(ID3D11Device* Device, 
                          const std::vector<FSkinnedVertex>& Vertices,
                          const std::vector<uint32>& Indices);

    void Render(ID3D11DeviceContext* Context);

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    uint32 GetVertexCount() const { return VertexCount; }
    uint32 GetIndexCount() const { return IndexCount; }

    bool IsValid() const { return VertexBuffer != nullptr; }
    void Cleanup();

private:
    HRESULT CreateVertexBuffer(ID3D11Device* Device, const FSkinnedVertex* Vertices, UINT32 Count);
    HRESULT CreateIndexBuffer(ID3D11Device* Device, const uint32* Indices, UINT32 Count);

private:
    std::string Name;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    UINT32 VertexCount = 0;
    UINT32 IndexCount = 0;
};

class KSkeletalMeshComponent : public KActorComponent
{
public:
    KSkeletalMeshComponent();
    virtual ~KSkeletalMeshComponent();

    virtual void Tick(float DeltaTime) override;
    virtual void Render(class KRenderer* Renderer) override;

    void SetSkeletalMesh(std::shared_ptr<KSkeletalMesh> InMesh);
    std::shared_ptr<KSkeletalMesh> GetSkeletalMesh() const { return SkeletalMesh; }

    void SetSkeleton(std::shared_ptr<KSkeleton> InSkeleton);
    std::shared_ptr<KSkeleton> GetSkeleton() const { return Skeleton; }

    void PlayAnimation(const std::string& AnimationName, EAnimationPlayMode PlayMode = EAnimationPlayMode::Loop);
    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

    bool IsAnimationPlaying() const { return bAnimationPlaying; }
    float GetCurrentAnimationTime() const;
    void SetAnimationTime(float Time);

    void SetShader(std::shared_ptr<KShaderProgram> InShader) { Shader = InShader; }
    std::shared_ptr<KShaderProgram> GetShader() const { return Shader; }

    void SetCastShadow(bool bCast) { bCastShadow = bCast; }
    bool GetCastShadow() const { return bCastShadow; }

    virtual EComponentType GetComponentTypeID() const override { return EComponentType::SkeletalMesh; }

    void AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Animation);
    std::shared_ptr<KAnimation> GetAnimation(const std::string& Name) const;
    bool HasAnimation(const std::string& Name) const;

    virtual void Serialize(KBinaryArchive& Archive) override;
    virtual void Deserialize(KBinaryArchive& Archive) override;

    ID3D11Buffer* GetBoneMatrixBuffer() const { return BoneMatrixBuffer.Get(); }
    void UpdateBoneMatrices(ID3D11DeviceContext* Context);

private:
    HRESULT CreateBoneMatrixBuffer(ID3D11Device* Device);
    void ComputeBoneMatrices();

private:
    std::shared_ptr<KSkeletalMesh> SkeletalMesh;
    std::shared_ptr<KSkeleton> Skeleton;
    std::shared_ptr<KShaderProgram> Shader;
    
    std::unordered_map<std::string, std::shared_ptr<KAnimation>> Animations;
    std::shared_ptr<KAnimationInstance> AnimationInstance;
    
    ComPtr<ID3D11Buffer> BoneMatrixBuffer;
    FBoneMatrixBuffer BoneMatrices;
    
    std::string CurrentAnimationName;
    bool bAnimationPlaying = false;
    bool bCastShadow = true;
};
