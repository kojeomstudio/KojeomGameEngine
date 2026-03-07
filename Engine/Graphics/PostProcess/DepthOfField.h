#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>

struct FDepthOfFieldParams
{
    float FocusDistance = 10.0f;
    float FocusRange = 5.0f;
    float BlurRadius = 5.0f;
    UINT32 MaxBlurSamples = 8;
    bool bEnabled = true;
    float NearBlurStart = 0.0f;
    float NearBlurEnd = 5.0f;
    float FarBlurStart = 20.0f;
    float FarBlurEnd = 100.0f;
    XMFLOAT2 Resolution;
    float Padding[1];
};

class KDepthOfField
{
public:
    KDepthOfField() = default;
    ~KDepthOfField() = default;

    KDepthOfField(const KDepthOfField&) = delete;
    KDepthOfField& operator=(const KDepthOfField&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ApplyDepthOfField(ID3D11DeviceContext* Context,
                           ID3D11ShaderResourceView* ColorSRV,
                           ID3D11ShaderResourceView* DepthSRV,
                           ID3D11RenderTargetView* OutputRTV);

    void SetParameters(const FDepthOfFieldParams& Params) { Parameters = Params; }
    const FDepthOfFieldParams& GetParameters() const { return Parameters; }

    void SetFocusDistance(float Distance) { Parameters.FocusDistance = Distance; }
    void SetFocusRange(float Range) { Parameters.FocusRange = Range; }
    void SetBlurRadius(float Radius) { Parameters.BlurRadius = Radius; }
    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }

    void SetCameraNearFar(float Near, float Far) { CameraNear = Near; CameraFar = Far; }

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
    float CalculateCoc(float Depth);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> OutputTexture;
    ComPtr<ID3D11RenderTargetView> OutputRTV;
    ComPtr<ID3D11ShaderResourceView> OutputSRV;

    ComPtr<ID3D11Texture2D> CocTexture;
    ComPtr<ID3D11RenderTargetView> CocRTV;
    ComPtr<ID3D11ShaderResourceView> CocSRV;

    ComPtr<ID3D11Texture2D> BlurTexture;
    ComPtr<ID3D11RenderTargetView> BlurRTV;
    ComPtr<ID3D11ShaderResourceView> BlurSRV;

    std::shared_ptr<KShaderProgram> CocShader;
    std::shared_ptr<KShaderProgram> DoFShader;

    ComPtr<ID3D11Buffer> DoFConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FDepthOfFieldParams Parameters;
    float CameraNear = 0.1f;
    float CameraFar = 1000.0f;
    bool bInitialized = false;
};
