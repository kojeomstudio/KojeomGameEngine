#include "SkySystem.h"
#include <cmath>

HRESULT KSkySystem::Initialize(KGraphicsDevice* InGraphicsDevice)
{
    if (!InGraphicsDevice)
    {
        LOG_ERROR("Invalid graphics device for SkySystem");
        return E_INVALIDARG;
    }

    GraphicsDevice = InGraphicsDevice;

    HRESULT hr = CreateSkyDome();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky dome mesh");
        return hr;
    }

    hr = CreateSkyShader();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky shader");
        return hr;
    }

    hr = CreateConstantBuffer();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create constant buffer");
        return hr;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_FRONT;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;

    hr = GraphicsDevice->GetDevice()->CreateRasterizerState(&rasterizerDesc, &SkyRasterizerState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky rasterizer state");
        return hr;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    hr = GraphicsDevice->GetDevice()->CreateDepthStencilState(&depthStencilDesc, &SkyDepthStencilState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky depth stencil state");
        return hr;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = GraphicsDevice->GetDevice()->CreateBlendState(&blendDesc, &SkyBlendState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky blend state");
        return hr;
    }

    SetSunDirection(0.0f, 45.0f);

    bInitialized = true;
    LOG_INFO("SkySystem initialized successfully");
    return S_OK;
}

void KSkySystem::Cleanup()
{
    SkyDomeMesh.reset();
    SkyShader.reset();
    SkyConstantBuffer.Reset();
    SkyRasterizerState.Reset();
    SkyDepthStencilState.Reset();
    SkyBlendState.Reset();
    bInitialized = false;
}

HRESULT KSkySystem::CreateSkyDome()
{
    const UINT32 segments = 32;
    const UINT32 rings = 16;
    const float radius = 500.0f;

    std::vector<FVertex> vertices;
    std::vector<UINT32> indices;

    for (UINT32 ring = 0; ring <= rings; ++ring)
    {
        float phi = XM_PI * static_cast<float>(ring) / static_cast<float>(rings);
        float y = cosf(phi);
        float ringRadius = sinf(phi);

        for (UINT32 segment = 0; segment <= segments; ++segment)
        {
            float theta = 2.0f * XM_PI * static_cast<float>(segment) / static_cast<float>(segments);
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);

            FVertex vertex;
            vertex.Position = XMFLOAT3(x * radius, y * radius, z * radius);
            vertex.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.Normal = XMFLOAT3(x, y, z);
            vertex.TexCoord = XMFLOAT2(static_cast<float>(segment) / static_cast<float>(segments),
                                        static_cast<float>(ring) / static_cast<float>(rings));

            vertices.push_back(vertex);
        }
    }

    for (UINT32 ring = 0; ring < rings; ++ring)
    {
        for (UINT32 segment = 0; segment < segments; ++segment)
        {
            UINT32 current = ring * (segments + 1) + segment;
            UINT32 next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next + 1);
            indices.push_back(current + 1);

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    SkyDomeMesh = std::make_shared<KMesh>();
    return SkyDomeMesh->Initialize(GraphicsDevice->GetDevice(), 
                                   vertices.data(), static_cast<UINT32>(vertices.size()),
                                   indices.data(), static_cast<UINT32>(indices.size()));
}

HRESULT KSkySystem::CreateSkyShader()
{
    const char* vsSource = R"(
cbuffer SkyBuffer : register(b0)
{
    matrix ViewProjection;
    float3 CameraPosition;
    float Padding0;
    
    float3 SunDirection;
    float SunIntensity;
    
    float3 RayleighCoefficients;
    float RayleighHeight;
    
    float3 MieCoefficients;
    float MieHeight;
    
    float MieAnisotropy;
    float SunAngleScale;
    float AtmosphereRadius;
    float PlanetRadius;
    
    float Exposure;
    float3 Padding1;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 RayDirection : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float3 worldPos = input.Position + CameraPosition;
    output.Position = mul(ViewProjection, float4(worldPos, 1.0));
    output.WorldPos = worldPos;
    output.RayDirection = normalize(input.Position);
    
    return output;
}
)";

    const char* psSource = R"(
static const float PI = 3.14159265359;
static const int NUM_SAMPLES = 16;

cbuffer SkyBuffer : register(b0)
{
    matrix ViewProjection;
    float3 CameraPosition;
    float Padding0;
    
    float3 SunDirection;
    float SunIntensity;
    
    float3 RayleighCoefficients;
    float RayleighHeight;
    
    float3 MieCoefficients;
    float MieHeight;
    
    float MieAnisotropy;
    float SunAngleScale;
    float AtmosphereRadius;
    float PlanetRadius;
    
    float Exposure;
    float3 Padding1;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 RayDirection : TEXCOORD1;
};

float2 RaySphereIntersect(float3 ro, float3 rd, float radius)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float d = b * b - c;
    
    if (d < 0.0)
        return float2(-1.0, -1.0);
    
    d = sqrt(d);
    return float2(-b - d, -b + d);
}

