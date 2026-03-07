#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../GraphicsDevice.h"
#include "../Camera.h"
#include "../Mesh.h"
#include "../Shader.h"
#include "../Texture.h"

struct FAtmosphereParams
{
    XMFLOAT3 SunDirection = { 0.0f, 1.0f, 0.0f };
    float SunIntensity = 1.0f;
    XMFLOAT3 RayleighCoefficients = { 5.8e-6f, 13.5e-6f, 33.1e-6f };
    float RayleighHeight = 8000.0f;
    XMFLOAT3 MieCoefficients = { 21.0e-6f, 21.0e-6f, 21.0e-6f };
    float MieHeight = 1200.0f;
    float MieAnisotropy = 0.758f;
    float SunAngleScale = 0.9995f;
    float AtmosphereRadius = 6420000.0f;
    float PlanetRadius = 6360000.0f;
    float Exposure = 1.0f;
    float Padding[3];
};

struct FSkyConstantBuffer
{
    XMMATRIX ViewProjection;
    XMFLOAT3 CameraPosition;
    float Padding0;
    FAtmosphereParams Atmosphere;
};

class KSkySystem
{
public:
    KSkySystem() = default;
    ~KSkySystem() = default;

    KSkySystem(const KSkySystem&) = delete;
    KSkySystem& operator=(const KSkySystem&) = delete;

    HRESULT Initialize(KGraphicsDevice* InGraphicsDevice);
    void Cleanup();

    void Render(KCamera* Camera, ID3D11DepthStencilView* DepthStencilView = nullptr);
    
    void SetSunDirection(const XMFLOAT3& Direction);
    void SetSunDirection(float Azimuth, float Elevation);
    void SetSunIntensity(float Intensity) { Params.SunIntensity = Intensity; }
    void SetExposure(float Exposure) { Params.Exposure = Exposure; }
    void SetRayleighCoefficients(const XMFLOAT3& Coefficients) { Params.RayleighCoefficients = Coefficients; }
    void SetMieCoefficients(const XMFLOAT3& Coefficients) { Params.MieCoefficients = Coefficients; }
    void SetMieAnisotropy(float Anisotropy) { Params.MieAnisotropy = Anisotropy; }
    void SetAtmosphereParams(const FAtmosphereParams& InParams) { Params = InParams; }
    void SetTimeOfDay(float Hour, float Latitude = 45.0f);

    const XMFLOAT3& GetSunDirection() const { return Params.SunDirection; }
    float GetSunIntensity() const { return Params.SunIntensity; }
    float GetExposure() const { return Params.Exposure; }
    const FAtmosphereParams& GetAtmosphereParams() const { return Params; }

    bool IsInitialized() const { return bInitialized; }

    void SetEnabled(bool bEnable) { bEnabled = bEnable; }
    bool IsEnabled() const { return bEnabled; }

    void SetCloudEnabled(bool bEnable) { bCloudEnabled = bEnable; }
    bool IsCloudEnabled() const { return bCloudEnabled; }
    void SetCloudDensity(float Density) { CloudDensity = Density; }
    void SetCloudCoverage(float Coverage) { CloudCoverage = Coverage; }

    XMFLOAT3 ComputeSkyColor(const XMFLOAT3& ViewDirection) const;

private:
    HRESULT CreateSkyDome();
    HRESULT CreateSkyShader();
    HRESULT CreateConstantBuffer();
    void UpdateConstantBuffer(KCamera* Camera);
    float ComputeSunAltitude(float Hour, float Latitude) const;
    XMFLOAT3 ComputeSunDirectionFromAltitudeAzimuth(float Altitude, float Azimuth) const;

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    
    std::shared_ptr<KMesh> SkyDomeMesh;
    std::shared_ptr<KShaderProgram> SkyShader;
    
    ComPtr<ID3D11Buffer> SkyConstantBuffer;
    ComPtr<ID3D11RasterizerState> SkyRasterizerState;
    ComPtr<ID3D11DepthStencilState> SkyDepthStencilState;
    ComPtr<ID3D11BlendState> SkyBlendState;
    
    FAtmosphereParams Params;
    
    bool bInitialized = false;
    bool bEnabled = true;
    bool bCloudEnabled = false;
    
    float CloudDensity = 0.5f;
    float CloudCoverage = 0.5f;
};
