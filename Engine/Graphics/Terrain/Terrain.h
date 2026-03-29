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

struct FTerrainConfig
{
    UINT32 Resolution = 128;
    float Scale = 1.0f;
    float HeightScale = 50.0f;
    float TextureScale = 1.0f;
    UINT32 LODCount = 4;
    float LODDistances[4] = { 50.0f, 100.0f, 200.0f, 400.0f };
};

struct FTerrainVertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Bitangent;
};

struct FTerrainLOD
{
    std::shared_ptr<KMesh> Mesh;
    float Distance;
    UINT32 IndexCount = 0;
};

struct FSplatLayer
{
    std::shared_ptr<KTexture> DiffuseTexture;
    std::shared_ptr<KTexture> NormalTexture;
    float TextureScale = 1.0f;
    std::string Name;
};

class KHeightMap
{
public:
    KHeightMap() = default;
    ~KHeightMap() = default;

    HRESULT LoadFromRawFile(const std::wstring& Path, UINT32 Width, UINT32 Height);
    HRESULT LoadFromImage(const std::wstring& Path);
    HRESULT GeneratePerlinNoise(UINT32 Width, UINT32 Height, float Scale, int32 Octaves, float Persistence);
    HRESULT GenerateFlat(UINT32 InWidth, UINT32 InHeight, float FlatHeight = 0.0f);

    float GetHeight(UINT32 X, UINT32 Z) const;
    float GetHeightInterpolated(float X, float Z) const;
    void SetHeight(UINT32 X, UINT32 Z, float HeightValue);

    XMFLOAT3 GetNormal(UINT32 X, UINT32 Z) const;

    UINT32 GetWidth() const { return Width; }
    UINT32 GetHeightDimension() const { return HeightDim; }
    const std::vector<float>& GetData() const { return Data; }
    bool IsValid() const { return !Data.empty(); }

    void Smooth(int32 Iterations = 1);
    void Normalize();

private:
    std::vector<float> Data;
    UINT32 Width = 0;
    UINT32 HeightDim = 0;
};

class KTerrain
{
public:
    KTerrain() = default;
    ~KTerrain() = default;

    KTerrain(const KTerrain&) = delete;
    KTerrain& operator=(const KTerrain&) = delete;

    HRESULT Initialize(class KGraphicsDevice* Device, const FTerrainConfig& Config);
    HRESULT InitializeWithHeightMap(class KGraphicsDevice* Device, const FTerrainConfig& Config, 
                                     std::shared_ptr<KHeightMap> HeightMap);
    void Cleanup();

    void Render(class KRenderer* Renderer);
    void RenderLOD(class KRenderer* Renderer, const XMFLOAT3& CameraPosition);

    HRESULT SetHeightMap(std::shared_ptr<KHeightMap> InHeightMap);
    std::shared_ptr<KHeightMap> GetHeightMap() const { return HeightMap; }

    void SetWorldPosition(const XMFLOAT3& Position) { WorldPosition = Position; UpdateWorldMatrix(); }
    const XMFLOAT3& GetWorldPosition() const { return WorldPosition; }

    void SetWorldScale(const XMFLOAT3& Scale) { WorldScale = Scale; UpdateWorldMatrix(); }
    const XMFLOAT3& GetWorldScale() const { return WorldScale; }

    XMMATRIX GetWorldMatrix() const { return WorldMatrix; }

    float GetHeightAtWorldPosition(float WorldX, float WorldZ) const;
    XMFLOAT3 GetNormalAtWorldPosition(float WorldX, float WorldZ) const;

    void SetBaseTexture(std::shared_ptr<KTexture> Texture) { BaseTexture = Texture; }
    void SetSplatMap(std::shared_ptr<KTexture> Texture) { SplatMap = Texture; }

    void AddSplatLayer(const FSplatLayer& Layer);
    void RemoveSplatLayer(UINT32 Index);
    void ClearSplatLayers();
    const std::vector<FSplatLayer>& GetSplatLayers() const { return SplatLayers; }

    void SetTerrainShader(std::shared_ptr<KShaderProgram> Shader) { TerrainShader = Shader; }
    std::shared_ptr<KShaderProgram> GetTerrainShader() const { return TerrainShader; }

    const FTerrainConfig& GetConfig() const { return Config; }
    bool IsInitialized() const { return bInitialized; }

    void SetLODEnabled(bool bEnabled) { bLODEnabled = bEnabled; }
    bool IsLODEnabled() const { return bLODEnabled; }

    void SetCastShadows(bool bCast) { bCastShadows = bCast; }
    bool CastShadows() const { return bCastShadows; }

    FBoundingBox GetBoundingBox() const;

private:
    HRESULT GenerateTerrainMesh();
    HRESULT GenerateLODMeshes();
    void UpdateWorldMatrix();
    void CalculateNormals();
    void CalculateTangents();

    FTerrainVertex CreateVertex(UINT32 X, UINT32 Z) const;

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    FTerrainConfig Config;
    std::shared_ptr<KHeightMap> HeightMap;

    std::shared_ptr<KMesh> BaseMesh;
    std::vector<FTerrainLOD> LODMeshes;

    std::shared_ptr<KShaderProgram> TerrainShader;
    std::shared_ptr<KTexture> BaseTexture;
    std::shared_ptr<KTexture> SplatMap;
    std::vector<FSplatLayer> SplatLayers;

    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    UINT32 VertexCount = 0;
    UINT32 IndexCount = 0;

    XMFLOAT3 WorldPosition = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 WorldScale = { 1.0f, 1.0f, 1.0f };
    XMMATRIX WorldMatrix = XMMatrixIdentity();

    bool bInitialized = false;
    bool bLODEnabled = true;
    bool bCastShadows = true;
};

class KTerrainComponent : public KActorComponent
{
public:
    KTerrainComponent() = default;
    ~KTerrainComponent() = default;

    HRESULT Initialize(KGraphicsDevice* Device, const FTerrainConfig& Config);
    HRESULT LoadHeightMap(const std::wstring& Path);

    void Tick(float DeltaTime) override;
    void Render(KRenderer* Renderer) override;

    KTerrain* GetTerrain() const { return Terrain.get(); }

    virtual EComponentType GetComponentTypeID() const override { return EComponentType::Terrain; }

    void Serialize(KBinaryArchive& Archive) override;
    void Deserialize(KBinaryArchive& Archive) override;

private:
    std::unique_ptr<KTerrain> Terrain;
    FTerrainConfig Config;
    std::wstring HeightMapPath;
};
