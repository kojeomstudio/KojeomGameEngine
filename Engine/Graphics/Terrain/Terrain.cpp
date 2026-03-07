#include "Terrain.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/GraphicsDevice.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>

HRESULT KHeightMap::LoadFromRawFile(const std::wstring& Path, UINT32 InWidth, UINT32 InHeight)
{
    std::ifstream file(Path, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open height map file");
        return E_FAIL;
    }

    Width = InWidth;
    HeightDim = InHeight;
    Data.resize(static_cast<size_t>(Width) * HeightDim);

    for (size_t i = 0; i < Data.size(); ++i)
    {
        unsigned char height;
        file.read(reinterpret_cast<char*>(&height), 1);
        Data[i] = static_cast<float>(height) / 255.0f;
    }

    file.close();
    return S_OK;
}

HRESULT KHeightMap::LoadFromImage(const std::wstring& Path)
{
    LOG_WARNING("Image-based height map loading requires image library. Using flat terrain.");
    return GenerateFlat(128, 128, 0.0f);
}

HRESULT KHeightMap::GeneratePerlinNoise(UINT32 InWidth, UINT32 InHeight, float Scale, int32 Octaves, float Persistence)
{
    Width = InWidth;
    HeightDim = InHeight;
    Data.resize(static_cast<size_t>(Width) * HeightDim);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<std::vector<float>> noiseData(HeightDim, std::vector<float>(Width));

    for (UINT32 y = 0; y < HeightDim; ++y)
    {
        for (UINT32 x = 0; x < Width; ++x)
        {
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseHeight = 0.0f;
            float maxAmplitude = 0.0f;

            for (int32 o = 0; o < Octaves; ++o)
            {
                float sampleX = (static_cast<float>(x) / Scale) * frequency;
                float sampleY = (static_cast<float>(y) / Scale) * frequency;

                float noise = std::sin(sampleX * 12.9898f + sampleY * 78.233f) * 43758.5453f;
                noise = noise - std::floor(noise);

                noiseHeight += noise * amplitude;
                maxAmplitude += amplitude;

                amplitude *= Persistence;
                frequency *= 2.0f;
            }

            noiseData[y][x] = noiseHeight / maxAmplitude;
        }
    }

    for (UINT32 y = 0; y < HeightDim; ++y)
    {
        for (UINT32 x = 0; x < Width; ++x)
        {
            Data[y * Width + x] = noiseData[y][x];
        }
    }

    Smooth(2);
    return S_OK;
}

HRESULT KHeightMap::GenerateFlat(UINT32 InWidth, UINT32 InHeight, float FlatHeight)
{
    Width = InWidth;
    HeightDim = InHeight;
    Data.resize(static_cast<size_t>(Width) * HeightDim, FlatHeight);
    return S_OK;
}

float KHeightMap::GetHeight(UINT32 X, UINT32 Z) const
{
    if (X >= Width || Z >= HeightDim) return 0.0f;
    return Data[Z * Width + X];
}

float KHeightMap::GetHeightInterpolated(float X, float Z) const
{
    if (Width == 0 || HeightDim == 0) return 0.0f;

    float fx = std::max(0.0f, std::min(X, static_cast<float>(Width - 1)));
    float fz = std::max(0.0f, std::min(Z, static_cast<float>(HeightDim - 1)));

    UINT32 x0 = static_cast<UINT32>(fx);
    UINT32 z0 = static_cast<UINT32>(fz);
    UINT32 x1 = std::min(x0 + 1, Width - 1);
    UINT32 z1 = std::min(z0 + 1, HeightDim - 1);

    float tx = fx - static_cast<float>(x0);
    float tz = fz - static_cast<float>(z0);

    float h00 = GetHeight(x0, z0);
    float h10 = GetHeight(x1, z0);
    float h01 = GetHeight(x0, z1);
    float h11 = GetHeight(x1, z1);

    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;

    return h0 * (1.0f - tz) + h1 * tz;
}

