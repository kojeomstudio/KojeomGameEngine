#include "SkeletalMeshComponent.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/GraphicsDevice.h"

KSkeletalMesh::~KSkeletalMesh()
{
    Cleanup();
}

HRESULT KSkeletalMesh::CreateFromData(ID3D11Device* Device, 
                                       const std::vector<FSkinnedVertex>& Vertices,
                                       const std::vector<uint32>& Indices)
{
    if (!Device || Vertices.empty())
    {
        return E_INVALIDARG;
    }

    HRESULT hr = CreateVertexBuffer(Device, Vertices.data(), static_cast<UINT32>(Vertices.size()));
    if (FAILED(hr))
    {
        return hr;
    }

    if (!Indices.empty())
    {
        hr = CreateIndexBuffer(Device, Indices.data(), static_cast<UINT32>(Indices.size()));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}

void KSkeletalMesh::Render(ID3D11DeviceContext* Context)
{
    if (!Context || !VertexBuffer)
    {
        return;
    }

    UINT stride = sizeof(FSkinnedVertex);
    UINT offset = 0;
    Context->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &stride, &offset);

    if (IndexBuffer && IndexCount > 0)
    {
        Context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->DrawIndexed(IndexCount, 0, 0);
    }
    else
    {
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->Draw(VertexCount, 0);
    }
}

void KSkeletalMesh::Cleanup()
{
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    VertexCount = 0;
    IndexCount = 0;
}

HRESULT KSkeletalMesh::CreateVertexBuffer(ID3D11Device* Device, const FSkinnedVertex* Vertices, UINT32 Count)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(FSkinnedVertex) * Count;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = Vertices;

    HRESULT hr = Device->CreateBuffer(&bd, &initData, &VertexBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    VertexCount = Count;
    return S_OK;
}

HRESULT KSkeletalMesh::CreateIndexBuffer(ID3D11Device* Device, const uint32* Indices, UINT32 Count)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(uint32) * Count;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = Indices;

    HRESULT hr = Device->CreateBuffer(&bd, &initData, &IndexBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    IndexCount = Count;
    return S_OK;
}

KSkeletalMeshComponent::KSkeletalMeshComponent()
{
    ZeroMemory(&BoneMatrices, sizeof(BoneMatrices));
    for (uint32 i = 0; i < MAX_SKINNING_BONES; ++i)
    {
        BoneMatrices.BoneMatrices[i] = XMMatrixIdentity();
    }
}

KSkeletalMeshComponent::~KSkeletalMeshComponent()
{
    BoneMatrixBuffer.Reset();
}

void KSkeletalMeshComponent::Tick(float DeltaTime)
{
    if (bAnimationPlaying && AnimationInstance)
    {
        AnimationInstance->Update(DeltaTime);
        ComputeBoneMatrices();
    }
}

void KSkeletalMeshComponent::Render(KRenderer* Renderer)
{
    if (!SkeletalMesh || !Renderer)
    {
        return;
    }

    KGraphicsDevice* graphicsDevice = Renderer->GetGraphicsDevice();
    if (!graphicsDevice)
    {
        return;
    }

    ID3D11DeviceContext* context = graphicsDevice->GetContext();
    if (!context)
    {
        return;
    }

    if (bAnimationPlaying && BoneMatrixBuffer)
    {
        UpdateBoneMatrices(context);
    }

    XMMATRIX worldMatrix = XMMatrixIdentity();
    if (Owner)
    {
        worldMatrix = Owner->GetWorldMatrix();
    }

    Renderer->RenderSkeletalMesh(SkeletalMesh.get(), worldMatrix, BoneMatrixBuffer.Get());
}

void KSkeletalMeshComponent::SetSkeletalMesh(std::shared_ptr<KSkeletalMesh> InMesh)
{
    SkeletalMesh = InMesh;
}

void KSkeletalMeshComponent::SetSkeleton(std::shared_ptr<KSkeleton> InSkeleton)
{
    Skeleton = InSkeleton;
    if (Skeleton)
    {
        ComputeBoneMatrices();
    }
}

