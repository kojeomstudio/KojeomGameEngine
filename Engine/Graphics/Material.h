#pragma once

#include "../Utils/Common.h"
#include "../Utils/Math.h"
#include <memory>

class KTexture;
class KGraphicsDevice;

struct FPBRMaterialParams
{
    XMFLOAT4 AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float AO = 1.0f;
    float EmissiveIntensity = 0.0f;
    XMFLOAT4 EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    float NormalIntensity = 1.0f;
    float HeightScale = 0.0f;
    float Padding0 = 0.0f;
    float Padding1 = 0.0f;

    static FPBRMaterialParams Default()
    {
        return FPBRMaterialParams();
    }

    static FPBRMaterialParams Metal()
    {
        FPBRMaterialParams params;
        params.Metallic = 1.0f;
        params.Roughness = 0.3f;
        return params;
    }

    static FPBRMaterialParams Plastic()
    {
        FPBRMaterialParams params;
        params.Metallic = 0.0f;
        params.Roughness = 0.4f;
        return params;
    }

    static FPBRMaterialParams Rubber()
    {
        FPBRMaterialParams params;
        params.Metallic = 0.0f;
        params.Roughness = 0.9f;
        return params;
    }

    static FPBRMaterialParams Gold()
    {
        FPBRMaterialParams params;
        params.AlbedoColor = XMFLOAT4(1.0f, 0.765557f, 0.336057f, 1.0f);
        params.Metallic = 1.0f;
        params.Roughness = 0.3f;
        return params;
    }

    static FPBRMaterialParams Silver()
    {
        FPBRMaterialParams params;
        params.AlbedoColor = XMFLOAT4(0.971519f, 0.959915f, 0.915324f, 1.0f);
        params.Metallic = 1.0f;
        params.Roughness = 0.3f;
        return params;
    }

    static FPBRMaterialParams Copper()
    {
        FPBRMaterialParams params;
        params.AlbedoColor = XMFLOAT4(0.955008f, 0.637427f, 0.538163f, 1.0f);
        params.Metallic = 1.0f;
        params.Roughness = 0.35f;
        return params;
    }
};

static_assert(sizeof(FPBRMaterialParams) % 16 == 0, "FPBRMaterialParams must be 16-byte aligned");

enum class EMaterialTextureSlot : uint32
{
    Albedo = 0,
    Normal = 1,
    Metallic = 2,
    Roughness = 3,
    AO = 4,
    Emissive = 5,
    Height = 6,
    Count = 7
};

class KMaterial
{
public:
    KMaterial() = default;
    ~KMaterial() = default;

    KMaterial(const KMaterial&) = delete;
    KMaterial& operator=(const KMaterial&) = delete;

    HRESULT Initialize(KGraphicsDevice* InDevice);
    void Cleanup();

    void SetAlbedoColor(const XMFLOAT4& Color) { Params.AlbedoColor = Color; bParamsDirty = true; }
    void SetMetallic(float Value) { Params.Metallic = Value; bParamsDirty = true; }
    void SetRoughness(float Value) { Params.Roughness = Value; bParamsDirty = true; }
    void SetAO(float Value) { Params.AO = Value; bParamsDirty = true; }
    void SetEmissive(const XMFLOAT4& Color, float Intensity);
    void SetNormalIntensity(float Value) { Params.NormalIntensity = Value; bParamsDirty = true; }
    void SetHeightScale(float Value) { Params.HeightScale = Value; bParamsDirty = true; }

    void SetParams(const FPBRMaterialParams& InParams) { Params = InParams; bParamsDirty = true; }
    const FPBRMaterialParams& GetParams() const { return Params; }

    void SetTexture(EMaterialTextureSlot Slot, std::shared_ptr<KTexture> Texture);
    std::shared_ptr<KTexture> GetTexture(EMaterialTextureSlot Slot) const;
    bool HasTexture(EMaterialTextureSlot Slot) const;

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void UpdateConstantBuffer(ID3D11DeviceContext* Context);
    ID3D11Buffer* GetConstantBuffer() const { return ConstantBuffer.Get(); }
    ID3D11Buffer* const* GetConstantBufferAddress() const { return ConstantBuffer.GetAddressOf(); }

    void BindTextures(ID3D11DeviceContext* Context);
    void UnbindTextures(ID3D11DeviceContext* Context);

    bool IsValid() const { return bInitialized; }

    uint32 GetTextureMask() const { return TextureMask; }

private:
    HRESULT CreateConstantBuffer();

private:
    std::string Name = "DefaultMaterial";
    KGraphicsDevice* GraphicsDevice = nullptr;
    FPBRMaterialParams Params;
    ComPtr<ID3D11Buffer> ConstantBuffer;
    std::shared_ptr<KTexture> Textures[static_cast<uint32>(EMaterialTextureSlot::Count)];
    uint32 TextureMask = 0;
    bool bInitialized = false;
    bool bParamsDirty = true;
};
