#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include <memory>

struct FLensEffectsParams
{
    bool bChromaticAberrationEnabled = true;
    float ChromaticAberrationStrength = 0.005f;
    float ChromaticAberrationOffset = 0.0f;
    
    bool bVignetteEnabled = true;
    float VignetteIntensity = 0.5f;
    float VignetteSmoothness = 0.5f;
    float VignetteRoundness = 1.0f;
    
    bool bFilmGrainEnabled = false;
    float FilmGrainIntensity = 0.1f;
    float FilmGrainSpeed = 1.0f;
    float Time = 0.0f;
    
    XMFLOAT2 Resolution;
    float Padding[3];
};

class KLensEffects
{
public:
    KLensEffects() = default;
    ~KLensEffects() = default;

    KLensEffects(const KLensEffects&) = delete;
    KLensEffects& operator=(const KLensEffects&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ApplyLensEffects(ID3D11DeviceContext* Context,
                          ID3D11ShaderResourceView* InputSRV,
                          ID3D11RenderTargetView* OutputRTV,
                          float DeltaTime);

    void SetParameters(const FLensEffectsParams& Params) { Parameters = Params; }
    const FLensEffectsParams& GetParameters() const { return Parameters; }

    void SetChromaticAberrationEnabled(bool bEnabled) { Parameters.bChromaticAberrationEnabled = bEnabled; }
    void SetChromaticAberrationStrength(float Strength) { Parameters.ChromaticAberrationStrength = Strength; }
    
    void SetVignetteEnabled(bool bEnabled) { Parameters.bVignetteEnabled = bEnabled; }
    void SetVignetteIntensity(float Intensity) { Parameters.VignetteIntensity = Intensity; }
    
    void SetFilmGrainEnabled(bool bEnabled) { Parameters.bFilmGrainEnabled = bEnabled; }
    void SetFilmGrainIntensity(float Intensity) { Parameters.FilmGrainIntensity = Intensity; }

    ID3D11ShaderResourceView* GetOutputSRV() const { return OutputSRV.Get(); }
    ID3D11RenderTargetView* GetOutputRTV() const { return OutputRTV.Get(); }

    bool IsInitialized() const { return bInitialized; }

private:
    HRESULT CreateRenderTargets(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void UpdateConstantBuffer(ID3D11DeviceContext* Context, float DeltaTime);
    void RenderFullscreenQuad(ID3D11DeviceContext* Context);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> OutputTexture;
    ComPtr<ID3D11RenderTargetView> OutputRTV;
    ComPtr<ID3D11ShaderResourceView> OutputSRV;

    std::shared_ptr<KShaderProgram> LensEffectsShader;

    ComPtr<ID3D11Buffer> LensEffectsConstantBuffer;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FLensEffectsParams Parameters;
    bool bInitialized = false;
};
