#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>

struct FTAAParams
{
    float BlendFactor = 0.1f;
    float FeedbackMin = 0.88f;
    float FeedbackMax = 0.97f;
    float MotionBlurScale = 1.0f;
    bool bEnabled = true;
    bool bSharpenEnabled = true;
    float SharpenStrength = 0.5f;
    float Padding;
};

class KTAA
{
public:
    KTAA() = default;
    ~KTAA() = default;

    KTAA(const KTAA&) = delete;
    KTAA& operator=(const KTAA&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ApplyTAA(ID3D11DeviceContext* Context,
                  ID3D11ShaderResourceView* CurrentColorSRV,
                  ID3D11ShaderResourceView* VelocitySRV,
                  ID3D11ShaderResourceView* DepthSRV,
                  const XMMATRIX& CurrentViewProjection,
                  const XMMATRIX& PreviousViewProjection);

    ID3D11ShaderResourceView* GetOutputSRV() const { return HistorySRV[CurrentHistoryIndex].Get(); }
    ID3D11RenderTargetView* GetOutputRTV() const { return HistoryRTV[CurrentHistoryIndex].Get(); }

    void SetParameters(const FTAAParams& Params) { Parameters = Params; }
    const FTAAParams& GetParameters() const { return Parameters; }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    void SetBlendFactor(float Factor) { Parameters.BlendFactor = Factor; }

    bool IsInitialized() const { return bInitialized; }
    bool IsEnabled() const { return Parameters.bEnabled; }

private:
    HRESULT CreateHistoryTextures(ID3D11Device* Device);
    HRESULT CreateOutputTexture(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> HistoryTexture[2];
    ComPtr<ID3D11RenderTargetView> HistoryRTV[2];
    ComPtr<ID3D11ShaderResourceView> HistorySRV[2];
    int32 CurrentHistoryIndex = 0;

    ComPtr<ID3D11Texture2D> OutputTexture;
    ComPtr<ID3D11RenderTargetView> OutputRTV;
    ComPtr<ID3D11ShaderResourceView> OutputSRV;

    ComPtr<ID3D11Buffer> TAAConstantBuffer;

    std::shared_ptr<KShaderProgram> TAAShader;
    std::shared_ptr<KShaderProgram> SharpenShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    XMMATRIX PreviousViewProjection = XMMatrixIdentity();

    FTAAParams Parameters;
    bool bInitialized = false;
    bool bFirstFrame = true;

    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};
