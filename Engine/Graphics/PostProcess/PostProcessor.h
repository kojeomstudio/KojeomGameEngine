#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include "AutoExposure.h"
#include <memory>
#include <vector>

struct FPostProcessParams
{
    float Exposure = 1.0f;
    float Gamma = 2.2f;
    float BloomThreshold = 1.0f;
    float BloomIntensity = 0.5f;
    float BloomBlurSigma = 2.0f;
    UINT32 BloomBlurIterations = 5;
    bool bBloomEnabled = true;
    bool bTonemappingEnabled = true;
    bool bFXAAEnabled = true;
    float FXAASpanMax = 8.0f;
    float FXAAReduceMin = 1.0f / 128.0f;
    float FXAAReduceMul = 1.0f / 8.0f;
    XMFLOAT2 Resolution;
    bool bColorGradingEnabled = false;
    float ColorGradingIntensity = 1.0f;
    XMFLOAT3 ColorFilter;
    float Padding[2];
    float Saturation = 1.0f;
    float Contrast = 1.0f;
};

class KPostProcessor
{
public:
    KPostProcessor() = default;
    ~KPostProcessor() = default;

    KPostProcessor(const KPostProcessor&) = delete;
    KPostProcessor& operator=(const KPostProcessor&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    ID3D11RenderTargetView* GetHDRRenderTargetView() const { return HDRRenderTargetView.Get(); }
    ID3D11ShaderResourceView* GetHDRShaderResourceView() const { return HDRShaderResourceView.Get(); }
    ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView.Get(); }

    void BeginHDRPass(ID3D11DeviceContext* Context);
    void ApplyPostProcessing(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget);
    void ApplyPostProcessing(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget, float DeltaTime);

    void SetParameters(const FPostProcessParams& Params) { Parameters = Params; }
    const FPostProcessParams& GetParameters() const { return Parameters; }

    void SetBloomEnabled(bool bEnabled) { Parameters.bBloomEnabled = bEnabled; }
    void SetTonemappingEnabled(bool bEnabled) { Parameters.bTonemappingEnabled = bEnabled; }
    void SetFXAAEnabled(bool bEnabled) { Parameters.bFXAAEnabled = bEnabled; }
    void SetColorGradingEnabled(bool bEnabled) { Parameters.bColorGradingEnabled = bEnabled; }
    void SetColorGradingIntensity(float Intensity) { Parameters.ColorGradingIntensity = Intensity; }
    void SetColorFilter(const XMFLOAT3& Filter) { Parameters.ColorFilter = Filter; }
    void SetSaturation(float Sat) { Parameters.Saturation = Sat; }
    void SetContrast(float Cont) { Parameters.Contrast = Cont; }

    KAutoExposure* GetAutoExposure() const { return AutoExposure.get(); }
    void SetAutoExposureEnabled(bool bEnabled);
    bool IsAutoExposureEnabled() const;

    HRESULT LoadColorGradingLUT(ID3D11Device* Device, const std::wstring& Path);
    HRESULT CreateDefaultColorGradingLUT(ID3D11Device* Device);

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateHDRRenderTarget(ID3D11Device* Device);
    HRESULT CreateIntermediateRenderTargets(ID3D11Device* Device);
    HRESULT CreateBloomRenderTargets(ID3D11Device* Device);
    HRESULT CreateColorGradingResources(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void ExtractBrightAreas(ID3D11DeviceContext* Context);
    void BlurBloomTexture(ID3D11DeviceContext* Context);
    void ApplyBloom(ID3D11DeviceContext* Context);
    void ApplyColorGrading(ID3D11DeviceContext* Context);
    void ApplyTonemapping(ID3D11DeviceContext* Context);
    void ApplyFXAA(ID3D11DeviceContext* Context, ID3D11RenderTargetView* FinalTarget);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context);
    void UpdatePostProcessBuffer(ID3D11DeviceContext* Context);

    void SetRenderTarget(ID3D11DeviceContext* Context, ID3D11RenderTargetView* RTV, ID3D11DepthStencilView* DSV = nullptr);
    void ClearRenderTarget(ID3D11DeviceContext* Context, ID3D11RenderTargetView* RTV, const float ClearColor[4]);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> HDRTexture;
    ComPtr<ID3D11RenderTargetView> HDRRenderTargetView;
    ComPtr<ID3D11ShaderResourceView> HDRShaderResourceView;
    ComPtr<ID3D11DepthStencilView> DepthStencilView;

    ComPtr<ID3D11Texture2D> BloomExtractTexture;
    ComPtr<ID3D11RenderTargetView> BloomExtractRTV;
    ComPtr<ID3D11ShaderResourceView> BloomExtractSRV;

    ComPtr<ID3D11Texture2D> BloomBlurTextures[2];
    ComPtr<ID3D11RenderTargetView> BloomBlurRTVs[2];
    ComPtr<ID3D11ShaderResourceView> BloomBlurSRVs[2];

    ComPtr<ID3D11Texture2D> IntermediateTexture;
    ComPtr<ID3D11RenderTargetView> IntermediateRTV;
    ComPtr<ID3D11ShaderResourceView> IntermediateSRV;

    std::shared_ptr<KShaderProgram> BrightExtractShader;
    std::shared_ptr<KShaderProgram> BlurShader;
    std::shared_ptr<KShaderProgram> BloomCombineShader;
    std::shared_ptr<KShaderProgram> TonemapperShader;
    std::shared_ptr<KShaderProgram> FXAAShader;
    std::shared_ptr<KShaderProgram> ColorGradingShader;

    ComPtr<ID3D11Buffer> PostProcessConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;
    ComPtr<ID3D11SamplerState> ColorGradingSamplerState;

    ComPtr<ID3D11Texture3D> ColorGradingLUT;
    ComPtr<ID3D11ShaderResourceView> ColorGradingLUTSRV;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    std::unique_ptr<KAutoExposure> AutoExposure;

    FPostProcessParams Parameters;
    bool bInitialized = false;
    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};