void KHeightMap::SetHeight(UINT32 X, UINT32 Z, float HeightValue)
{
    if (X < Width && Z < HeightDim)
    {
        Data[Z * Width + X] = HeightValue;
    }
}

XMFLOAT3 KHeightMap::GetNormal(UINT32 X, UINT32 Z) const
{
    float hL = GetHeight(X > 0 ? X - 1 : X, Z);
    float hR = GetHeight(X < Width - 1 ? X + 1 : X, Z);
    float hD = GetHeight(X, Z > 0 ? Z - 1 : Z);
    float hU = GetHeight(X, Z < HeightDim - 1 ? Z + 1 : Z);

    XMFLOAT3 normal(hL - hR, 2.0f, hD - hU);
    XMVECTOR v = XMLoadFloat3(&normal);
    v = XMVector3Normalize(v);
    XMStoreFloat3(&normal, v);

    return normal;
}

void KHeightMap::Smooth(int32 Iterations)
{
    if (Data.empty()) return;

    for (int32 iter = 0; iter < Iterations; ++iter)
    {
        std::vector<float> smoothed = Data;

        for (UINT32 z = 1; z < HeightDim - 1; ++z)
        {
            for (UINT32 x = 1; x < Width - 1; ++x)
            {
                float sum = 0.0f;
                int32 count = 0;

                for (int32 dz = -1; dz <= 1; ++dz)
                {
                    for (int32 dx = -1; dx <= 1; ++dx)
                    {
                        sum += GetHeight(x + dx, z + dz);
                        ++count;
                    }
                }

                smoothed[z * Width + x] = sum / static_cast<float>(count);
            }
        }

        Data = smoothed;
    }
}

void KHeightMap::Normalize()
{
    if (Data.empty()) return;

    float minVal = Data[0];
    float maxVal = Data[0];

    for (const float& val : Data)
    {
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }

    float range = maxVal - minVal;
    if (range < 0.0001f) range = 1.0f;

    for (float& val : Data)
    {
        val = (val - minVal) / range;
    }
}

HRESULT KTerrain::Initialize(KGraphicsDevice* Device, const FTerrainConfig& InConfig)
{
    GraphicsDevice = Device;
    Config = InConfig;

    auto heightMap = std::make_shared<KHeightMap>();
    HRESULT hr = heightMap->GeneratePerlinNoise(Config.Resolution, Config.Resolution, 32.0f, 6, 0.5f);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to generate height map for terrain");
        return hr;
    }

    return InitializeWithHeightMap(Device, InConfig, heightMap);
}

HRESULT KTerrain::InitializeWithHeightMap(KGraphicsDevice* Device, const FTerrainConfig& InConfig,
                                           std::shared_ptr<KHeightMap> InHeightMap)
{
    if (!Device || !InHeightMap || !InHeightMap->IsValid())
    {
        LOG_ERROR("Invalid parameters for terrain initialization");
        return E_INVALIDARG;
    }

    GraphicsDevice = Device;
    Config = InConfig;
    HeightMap = InHeightMap;

    HRESULT hr = GenerateTerrainMesh();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to generate terrain mesh");
        return hr;
    }

    hr = GenerateLODMeshes();
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to generate LOD meshes, using base mesh only");
    }

    UpdateWorldMatrix();
    bInitialized = true;

    LOG_INFO("Terrain initialized successfully");
    return S_OK;
}

void KTerrain::Cleanup()
{
    BaseMesh.reset();
    LODMeshes.clear();
    HeightMap.reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    BaseTexture.reset();
    SplatMap.reset();
    SplatLayers.clear();
    TerrainShader.reset();
    bInitialized = false;
}

