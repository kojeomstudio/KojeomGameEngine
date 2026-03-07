#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Utils/Math.h"
#include "../Mesh.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../../Scene/Actor.h"
#include "../../Serialization/BinaryArchive.h"
#include <vector>
#include <memory>
#include <string>

struct FWaterConfig
{
    UINT32 Resolution = 256;
    float Width = 100.0f;
    float Depth = 100.0f;
    float TessellationFactor = 1.0f;
};

struct FWaterVertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Bitangent;
};

struct FWaveParameters
{
    float Amplitude = 1.0f;
    float Frequency = 0.5f;
    float Speed = 1.0f;
    float Steepness = 1.0f;
    XMFLOAT2 Direction = { 1.0f, 0.0f };
};

struct FWaterParameters
{
    XMFLOAT4 DeepColor = { 0.0f, 0.1f, 0.3f, 1.0f };
    XMFLOAT4 ShallowColor = { 0.0f, 0.4f, 0.6f, 1.0f };
    XMFLOAT4 FoamColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    float Transparency = 0.8f;
    float RefractionScale = 0.1f;
    float ReflectionScale = 0.5f;
    float FresnelBias = 0.1f;
    float FresnelPower = 2.0f;
    float FoamIntensity = 0.5f;
    float FoamThreshold = 0.8f;
    float WaveTime = 0.0f;
    float WaveSpeed = 1.0f;
    float NormalMapTiling = 4.0f;
    float DepthMaxDistance = 10.0f;
    
    FWaveParameters Waves[4];
    UINT32 WaveCount = 4;
};

struct FWaterBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
    XMMATRIX ReflectionView;
    
    XMFLOAT4 DeepColor;
    XMFLOAT4 ShallowColor;
    XMFLOAT4 FoamColor;
    XMFLOAT4 CameraPosition;
    
    XMFLOAT4 WaveDirections[4];
    XMFLOAT4 WaveParams[4];
    
    float Time;
    float Transparency;
    float RefractionScale;
    float ReflectionScale;
    
    float FresnelBias;
    float FresnelPower;
    float FoamIntensity;
    float FoamThreshold;
    
    float NormalMapTiling;
    float DepthMaxDistance;
    float WaveSpeed;
    float Padding;
    
    UINT32 WaveCount;
    XMFLOAT3 Padding2;
};

class KWater
{
public:
    KWater() = default;
    ~KWater() = default;

    KWater(const KWater&) = delete;
    KWater& operator=(const KWater&) = delete;

    HRESULT Initialize(class KGraphicsDevice* Device, const FWaterConfig& Config);
    void Cleanup();

    void Update(float DeltaTime);
    void Render(class KRenderer* Renderer, ID3D11ShaderResourceView* ReflectionTexture = nullptr,
                ID3D11ShaderResourceView* RefractionTexture = nullptr, 
                ID3D11ShaderResourceView* DepthTexture = nullptr);

    void SetWorldPosition(const XMFLOAT3& Position) { WorldPosition = Position; UpdateWorldMatrix(); }
    const XMFLOAT3& GetWorldPosition() const { return WorldPosition; }

    void SetWorldScale(const XMFLOAT3& Scale) { WorldScale = Scale; UpdateWorldMatrix(); }
    const XMFLOAT3& GetWorldScale() const { return WorldScale; }

    XMMATRIX GetWorldMatrix() const { return WorldMatrix; }

    void SetWaterShader(std::shared_ptr<KShaderProgram> Shader) { WaterShader = Shader; }
    std::shared_ptr<KShaderProgram> GetWaterShader() const { return WaterShader; }

    void SetNormalMap(std::shared_ptr<KTexture> Texture) { NormalMap = Texture; }
    void SetNormalMap2(std::shared_ptr<KTexture> Texture) { NormalMap2 = Texture; }
    void SetDuDvMap(std::shared_ptr<KTexture> Texture) { DuDvMap = Texture; }
    void SetFoamTexture(std::shared_ptr<KTexture> Texture) { FoamTexture = Texture; }

    FWaterParameters& GetParameters() { return Parameters; }
    const FWaterParameters& GetParameters() const { return Parameters; }
    void SetParameters(const FWaterParameters& InParams) { Parameters = InParams; }

    const FWaterConfig& GetConfig() const { return Config; }
    bool IsInitialized() const { return bInitialized; }

    void SetReflectionEnabled(bool bEnabled) { bReflectionEnabled = bEnabled; }
    bool IsReflectionEnabled() const { return bReflectionEnabled; }

    void SetRefractionEnabled(bool bEnabled) { bRefractionEnabled = bEnabled; }
    bool IsRefractionEnabled() const { return bRefractionEnabled; }

    void SetWaveAnimationEnabled(bool bEnabled) { bWaveAnimationEnabled = bEnabled; }
    bool IsWaveAnimationEnabled() const { return bWaveAnimationEnabled; }

    float GetWaterHeight() const { return WorldPosition.y; }
    void SetWaterHeight(float Height) { WorldPosition.y = Height; UpdateWorldMatrix(); }

    FBoundingBox GetBoundingBox() const;

private:
    HRESULT CreateWaterMesh();
    HRESULT CreateConstantBuffer();
    void UpdateWorldMatrix();
    void UpdateConstantBuffer(class KCamera* Camera, const XMMATRIX& ReflectionViewMatrix);
    void InitializeDefaultWaves();

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    FWaterConfig Config;
    FWaterParameters Parameters;

    std::shared_ptr<KMesh> WaterMesh;
    std::shared_ptr<KShaderProgram> WaterShader;

    std::shared_ptr<KTexture> NormalMap;
    std::shared_ptr<KTexture> NormalMap2;
    std::shared_ptr<KTexture> DuDvMap;
    std::shared_ptr<KTexture> FoamTexture;

    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    ComPtr<ID3D11Buffer> WaterConstantBuffer;
    
    ComPtr<ID3D11SamplerState> WaterSamplerState;
    ComPtr<ID3D11SamplerState> ClampedSamplerState;

    UINT32 VertexCount = 0;
    UINT32 IndexCount = 0;

    XMFLOAT3 WorldPosition = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 WorldScale = { 1.0f, 1.0f, 1.0f };
    XMMATRIX WorldMatrix = XMMatrixIdentity();

    bool bInitialized = false;
    bool bReflectionEnabled = true;
    bool bRefractionEnabled = true;
    bool bWaveAnimationEnabled = true;
};

class KWaterComponent : public KActorComponent
{
public:
    KWaterComponent() = default;
    ~KWaterComponent() = default;

    HRESULT Initialize(KGraphicsDevice* Device, const FWaterConfig& Config);

    void Tick(float DeltaTime) override;
    void Render(KRenderer* Renderer) override;

    KWater* GetWater() const { return Water.get(); }

    void SetReflectionTexture(ID3D11ShaderResourceView* Texture) { ReflectionTexture = Texture; }
    void SetRefractionTexture(ID3D11ShaderResourceView* Texture) { RefractionTexture = Texture; }
    void SetDepthTexture(ID3D11ShaderResourceView* Texture) { DepthTexture = Texture; }

    void Serialize(KBinaryArchive& Archive) override;
    void Deserialize(KBinaryArchive& Archive) override;

private:
    std::unique_ptr<KWater> Water;
    FWaterConfig Config;
    
    ID3D11ShaderResourceView* ReflectionTexture = nullptr;
    ID3D11ShaderResourceView* RefractionTexture = nullptr;
    ID3D11ShaderResourceView* DepthTexture = nullptr;
};
