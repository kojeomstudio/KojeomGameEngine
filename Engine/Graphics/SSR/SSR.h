#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>
#include <vector>

struct FSSRParams
{
    float MaxDistance = 100.0f;
    float Resolution = 0.5f;
    float Thickness = 0.5f;
    float StepCount = 32.0f;
    float MaxSteps = 64.0f;
    float EdgeFade = 0.1f;
    float FresnelPower = 3.0f;
    float Intensity = 1.0f;
    bool bEnabled = true;
    bool bTemporalEnabled = false;
    float Padding[2];
};

class KSSR
{
public:
    KSSR() = default;
    ~KSSR() = default;

    KSSR(const KSSR&) = delete;
    KSSR& operator=(const KSSR&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ComputeSSR(ID3D11DeviceContext* Context,
                    ID3D11ShaderResourceView* ColorSRV,
                    ID3D11ShaderResourceView* NormalSRV,
                    ID3D11ShaderResourceView* PositionSRV,
                    ID3D11ShaderResourceView* DepthSRV,
                    const XMMATRIX& Projection,
                    const XMMATRIX& View);

    void ApplySSR(ID3D11DeviceContext* Context,
                  ID3D11ShaderResourceView* SourceSRV);

    ID3D11ShaderResourceView* GetSSROutputSRV() const { return SSRSRV.Get(); }
    ID3D11ShaderResourceView* GetCombinedSRV() const { return CombinedSRV.Get(); }

    void SetParameters(const FSSRParams& Params) { Parameters = Params; }
    const FSSRParams& GetParameters() const { return Parameters; }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    void SetIntensity(float Intensity) { Parameters.Intensity = Intensity; }
    void SetMaxDistance(float Distance) { Parameters.MaxDistance = Distance; }
    void SetEdgeFade(float Fade) { Parameters.EdgeFade = Fade; }

    bool IsInitialized() const { return bInitialized; }
    bool IsEnabled() const { return Parameters.bEnabled; }

private:
    HRESULT CreateSSRRenderTargets(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> SSRTexture;
    ComPtr<ID3D11RenderTargetView> SSRRTV;
    ComPtr<ID3D11ShaderResourceView> SSRSRV;

    ComPtr<ID3D11Texture2D> CombinedTexture;
    ComPtr<ID3D11RenderTargetView> CombinedRTV;
    ComPtr<ID3D11ShaderResourceView> CombinedSRV;

    ComPtr<ID3D11Buffer> SSRConstantBuffer;
    ComPtr<ID3D11Buffer> CombineConstantBuffer;

    std::shared_ptr<KShaderProgram> SSRShader;
    std::shared_ptr<KShaderProgram> CombineShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;
    ComPtr<ID3D11SamplerState> ClampSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FSSRParams Parameters;
    bool bInitialized = false;

    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};
