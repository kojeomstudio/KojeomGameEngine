#include "Material.h"
#include "GraphicsDevice.h"
#include "Texture.h"

HRESULT KMaterial::Initialize(KGraphicsDevice* InDevice)
{
    if (!InDevice)
    {
        return E_INVALIDARG;
    }

    GraphicsDevice = InDevice;

    HRESULT hr = CreateConstantBuffer();
    if (FAILED(hr))
    {
        return hr;
    }

    bInitialized = true;
    bParamsDirty = true;
    return S_OK;
}

void KMaterial::Cleanup()
{
    ConstantBuffer.Reset();
    for (uint32 i = 0; i < static_cast<uint32>(EMaterialTextureSlot::Count); ++i)
    {
        Textures[i].reset();
    }
    TextureMask = 0;
    bInitialized = false;
}

void KMaterial::SetEmissive(const XMFLOAT4& Color, float Intensity)
{
    Params.EmissiveColor = Color;
    Params.EmissiveIntensity = Intensity;
    bParamsDirty = true;
}

void KMaterial::SetTexture(EMaterialTextureSlot Slot, std::shared_ptr<KTexture> Texture)
{
    uint32 slotIndex = static_cast<uint32>(Slot);
    if (slotIndex < static_cast<uint32>(EMaterialTextureSlot::Count))
    {
        Textures[slotIndex] = Texture;
        if (Texture)
        {
            TextureMask |= (1 << slotIndex);
        }
        else
        {
            TextureMask &= ~(1 << slotIndex);
        }
    }
}

std::shared_ptr<KTexture> KMaterial::GetTexture(EMaterialTextureSlot Slot) const
{
    uint32 slotIndex = static_cast<uint32>(Slot);
    if (slotIndex < static_cast<uint32>(EMaterialTextureSlot::Count))
    {
        return Textures[slotIndex];
    }
    return nullptr;
}

bool KMaterial::HasTexture(EMaterialTextureSlot Slot) const
{
    uint32 slotIndex = static_cast<uint32>(Slot);
    return (TextureMask & (1 << slotIndex)) != 0;
}

void KMaterial::UpdateConstantBuffer(ID3D11DeviceContext* Context)
{
    if (!ConstantBuffer || !bParamsDirty) return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, &Params, sizeof(FPBRMaterialParams));
        Context->Unmap(ConstantBuffer.Get(), 0);
        bParamsDirty = false;
    }
}

void KMaterial::BindTextures(ID3D11DeviceContext* Context)
{
    ID3D11ShaderResourceView* srvs[static_cast<uint32>(EMaterialTextureSlot::Count)] = {};

    for (uint32 i = 0; i < static_cast<uint32>(EMaterialTextureSlot::Count); ++i)
    {
        if (Textures[i])
        {
            srvs[i] = Textures[i]->GetShaderResourceView();
        }
    }

    Context->PSSetShaderResources(0, static_cast<uint32>(EMaterialTextureSlot::Count), srvs);
}

void KMaterial::UnbindTextures(ID3D11DeviceContext* Context)
{
    ID3D11ShaderResourceView* nullSRVs[static_cast<uint32>(EMaterialTextureSlot::Count)] = {};
    Context->PSSetShaderResources(0, static_cast<uint32>(EMaterialTextureSlot::Count), nullSRVs);
}

HRESULT KMaterial::CreateConstantBuffer()
{
    if (!GraphicsDevice) return E_FAIL;

    ID3D11Device* device = GraphicsDevice->GetDevice();
    if (!device) return E_FAIL;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(FPBRMaterialParams);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &Params;

    HRESULT hr = device->CreateBuffer(&desc, &initData, &ConstantBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
