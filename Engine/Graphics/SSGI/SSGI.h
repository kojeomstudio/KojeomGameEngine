#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>
#include <vector>

struct FSSGIParams
{
    float Radius = 0.5f;
    float Intensity = 1.0f;
    float SampleCount = 16.0f;
    float MaxSteps = 32.0f;
    float Thickness = 0.5f;
    float Falloff = 1.0f;
    float TemporalBlend = 0.9f;
    float BlurStrength = 1.0f;
    bool bEnabled = true;
    bool bTemporalEnabled = true;
    bool bBlurEnabled = true;
    float Padding;
};

class KSSGI
{
public:
    KSSGI() = default;
    ~KSSGI() = default;

    KSSGI(const KSSGI&) = delete;
    KSSGI& operator=(const KSSGI&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ComputeSSGI(ID3D11DeviceContext* Context,
                     ID3D11ShaderResourceView* NormalSRV,
                     ID3D11ShaderResourceView* PositionSRV,
                     ID3D11ShaderResourceView* DepthSRV,
                     ID3D11ShaderResourceView* AlbedoSRV,
                     const XMMATRIX& Projection,
                     const XMMATRIX& View);

    void ApplyBlur(ID3D11DeviceContext* Context);

    ID3D11ShaderResourceView* GetSSGISRV() const { return SSGISRV.Get(); }
    ID3D11ShaderResourceView* GetBlurredSRV() const { return BlurredSRV.Get(); }

    void SetParameters(const FSSGIParams& Params) { Parameters = Params; }
    const FSSGIParams& GetParameters() const { return Parameters; }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    void SetIntensity(float Intensity) { Parameters.Intensity = Intensity; }
    void SetRadius(float Radius) { Parameters.Radius = Radius; }
    void SetTemporalBlend(float Blend) { Parameters.TemporalBlend = Blend; }

    bool IsInitialized() const { return bInitialized; }
    bool IsEnabled() const { return Parameters.bEnabled; }

private:
    HRESULT CreateSSGIRenderTargets(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);
    HRESULT CreateNoiseTexture(ID3D11Device* Device);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> SSGITexture;
    ComPtr<ID3D11RenderTargetView> SSGIRTV;
    ComPtr<ID3D11ShaderResourceView> SSGISRV;

    ComPtr<ID3D11Texture2D> HistoryTexture;
    ComPtr<ID3D11RenderTargetView> HistoryRTV;
    ComPtr<ID3D11ShaderResourceView> HistorySRV;

    ComPtr<ID3D11Texture2D> BlurredTexture;
    ComPtr<ID3D11RenderTargetView> BlurredRTV;
    ComPtr<ID3D11ShaderResourceView> BlurredSRV;

    ComPtr<ID3D11Texture2D> BlurTempTexture;
    ComPtr<ID3D11RenderTargetView> BlurTempRTV;
    ComPtr<ID3D11ShaderResourceView> BlurTempSRV;

    ComPtr<ID3D11Texture2D> NoiseTexture;
    ComPtr<ID3D11ShaderResourceView> NoiseSRV;

    ComPtr<ID3D11Buffer> SSGIConstantBuffer;
    ComPtr<ID3D11Buffer> BlurConstantBuffer;

    std::shared_ptr<KShaderProgram> SSGIShader;
    std::shared_ptr<KShaderProgram> BlurShader;
    std::shared_ptr<KShaderProgram> TemporalShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;
    ComPtr<ID3D11SamplerState> ClampSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FSSGIParams Parameters;
    bool bInitialized = false;
    int FrameIndex = 0;

    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};