float RayleighPhase(float cosTheta)
{
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

float MiePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float num = (1.0 - g2);
    float denom = 4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return num / denom;
}

float3 Atmosphere(float3 rayOrigin, float3 rayDir, float3 sunDir)
{
    float2 atmosphereIntersect = RaySphereIntersect(rayOrigin, rayDir, AtmosphereRadius);
    if (atmosphereIntersect.x > atmosphereIntersect.y)
        return float3(0.0, 0.0, 0.0);
    
    float2 planetIntersect = RaySphereIntersect(rayOrigin, rayDir, PlanetRadius);
    
    float tMax = atmosphereIntersect.y;
    if (planetIntersect.x > 0.0)
    {
        tMax = planetIntersect.x;
    }
    
    float tMin = max(0.0, atmosphereIntersect.x);
    
    float segmentLength = (tMax - tMin) / float(NUM_SAMPLES);
    
    float3 sumR = float3(0.0, 0.0, 0.0);
    float3 sumM = float3(0.0, 0.0, 0.0);
    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;
    
    float cosTheta = dot(rayDir, sunDir);
    float g = MieAnisotropy;
    
    float phaseR = RayleighPhase(cosTheta);
    float phaseM = MiePhase(cosTheta, g);
    
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float t = tMin + (float(i) + 0.5) * segmentLength;
        float3 pos = rayOrigin + t * rayDir;
        
        float height = length(pos) - PlanetRadius;
        
        float densityR = exp(-height / RayleighHeight) * segmentLength;
        float densityM = exp(-height / MieHeight) * segmentLength;
        
        opticalDepthR += densityR;
        opticalDepthM += densityM;
        
        float2 lightIntersect = RaySphereIntersect(pos, sunDir, AtmosphereRadius);
        float lightSegmentLength = lightIntersect.y / float(NUM_SAMPLES);
        
        float lightOpticalDepthR = 0.0;
        float lightOpticalDepthM = 0.0;
        
        for (int j = 0; j < NUM_SAMPLES; ++j)
        {
            float lightT = float(j) + 0.5;
            float3 lightPos = pos + lightT * lightSegmentLength * sunDir;
            float lightHeight = length(lightPos) - PlanetRadius;
            
            lightOpticalDepthR += exp(-lightHeight / RayleighHeight) * lightSegmentLength;
            lightOpticalDepthM += exp(-lightHeight / MieHeight) * lightSegmentLength;
        }
        
        float3 tau = RayleighCoefficients * (opticalDepthR + lightOpticalDepthR) +
                     MieCoefficients * 1.1 * (opticalDepthM + lightOpticalDepthM);
        float3 attenuation = exp(-tau);
        
        sumR += densityR * attenuation;
        sumM += densityM * attenuation;
    }
    
    float3 result = SunIntensity * (sumR * RayleighCoefficients * phaseR + 
                                     sumM * MieCoefficients * phaseM);
    
    return result;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 rayOrigin = float3(0.0, PlanetRadius + CameraPosition.y, 0.0);
    float3 rayDir = normalize(input.RayDirection);
    
    float3 color = Atmosphere(rayOrigin, rayDir, SunDirection);
    
    float sunCos = dot(rayDir, SunDirection);
    if (sunCos > SunAngleScale)
    {
        float sunIntensity = smoothstep(SunAngleScale, 1.0, sunCos);
        float3 sunColor = float3(1.0, 0.9, 0.7) * sunIntensity * SunIntensity * 5.0;
        color += sunColor;
    }
    
    color = ACESFilm(color * Exposure);
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}
)";

    SkyShader = std::make_shared<KShaderProgram>();
    HRESULT hr = SkyShader->CompileFromSource(GraphicsDevice->GetDevice(), vsSource, psSource, "main", "main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile sky shader from source");
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    return SkyShader->CreateInputLayout(GraphicsDevice->GetDevice(), layout, 4);
}

HRESULT KSkySystem::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSkyConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;

    HRESULT hr = GraphicsDevice->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &SkyConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create sky constant buffer");
        return hr;
    }

    return S_OK;
}