void KSkeletalMeshComponent::PlayAnimation(const std::string& AnimationName, EAnimationPlayMode PlayMode)
{
    auto It = Animations.find(AnimationName);
    if (It == Animations.end())
    {
        return;
    }

    if (!AnimationInstance)
    {
        AnimationInstance = std::make_shared<KAnimationInstance>();
        if (Skeleton)
        {
            AnimationInstance->SetSkeleton(Skeleton.get());
        }
    }

    AnimationInstance->PlayAnimation(AnimationName, It->second, PlayMode);
    
    CurrentAnimationName = AnimationName;
    bAnimationPlaying = true;
}

void KSkeletalMeshComponent::StopAnimation()
{
    if (AnimationInstance)
    {
        AnimationInstance->StopAnimation();
    }
    bAnimationPlaying = false;
}

void KSkeletalMeshComponent::PauseAnimation()
{
    if (AnimationInstance)
    {
        AnimationInstance->PauseAnimation();
    }
    bAnimationPlaying = false;
}

void KSkeletalMeshComponent::ResumeAnimation()
{
    if (AnimationInstance)
    {
        AnimationInstance->ResumeAnimation();
    }
    bAnimationPlaying = true;
}

float KSkeletalMeshComponent::GetCurrentAnimationTime() const
{
    if (AnimationInstance)
    {
        return AnimationInstance->GetCurrentTime();
    }
    return 0.0f;
}

void KSkeletalMeshComponent::SetAnimationTime(float Time)
{
    if (AnimationInstance)
    {
        AnimationInstance->SetCurrentTime(Time);
    }
}

void KSkeletalMeshComponent::AddAnimation(const std::string& Name, std::shared_ptr<KAnimation> Animation)
{
    Animations[Name] = Animation;
}

std::shared_ptr<KAnimation> KSkeletalMeshComponent::GetAnimation(const std::string& Name) const
{
    auto It = Animations.find(Name);
    if (It != Animations.end())
    {
        return It->second;
    }
    return nullptr;
}

bool KSkeletalMeshComponent::HasAnimation(const std::string& Name) const
{
    return Animations.find(Name) != Animations.end();
}

void KSkeletalMeshComponent::Serialize(KBinaryArchive& Archive)
{
    Archive << CurrentAnimationName;
    Archive << bAnimationPlaying;
    Archive << bCastShadow;
}

void KSkeletalMeshComponent::Deserialize(KBinaryArchive& Archive)
{
    Archive >> CurrentAnimationName;
    Archive >> bAnimationPlaying;
    Archive >> bCastShadow;
}

HRESULT KSkeletalMeshComponent::CreateBoneMatrixBuffer(ID3D11Device* Device)
{
    if (!Device)
    {
        return E_INVALIDARG;
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(FBoneMatrixBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&bd, nullptr, &BoneMatrixBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

void KSkeletalMeshComponent::UpdateBoneMatrices(ID3D11DeviceContext* Context)
{
    if (!Context || !BoneMatrixBuffer)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Context->Map(BoneMatrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &BoneMatrices, sizeof(FBoneMatrixBuffer));
        Context->Unmap(BoneMatrixBuffer.Get(), 0);
    }
}

void KSkeletalMeshComponent::ComputeBoneMatrices()
{
    if (!Skeleton)
    {
        return;
    }

    const std::vector<FBone>& bones = Skeleton->GetBones();
    uint32 boneCount = std::min(static_cast<uint32>(bones.size()), MAX_SKINNING_BONES);

    if (AnimationInstance && AnimationInstance->IsPlaying())
    {
        const std::vector<XMMATRIX>& animBoneMatrices = AnimationInstance->GetBoneMatrices();
        uint32 animBoneCount = std::min(static_cast<uint32>(animBoneMatrices.size()), boneCount);
        
        for (uint32 i = 0; i < animBoneCount; ++i)
        {
            BoneMatrices.BoneMatrices[i] = XMMatrixTranspose(animBoneMatrices[i]);
        }
        
        for (uint32 i = animBoneCount; i < boneCount; ++i)
        {
            BoneMatrices.BoneMatrices[i] = XMMatrixTranspose(bones[i].InverseBindPose);
        }
    }
    else
    {
        for (uint32 i = 0; i < boneCount; ++i)
        {
            BoneMatrices.BoneMatrices[i] = XMMatrixTranspose(bones[i].InverseBindPose);
        }
    }

    for (uint32 i = boneCount; i < MAX_SKINNING_BONES; ++i)
    {
        BoneMatrices.BoneMatrices[i] = XMMatrixIdentity();
    }
}
