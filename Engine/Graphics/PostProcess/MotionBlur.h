#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>

struct FMotionBlurParams
{
    float Intensity = 1.0f;
    UINT32 MaxSamples = 16;
    float MinVelocity = 1.0f;
    float MaxVelocity = 100.0f;
    bool bEnabled = true;
    XMFLOAT2 Resolution;
    float Padding[2];
};

class KMotionBlur
{
public:
    KMotionBlur() = default;
    ~KMotionBlur() = default;

    KMotionBlur(const KMotionBlur&) = delete;
    KMotionBlur& operator=(const KMotionBlur&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ApplyMotionBlur(ID3D11DeviceContext* Context,
                         ID3D11ShaderResourceView* ColorSRV,
                         ID3D11ShaderResourceView* VelocitySRV,
                         ID3D11RenderTargetView* OutputRTV);

    void SetParameters(const FMotionBlurParams& Params) { Parameters = Params; }
    const FMotionBlurParams& GetParameters() const { return Parameters; }

    void SetIntensity(float Intensity) { Parameters.Intensity = Intensity; }
    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    void SetMaxSamples(UINT32 Samples) { Parameters.MaxSamples = Samples; }

    void SetMatrices(const XMMATRIX& CurrentViewProj, const XMMATRIX& PreviousViewProj);

    ID3D11ShaderResourceView* GetOutputSRV() const { return OutputSRV.Get(); }
    ID3D11RenderTargetView* GetOutputRTV() const { return OutputRTV.Get(); }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateRenderTargets(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void UpdateConstantBuffer(ID3D11DeviceContext* Context);
    void RenderFullscreenQuad(ID3D11DeviceContext* Context);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> OutputTexture;
    ComPtr<ID3D11RenderTargetView> OutputRTV;
    ComPtr<ID3D11ShaderResourceView> OutputSRV;

    std::shared_ptr<KShaderProgram> MotionBlurShader;

    ComPtr<ID3D11Buffer> MotionBlurConstantBuffer;
    ComPtr<ID3D11Buffer> MatrixConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FMotionBlurParams Parameters;
    XMMATRIX CurrentViewProj = XMMatrixIdentity();
    XMMATRIX PreviousViewProj = XMMatrixIdentity();
    bool bInitialized = false;
};
