#include "InstancedRenderer.h"
#include "../Texture.h"

HRESULT KInstancedRenderer::Initialize(ID3D11Device* Device, UINT32 InMaxInstances)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for instanced renderer");
        return E_INVALIDARG;
    }

    MaxInstances = InMaxInstances;
    Instances.reserve(MaxInstances);

    HRESULT hr = CreateInstanceBuffer(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create instance buffer");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Instanced renderer initialized with max instances");
    return S_OK;
}

void KInstancedRenderer::Cleanup()
{
    InstanceBuffer.Reset();
    Instances.clear();
    Instances.shrink_to_fit();
    bInitialized = false;
}

void KInstancedRenderer::BeginFrame()
{
    ClearInstances();
}

void KInstancedRenderer::AddInstance(const XMMATRIX& WorldMatrix)
{
    if (Instances.size() < MaxInstances)
    {
        Instances.push_back(FInstanceData(WorldMatrix));
        bInstanceBufferDirty = true;
    }
}

void KInstancedRenderer::AddInstances(const std::vector<XMMATRIX>& WorldMatrices)
{
    for (const auto& Matrix : WorldMatrices)
    {
        if (Instances.size() >= MaxInstances)
        {
            break;
        }
        Instances.push_back(FInstanceData(Matrix));
    }
    bInstanceBufferDirty = true;
}

void KInstancedRenderer::ClearInstances()
{
    Instances.clear();
    bInstanceBufferDirty = true;
}

HRESULT KInstancedRenderer::CreateInstanceBuffer(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FInstanceData) * MaxInstances;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&desc, nullptr, &InstanceBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create instance buffer");
        return hr;
    }

    return S_OK;
}

HRESULT KInstancedRenderer::UpdateInstanceBuffer(ID3D11DeviceContext* Context)
{
    if (!InstanceBuffer || Instances.empty())
    {
        return S_OK;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to map instance buffer");
        return hr;
    }

    FInstanceData* data = static_cast<FInstanceData*>(mapped.pData);
    for (size_t i = 0; i < Instances.size(); ++i)
    {
        data[i].WorldMatrix = XMMatrixTranspose(Instances[i].WorldMatrix);
    }

    Context->Unmap(InstanceBuffer.Get(), 0);
    bInstanceBufferDirty = false;

    return S_OK;
}

void KInstancedRenderer::SetupInstancedInputLayout(ID3D11DeviceContext* Context, KMesh* Mesh)
{
    ID3D11Buffer* vertexBuffers[2] = {
        Mesh->GetVertexBuffer(),
        InstanceBuffer.Get()
    };

    UINT strides[2] = {
        sizeof(FVertex),
        sizeof(FInstanceData)
    };

    UINT offsets[2] = { 0, 0 };

    Context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
}

void KInstancedRenderer::Render(ID3D11DeviceContext* Context,
                                KMesh* Mesh,
                                KShaderProgram* Shader,
                                const XMMATRIX& ViewMatrix,
                                const XMMATRIX& ProjectionMatrix)
{
    if (!bInitialized || !Mesh || !Shader || Instances.empty())
    {
        return;
    }

    if (bInstanceBufferDirty)
    {
        UpdateInstanceBuffer(Context);
    }

    Shader->Bind(Context);

    SetupInstancedInputLayout(Context, Mesh);

    if (Mesh->HasIndices())
    {
        Context->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
        Context->DrawIndexedInstanced(
            Mesh->GetIndexCount(),
            static_cast<UINT>(Instances.size()),
            0, 0, 0
        );
    }
    else
    {
        Context->DrawInstanced(
            Mesh->GetVertexCount(),
            static_cast<UINT>(Instances.size()),
            0, 0
        );
    }

    ID3D11Buffer* nullBuffers[2] = { nullptr, nullptr };
    UINT nullStrides[2] = { 0, 0 };
    UINT nullOffsets[2] = { 0, 0 };
    Context->IASetVertexBuffers(0, 2, nullBuffers, nullStrides, nullOffsets);

    Shader->Unbind(Context);
}

void KInstancedRenderer::RenderWithTexture(ID3D11DeviceContext* Context,
                                           KMesh* Mesh,
                                           KShaderProgram* Shader,
                                           KTexture* Texture,
                                           const XMMATRIX& ViewMatrix,
                                           const XMMATRIX& ProjectionMatrix)
{
    if (Texture)
    {
        Texture->Bind(Context, 0);
    }

    Render(Context, Mesh, Shader, ViewMatrix, ProjectionMatrix);

    if (Texture)
    {
        Texture->Unbind(Context, 0);
    }
}

HRESULT KInstancedRenderer::SetMaxInstances(ID3D11Device* Device, UINT32 NewMaxInstances)
{
    if (NewMaxInstances == 0)
    {
        return E_INVALIDARG;
    }

    MaxInstances = NewMaxInstances;
    Instances.clear();
    Instances.reserve(MaxInstances);

    InstanceBuffer.Reset();
    return CreateInstanceBuffer(Device);
}