HRESULT KTerrain::GenerateTerrainMesh()
{
    if (!HeightMap || !HeightMap->IsValid()) return E_FAIL;

    UINT32 resolution = Config.Resolution;
    UINT32 vertexCount = resolution * resolution;
    UINT32 indexCount = (resolution - 1) * (resolution - 1) * 6;

    std::vector<FTerrainVertex> vertices(vertexCount);
    std::vector<UINT32> indices(indexCount);

    float halfScale = Config.Scale * (resolution - 1) * 0.5f;

    for (UINT32 z = 0; z < resolution; ++z)
    {
        for (UINT32 x = 0; x < resolution; ++x)
        {
            FTerrainVertex& vertex = vertices[z * resolution + x];
            vertex = CreateVertex(x, z);
        }
    }

    UINT32 index = 0;
    for (UINT32 z = 0; z < resolution - 1; ++z)
    {
        for (UINT32 x = 0; x < resolution - 1; ++x)
        {
            UINT32 topLeft = z * resolution + x;
            UINT32 topRight = topLeft + 1;
            UINT32 bottomLeft = (z + 1) * resolution + x;
            UINT32 bottomRight = bottomLeft + 1;

            indices[index++] = topLeft;
            indices[index++] = bottomLeft;
            indices[index++] = topRight;

            indices[index++] = topRight;
            indices[index++] = bottomLeft;
            indices[index++] = bottomRight;
        }
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.ByteWidth = sizeof(FTerrainVertex) * vertexCount;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    HRESULT hr = GraphicsDevice->GetDevice()->CreateBuffer(&vbDesc, &vbData, &VertexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create terrain vertex buffer");
        return hr;
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.ByteWidth = sizeof(UINT32) * indexCount;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = GraphicsDevice->GetDevice()->CreateBuffer(&ibDesc, &ibData, &IndexBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create terrain index buffer");
        return hr;
    }

    VertexCount = vertexCount;
    IndexCount = indexCount;

    BaseMesh = std::make_shared<KMesh>();
    hr = BaseMesh->InitializeFromBuffers(GraphicsDevice->GetDevice(), VertexBuffer.Get(), IndexBuffer.Get(),
                                          vertexCount, indexCount, sizeof(FTerrainVertex));

    return hr;
}

HRESULT KTerrain::GenerateLODMeshes()
{
    LODMeshes.clear();

    for (UINT32 lod = 0; lod < Config.LODCount; ++lod)
    {
        UINT32 lodResolution = Config.Resolution >> lod;
        if (lodResolution < 4) break;

        FTerrainLOD lodData;
        lodData.Distance = Config.LODDistances[lod];
        lodData.IndexCount = (lodResolution - 1) * (lodResolution - 1) * 6;

        std::vector<FTerrainVertex> vertices(lodResolution * lodResolution);
        std::vector<UINT32> indices(lodData.IndexCount);

        float step = static_cast<float>(Config.Resolution - 1) / static_cast<float>(lodResolution - 1);

        for (UINT32 z = 0; z < lodResolution; ++z)
        {
            for (UINT32 x = 0; x < lodResolution; ++x)
            {
                UINT32 srcX = static_cast<UINT32>(x * step);
                UINT32 srcZ = static_cast<UINT32>(z * step);

                vertices[z * lodResolution + x] = CreateVertex(srcX, srcZ);
            }
        }

        UINT32 index = 0;
        for (UINT32 z = 0; z < lodResolution - 1; ++z)
        {
            for (UINT32 x = 0; x < lodResolution - 1; ++x)
            {
                UINT32 topLeft = z * lodResolution + x;
                UINT32 topRight = topLeft + 1;
                UINT32 bottomLeft = (z + 1) * lodResolution + x;
                UINT32 bottomRight = bottomLeft + 1;

                indices[index++] = topLeft;
                indices[index++] = bottomLeft;
                indices[index++] = topRight;

                indices[index++] = topRight;
                indices[index++] = bottomLeft;
                indices[index++] = bottomRight;
            }
        }

        ComPtr<ID3D11Buffer> vb, ib;

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.ByteWidth = sizeof(FTerrainVertex) * vertices.size();
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices.data();

        HRESULT hr = GraphicsDevice->GetDevice()->CreateBuffer(&vbDesc, &vbData, &vb);
        if (FAILED(hr)) continue;

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.ByteWidth = sizeof(UINT32) * indices.size();
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();

        hr = GraphicsDevice->GetDevice()->CreateBuffer(&ibDesc, &ibData, &ib);
        if (FAILED(hr)) continue;

        lodData.Mesh = std::make_shared<KMesh>();
        lodData.Mesh->InitializeFromBuffers(GraphicsDevice->GetDevice(), vb.Get(), ib.Get(),
                                             static_cast<UINT32>(vertices.size()),
                                             static_cast<UINT32>(indices.size()),
                                             sizeof(FTerrainVertex));

        LODMeshes.push_back(std::move(lodData));
    }

    return S_OK;
}

FTerrainVertex KTerrain::CreateVertex(UINT32 X, UINT32 Z) const
{
    FTerrainVertex vertex;

    float height = HeightMap->GetHeight(X, Z);
    float halfScale = Config.Scale * (Config.Resolution - 1) * 0.5f;

    vertex.Position = XMFLOAT3(
        static_cast<float>(X) * Config.Scale - halfScale,
        height * Config.HeightScale,
        static_cast<float>(Z) * Config.Scale - halfScale
    );

    vertex.TexCoord = XMFLOAT2(
        static_cast<float>(X) / static_cast<float>(Config.Resolution - 1) * Config.TextureScale,
        static_cast<float>(Z) / static_cast<float>(Config.Resolution - 1) * Config.TextureScale
    );

    vertex.Normal = HeightMap->GetNormal(X, Z);
    vertex.Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertex.Bitangent = XMFLOAT3(0.0f, 0.0f, 1.0f);

    return vertex;
}

void KTerrain::CalculateNormals()
{
}

void KTerrain::CalculateTangents()
{
}

void KTerrain::UpdateWorldMatrix()
{
    XMMATRIX translation = XMMatrixTranslation(WorldPosition.x, WorldPosition.y, WorldPosition.z);
    XMMATRIX scaling = XMMatrixScaling(WorldScale.x, WorldScale.y, WorldScale.z);
    WorldMatrix = scaling * translation;
}

void KTerrain::Render(KRenderer* Renderer)
{
    if (!bInitialized || !Renderer) return;

    XMFLOAT3 cameraPos = { 0.0f, 0.0f, 0.0f };
    RenderLOD(Renderer, cameraPos);
}

void KTerrain::RenderLOD(KRenderer* Renderer, const XMFLOAT3& CameraPosition)
{
    if (!bInitialized || !Renderer) return;

    std::shared_ptr<KMesh> meshToRender = BaseMesh;

    if (bLODEnabled && !LODMeshes.empty())
    {
        float distX = CameraPosition.x - WorldPosition.x;
        float distZ = CameraPosition.z - WorldPosition.z;
        float distance = std::sqrt(distX * distX + distZ * distZ);

        for (size_t i = LODMeshes.size(); i > 0; --i)
        {
            if (distance >= LODMeshes[i - 1].Distance && LODMeshes[i - 1].Mesh)
            {
                meshToRender = LODMeshes[i - 1].Mesh;
                break;
            }
        }

        if (distance < LODMeshes[0].Distance || !meshToRender)
        {
            meshToRender = BaseMesh;
        }
    }

    if (meshToRender)
    {
        Renderer->RenderMeshLit(meshToRender, WorldMatrix, BaseTexture);
    }
}

float KTerrain::GetHeightAtWorldPosition(float WorldX, float WorldZ) const
{
    if (!HeightMap) return 0.0f;

    float localX = (WorldX - WorldPosition.x + Config.Scale * (Config.Resolution - 1) * 0.5f) / Config.Scale;
    float localZ = (WorldZ - WorldPosition.z + Config.Scale * (Config.Resolution - 1) * 0.5f) / Config.Scale;

    float height = HeightMap->GetHeightInterpolated(localX, localZ);
    return height * Config.HeightScale * WorldScale.y + WorldPosition.y;
}

XMFLOAT3 KTerrain::GetNormalAtWorldPosition(float WorldX, float WorldZ) const
{
    if (!HeightMap) return XMFLOAT3(0.0f, 1.0f, 0.0f);

    float localX = (WorldX - WorldPosition.x + Config.Scale * (Config.Resolution - 1) * 0.5f) / Config.Scale;
    float localZ = (WorldZ - WorldPosition.z + Config.Scale * (Config.Resolution - 1) * 0.5f) / Config.Scale;

    UINT32 ix = static_cast<UINT32>(std::max(0.0f, std::min(localX, static_cast<float>(Config.Resolution - 1))));
    UINT32 iz = static_cast<UINT32>(std::max(0.0f, std::min(localZ, static_cast<float>(Config.Resolution - 1))));

    return HeightMap->GetNormal(ix, iz);
}

void KTerrain::AddSplatLayer(const FSplatLayer& Layer)
{
    SplatLayers.push_back(Layer);
}

void KTerrain::RemoveSplatLayer(UINT32 Index)
{
    if (Index < SplatLayers.size())
    {
        SplatLayers.erase(SplatLayers.begin() + Index);
    }
}

void KTerrain::ClearSplatLayers()
{
    SplatLayers.clear();
}

HRESULT KTerrain::SetHeightMap(std::shared_ptr<KHeightMap> InHeightMap)
{
    if (!InHeightMap || !InHeightMap->IsValid())
    {
        return E_INVALIDARG;
    }

    HeightMap = InHeightMap;

    VertexBuffer.Reset();
    IndexBuffer.Reset();
    BaseMesh.reset();
    LODMeshes.clear();

    HRESULT hr = GenerateTerrainMesh();
    if (FAILED(hr)) return hr;

    return GenerateLODMeshes();
}

FBoundingBox KTerrain::GetBoundingBox() const
{
    FBoundingBox box;

    float halfSize = Config.Scale * (Config.Resolution - 1) * 0.5f;
    box.Min = XMFLOAT3(
        WorldPosition.x - halfSize * WorldScale.x,
        WorldPosition.y,
        WorldPosition.z - halfSize * WorldScale.z
    );
    box.Max = XMFLOAT3(
        WorldPosition.x + halfSize * WorldScale.x,
        WorldPosition.y + Config.HeightScale * WorldScale.y,
        WorldPosition.z + halfSize * WorldScale.z
    );

    return box;
}

HRESULT KTerrainComponent::Initialize(KGraphicsDevice* Device, const FTerrainConfig& InConfig)
{
    Config = InConfig;
    Terrain = std::make_unique<KTerrain>();
    return Terrain->Initialize(Device, Config);
}

HRESULT KTerrainComponent::LoadHeightMap(const std::wstring& Path)
{
    HeightMapPath = Path;

    if (!Terrain) return E_FAIL;

    auto heightMap = std::make_shared<KHeightMap>();
    HRESULT hr = heightMap->LoadFromRawFile(Path, Config.Resolution, Config.Resolution);
    if (FAILED(hr)) return hr;

    return Terrain->SetHeightMap(heightMap);
}

void KTerrainComponent::Tick(float DeltaTime)
{
}

void KTerrainComponent::Render(KRenderer* Renderer)
{
    if (Terrain && GetOwner())
    {
        const FTransform& transform = GetOwner()->GetWorldTransform();
        Terrain->SetWorldPosition(transform.Position);
        Terrain->SetWorldScale(transform.Scale);
        Terrain->Render(Renderer);
    }
}

void KTerrainComponent::Serialize(KBinaryArchive& Archive)
{
    Archive << Config.Resolution;
    Archive << Config.Scale;
    Archive << Config.HeightScale;
    Archive << Config.TextureScale;
    Archive << Config.LODCount;

    Archive << HeightMapPath;
}

void KTerrainComponent::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Config.Resolution;
    Archive >> Config.Scale;
    Archive >> Config.HeightScale;
    Archive >> Config.TextureScale;
    Archive >> Config.LODCount;

    Archive >> HeightMapPath;
}
