#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>
#include <vector>

struct FSSAOParams
{
    float Radius = 0.5f;
    float Bias = 0.025f;
    float Power = 2.0f;
    UINT32 KernelSize = 16;
    UINT32 NoiseTextureSize = 4;
    bool bEnabled = true;
    bool bBlurEnabled = true;
    UINT32 BlurIterations = 2;
    float Padding[2];
};

class KSSAO
{
public:
    KSSAO() = default;
    ~KSSAO() = default;

    KSSAO(const KSSAO&) = delete;
    KSSAO& operator=(const KSSAO&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ComputeSSAO(ID3D11DeviceContext* Context,
                     ID3D11ShaderResourceView* NormalSRV,
                     ID3D11ShaderResourceView* PositionSRV,
                     ID3D11ShaderResourceView* DepthSRV,
                     const XMMATRIX& Projection,
                     const XMMATRIX& View);

    void ApplyAO(ID3D11DeviceContext* Context);

    ID3D11ShaderResourceView* GetAOOutputSRV() const;
    ID3D11ShaderResourceView* GetBlurredAOSRV() const { return BlurOutputSRV[0].Get(); }

    void SetParameters(const FSSAOParams& Params) { Parameters = Params; }
    const FSSAOParams& GetParameters() const { return Parameters; }

    void SetEnabled(bool bEnabled) { Parameters.bEnabled = bEnabled; }
    void SetRadius(float Radius) { Parameters.Radius = Radius; }
    void SetBias(float Bias) { Parameters.Bias = Bias; }
    void SetPower(float Power) { Parameters.Power = Power; }
    void SetKernelSize(UINT32 Size) { Parameters.KernelSize = Size; }

    bool IsInitialized() const { return bInitialized; }
    bool IsEnabled() const { return Parameters.bEnabled; }

private:
    HRESULT CreateSSAORenderTargets(ID3D11Device* Device);
    HRESULT CreateNoiseTexture(ID3D11Device* Device);
    HRESULT CreateKernelBuffer(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void GenerateSSAOKernel();
    void GenerateNoiseTexture();

    void BlurAO(ID3D11DeviceContext* Context);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> SSAOTexture;
    ComPtr<ID3D11RenderTargetView> SSAORTV;
    ComPtr<ID3D11ShaderResourceView> SSAOSRV;

    ComPtr<ID3D11Texture2D> BlurOutputTexture[2];
    ComPtr<ID3D11RenderTargetView> BlurOutputRTV[2];
    ComPtr<ID3D11ShaderResourceView> BlurOutputSRV[2];

    ComPtr<ID3D11Texture2D> NoiseTexture;
    ComPtr<ID3D11ShaderResourceView> NoiseSRV;

    ComPtr<ID3D11Buffer> KernelBuffer;
    ComPtr<ID3D11Buffer> SSAOConstantBuffer;

    std::shared_ptr<KShaderProgram> SSAOShader;
    std::shared_ptr<KShaderProgram> BlurShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    std::vector<XMFLOAT4> SSAOKernel;
    std::vector<XMFLOAT3> NoiseData;

    FSSAOParams Parameters;
    bool bInitialized = false;

    static constexpr float DefaultClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};