void KSkySystem::Render(KCamera* Camera, ID3D11DepthStencilView* DepthStencilView)
{
    if (!bInitialized || !bEnabled || !Camera)
        return;

    ID3D11DeviceContext* context = GraphicsDevice->GetContext();

    UpdateConstantBuffer(Camera);

    context->RSSetState(SkyRasterizerState.Get());
    context->OMSetDepthStencilState(SkyDepthStencilState.Get(), 0);
    
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(SkyBlendState.Get(), blendFactor, 0xFFFFFFFF);

    context->VSSetConstantBuffers(0, 1, SkyConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, SkyConstantBuffer.GetAddressOf());

    SkyShader->Bind(context);
    SkyDomeMesh->Render(context);

    context->RSSetState(nullptr);
    context->OMSetDepthStencilState(nullptr, 0);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void KSkySystem::UpdateConstantBuffer(KCamera* Camera)
{
    ID3D11DeviceContext* context = GraphicsDevice->GetContext();

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(SkyConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        FSkyConstantBuffer* buffer = static_cast<FSkyConstantBuffer*>(mappedResource.pData);
        XMMATRIX viewProj = Camera->GetViewMatrix() * Camera->GetProjectionMatrix();
        buffer->ViewProjection = XMMatrixTranspose(viewProj);
        buffer->CameraPosition = Camera->GetPosition();
        buffer->Atmosphere = Params;
        context->Unmap(SkyConstantBuffer.Get(), 0);
    }
}

void KSkySystem::SetSunDirection(const XMFLOAT3& Direction)
{
    XMVECTOR dir = XMLoadFloat3(&Direction);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&Params.SunDirection, dir);
}

void KSkySystem::SetSunDirection(float Azimuth, float Elevation)
{
    float elevationRad = XMConvertToRadians(Elevation);
    float azimuthRad = XMConvertToRadians(Azimuth);
    
    float x = cosf(elevationRad) * sinf(azimuthRad);
    float y = sinf(elevationRad);
    float z = cosf(elevationRad) * cosf(azimuthRad);
    
    Params.SunDirection = XMFLOAT3(x, y, z);
}

void KSkySystem::SetTimeOfDay(float Hour, float Latitude)
{
    float altitude = ComputeSunAltitude(Hour, Latitude);
    float azimuth = 180.0f + (Hour - 12.0f) * 15.0f;
    SetSunDirection(azimuth, altitude);
    
    float sunIntensity = 1.0f;
    if (altitude < 0.0f)
    {
        sunIntensity = 0.0f;
    }
    else if (altitude < 10.0f)
    {
        sunIntensity = altitude / 10.0f;
        sunIntensity = sunIntensity * sunIntensity;
    }
    Params.SunIntensity = sunIntensity;
}

float KSkySystem::ComputeSunAltitude(float Hour, float Latitude) const
{
    float latRad = XMConvertToRadians(Latitude);
    float hourAngle = (Hour - 12.0f) * 15.0f;
    float hourRad = XMConvertToRadians(hourAngle);
    
    float declination = 23.45f * sinf(XMConvertToRadians(360.0f / 365.0f * 80.0f));
    float decRad = XMConvertToRadians(declination);
    
    float sinAltitude = sinf(latRad) * sinf(decRad) + 
                        cosf(latRad) * cosf(decRad) * cosf(hourRad);
    
    return XMConvertToDegrees(asinf(sinAltitude));
}

XMFLOAT3 KSkySystem::ComputeSunDirectionFromAltitudeAzimuth(float Altitude, float Azimuth) const
{
    float altRad = XMConvertToRadians(Altitude);
    float azRad = XMConvertToRadians(Azimuth);
    
    float x = cosf(altRad) * sinf(azRad);
    float y = sinf(altRad);
    float z = cosf(altRad) * cosf(azRad);
    
    return XMFLOAT3(x, y, z);
}

XMFLOAT3 KSkySystem::ComputeSkyColor(const XMFLOAT3& ViewDirection) const
{
    XMVECTOR sunDir = XMLoadFloat3(&Params.SunDirection);
    XMVECTOR viewDir = XMLoadFloat3(&ViewDirection);
    float cosTheta = XMVectorGetX(XMVector3Dot(viewDir, sunDir));
    
    float rayleighFactor = 1.0f + cosTheta * cosTheta;
    
    float g = Params.MieAnisotropy;
    float g2 = g * g;
    float mieFactor = (1.0f - g2) / powf(1.0f + g2 - 2.0f * g * cosTheta, 1.5f);
    
    float intensity = rayleighFactor * 0.5f + mieFactor * 0.01f;
    intensity *= Params.SunIntensity * Params.Exposure;
    
    XMFLOAT3 skyColor;
    skyColor.x = 0.3f * intensity * (Params.RayleighCoefficients.x * 1e6f);
    skyColor.y = 0.5f * intensity * (Params.RayleighCoefficients.y * 1e6f);
    skyColor.z = 1.0f * intensity * (Params.RayleighCoefficients.z * 1e6f);
    
    skyColor.x = fminf(skyColor.x, 1.0f);
    skyColor.y = fminf(skyColor.y, 1.0f);
    skyColor.z = fminf(skyColor.z, 1.0f);
    
    return skyColor;
}
