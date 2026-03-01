#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include <memory>

class KTexture;
class KGraphicsDevice;

struct FIBLTextures
{
    std::shared_ptr<KTexture> IrradianceMap;
    std::shared_ptr<KTexture> PrefilteredMap;
    std::shared_ptr<KTexture> BRDFLUT;
    uint32 PrefilteredMipLevels = 5;

    bool IsValid() const
    {
        return IrradianceMap && PrefilteredMap && BRDFLUT;
    }
};

class KIBLSystem
{
public:
    KIBLSystem() = default;
    ~KIBLSystem() = default;

    KIBLSystem(const KIBLSystem&) = delete;
    KIBLSystem& operator=(const KIBLSystem&) = delete;

    HRESULT Initialize(KGraphicsDevice* InDevice, uint32 InIrradianceSize = 32, uint32 InPrefilterSize = 128);
    void Cleanup();

    HRESULT LoadEnvironmentMap(const std::wstring& HDRPath);
    HRESULT GenerateIBLTextures();

    bool IsInitialized() const { return bInitialized; }
    bool HasEnvironmentMap() const { return bHasEnvironmentMap; }

    const FIBLTextures& GetIBLTextures() const { return IBLTextures; }

    void Bind(ID3D11DeviceContext* Context, uint32 StartSlot = 10);
    void Unbind(ID3D11DeviceContext* Context, uint32 StartSlot = 10);

    void SetIBLIntensity(float Intensity) { IBLIntensity = Intensity; }
    float GetIBLIntensity() const { return IBLIntensity; }

    void SetRotation(float Rotation) { EnvironmentRotation = Rotation; }
    float GetRotation() const { return EnvironmentRotation; }

private:
    HRESULT CreateBRDFLUT();
    HRESULT CreateIrradianceCubemap();
    HRESULT CreatePrefilteredCubemap();
    HRESULT CreateConvolutionShaders();

    HRESULT ConvoluteToIrradiance();
    HRESULT ConvoluteToPrefiltered();

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    FIBLTextures IBLTextures;
    
    std::shared_ptr<KTexture> EnvironmentHDRTexture;
    
    ComPtr<ID3D11VertexShader> CubemapVS;
    ComPtr<ID3D11PixelShader> IrradiancePS;
    ComPtr<ID3D11PixelShader> PrefilterPS;
    ComPtr<ID3D11VertexShader> FullscreenVS;
    ComPtr<ID3D11PixelShader> BRDFLUTPS;
    
    ComPtr<ID3D11InputLayout> CubemapLayout;
    ComPtr<ID3D11Buffer> CubemapVB;
    ComPtr<ID3D11Buffer> TransformBuffer;
    ComPtr<ID3D11SamplerState> IBL_SAMPLERState;
    
    ComPtr<ID3D11RasterizerState> IBL_RasterizerState;
    ComPtr<ID3D11DepthStencilState> IBL_DepthState;

    uint32 IrradianceSize = 32;
    uint32 PrefilterSize = 128;
    float IBLIntensity = 1.0f;
    float EnvironmentRotation = 0.0f;

    bool bInitialized = false;
    bool bHasEnvironmentMap = false;
};
