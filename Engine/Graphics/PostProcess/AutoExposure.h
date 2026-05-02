#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include <memory>

struct FAutoExposureParams
{
    float TargetLuminance = 0.18f;
    float MinExposure = 0.1f;
    float MaxExposure = 10.0f;
    float AdaptationSpeedUp = 3.0f;
    float AdaptationSpeedDown = 1.0f;
    float LowPercentile = 0.5f;
    float HighPercentile = 0.95f;
    uint32 bEnabled = 1;
    float Padding[4];
};
static_assert(sizeof(FAutoExposureParams) % 16 == 0, "FAutoExposureParams must be 16-byte aligned");

class KAutoExposure
{
public:
    KAutoExposure() = default;
    ~KAutoExposure() = default;

    KAutoExposure(const KAutoExposure&) = delete;
    KAutoExposure& operator=(const KAutoExposure&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    float ComputeExposure(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* HDRTexture, float DeltaTime);
    
    void SetParameters(const FAutoExposureParams& Params) { Parameters = Params; }
    const FAutoExposureParams& GetParameters() const { return Parameters; }
    
    float GetCurrentExposure() const { return CurrentExposure; }
    float GetAverageLuminance() const { return AverageLuminance; }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    bool IsEnabled() const { return Parameters.bEnabled; }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateLuminanceResources(ID3D11Device* Device);
    HRESULT CreateHistogramResources(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);

    void DownsampleLuminance(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* HDRTexture);
    void ComputeHistogram(ID3D11DeviceContext* Context);
    float ReadAverageLuminance(ID3D11DeviceContext* Context);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    static constexpr UINT32 LuminanceSize = 128;
    static constexpr UINT32 HistogramSize = 256;

    ComPtr<ID3D11Texture2D> LuminanceTexture1;
    ComPtr<ID3D11RenderTargetView> LuminanceRTV1;
    ComPtr<ID3D11ShaderResourceView> LuminanceSRV1;

    ComPtr<ID3D11Texture2D> LuminanceTexture2;
    ComPtr<ID3D11RenderTargetView> LuminanceRTV2;
    ComPtr<ID3D11ShaderResourceView> LuminanceSRV2;

    ComPtr<ID3D11Texture2D> LuminanceTexture4;
    ComPtr<ID3D11RenderTargetView> LuminanceRTV4;
    ComPtr<ID3D11ShaderResourceView> LuminanceSRV4;

    ComPtr<ID3D11Texture2D> HistogramTexture;
    ComPtr<ID3D11UnorderedAccessView> HistogramUAV;
    ComPtr<ID3D11ShaderResourceView> HistogramSRV;

    ComPtr<ID3D11Buffer> HistogramReadbackBuffer;

    ComPtr<ID3D11Texture2D> LuminanceStagingTexture;

    ComPtr<ID3D11Buffer> AutoExposureConstantBuffer;
    ComPtr<ID3D11Buffer> ExposureConstantBuffer;

    std::shared_ptr<KShaderProgram> LuminanceShader;
    std::shared_ptr<KShaderProgram> DownsampleShader;
    std::shared_ptr<KShaderProgram> HistogramShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    FAutoExposureParams Parameters;
    float CurrentExposure = 1.0f;
    float AverageLuminance = 0.18f;
    float PreviousAverageLuminance = 0.18f;
    bool bInitialized = false;
};
