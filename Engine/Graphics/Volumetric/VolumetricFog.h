#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Mesh.h"
#include "../Light.h"
#include <memory>
#include <vector>

struct FVolumetricFogParams
{
    float Density = 0.01f;
    float HeightFalloff = 0.1f;
    float HeightBase = 0.0f;
    float Scattering = 0.5f;
    float Extinction = 0.01f;
    float Anisotropy = 0.2f;
    float StepCount = 64.0f;
    float MaxDistance = 100.0f;
    XMFLOAT3 FogColor = { 0.5f, 0.5f, 0.6f };
    bool bEnabled = true;
    float Padding[3];
};

struct FVolumetricLightParams
{
    XMFLOAT3 LightPosition = { 0.0f, 10.0f, 0.0f };
    float LightIntensity = 1.0f;
    XMFLOAT3 LightColor = { 1.0f, 1.0f, 1.0f };
    float LightRadius = 50.0f;
    bool bEnabled = true;
    float Padding[3];
};

class KVolumetricFog
{
public:
    KVolumetricFog() = default;
    ~KVolumetricFog() = default;

    KVolumetricFog(const KVolumetricFog&) = delete;
    KVolumetricFog& operator=(const KVolumetricFog&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    HRESULT Resize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight);

    void ComputeFog(ID3D11DeviceContext* Context,
                    ID3D11ShaderResourceView* DepthSRV,
                    ID3D11ShaderResourceView* PositionSRV,
                    const XMMATRIX& View,
                    const XMMATRIX& Projection,
                    const XMMATRIX& InverseProjection,
                    const XMFLOAT3& CameraPosition);

    void ApplyFog(ID3D11DeviceContext* Context,
                  ID3D11ShaderResourceView* SceneColorSRV);

    ID3D11ShaderResourceView* GetFogOutputSRV() const { return FogOutputSRV.Get(); }
    ID3D11ShaderResourceView* GetCombinedSRV() const { return CombinedSRV.Get(); }

    void SetFogParams(const FVolumetricFogParams& Params) { FogParams = Params; }
    const FVolumetricFogParams& GetFogParams() const { return FogParams; }

    void SetLightParams(const FVolumetricLightParams& Params) { LightParams = Params; }
    const FVolumetricLightParams& GetLightParams() const { return LightParams; }

    void SetEnabled(bool bEnabled) { FogParams.bEnabled = bEnabled; }
    void SetDensity(float Density) { FogParams.Density = Density; }
    void SetFogColor(const XMFLOAT3& Color) { FogParams.FogColor = Color; }

    bool IsInitialized() const { return bInitialized; }
    bool IsEnabled() const { return FogParams.bEnabled; }

private:
    HRESULT CreateFogRenderTargets(ID3D11Device* Device);
    HRESULT CreateShaders(ID3D11Device* Device);
    HRESULT CreateConstantBuffers(ID3D11Device* Device);
    HRESULT CreateSamplerStates(ID3D11Device* Device);
    HRESULT CreateFullscreenQuad(ID3D11Device* Device);

    void RenderFullscreenQuad(ID3D11DeviceContext* Context, ID3D11InputLayout* Layout);

private:
    ID3D11Device* Device = nullptr;
    UINT32 Width = 0;
    UINT32 Height = 0;

    ComPtr<ID3D11Texture2D> FogTexture;
    ComPtr<ID3D11RenderTargetView> FogRTV;
    ComPtr<ID3D11ShaderResourceView> FogSRV;

    ComPtr<ID3D11Texture2D> FogOutputTexture;
    ComPtr<ID3D11RenderTargetView> FogOutputRTV;
    ComPtr<ID3D11ShaderResourceView> FogOutputSRV;

    ComPtr<ID3D11Texture2D> CombinedTexture;
    ComPtr<ID3D11RenderTargetView> CombinedRTV;
    ComPtr<ID3D11ShaderResourceView> CombinedSRV;

    ComPtr<ID3D11Buffer> FogConstantBuffer;

    std::shared_ptr<KShaderProgram> FogShader;
    std::shared_ptr<KShaderProgram> CombineShader;

    ComPtr<ID3D11SamplerState> PointSamplerState;
    ComPtr<ID3D11SamplerState> LinearSamplerState;

    std::shared_ptr<KMesh> FullscreenQuadMesh;

    FVolumetricFogParams FogParams;
    FVolumetricLightParams LightParams;
    bool bInitialized = false;

    static constexpr float DefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
