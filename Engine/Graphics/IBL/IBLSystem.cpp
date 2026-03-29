#include "IBLSystem.h"
#include "../GraphicsDevice.h"
#include "../Texture.h"
#include "../Shader.h"
#include <cmath>

struct FCubemapTransformBuffer
{
    XMMATRIX ViewProjection;
    float Roughness;
    uint32 MipLevel;
    XMFLOAT2 Padding;
};

static const float CubemapVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

static const XMFLOAT3 CubemapDirections[6] = {
    { 1.0f,  0.0f,  0.0f},
    {-1.0f,  0.0f,  0.0f},
    { 0.0f,  1.0f,  0.0f},
    { 0.0f, -1.0f,  0.0f},
    { 0.0f,  0.0f,  1.0f},
    { 0.0f,  0.0f, -1.0f}
};

static const XMFLOAT3 CubemapUps[6] = {
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f,  1.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f}
};

HRESULT KIBLSystem::Initialize(KGraphicsDevice* InDevice, uint32 InIrradianceSize, uint32 InPrefilterSize)
{
    if (!InDevice)
    {
        LOG_ERROR("Invalid graphics device for IBL system");
        return E_INVALIDARG;
    }

    GraphicsDevice = InDevice;
    IrradianceSize = InIrradianceSize;
    PrefilterSize = InPrefilterSize;

    LOG_INFO("Initializing IBL System...");

    HRESULT hr = CreateConvolutionShaders();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create IBL convolution shaders");
        return hr;
    }

    ID3D11Device* Device = GraphicsDevice->GetDevice();

    D3D11_BUFFER_DESC VBDesc = {};
    VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    VBDesc.ByteWidth = sizeof(CubemapVertices);
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA VBData = {};
    VBData.pSysMem = CubemapVertices;
    
    hr = Device->CreateBuffer(&VBDesc, &VBData, &CubemapVB);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create cubemap vertex buffer");
        return hr;
    }

    D3D11_BUFFER_DESC ConstDesc = {};
    ConstDesc.Usage = D3D11_USAGE_DYNAMIC;
    ConstDesc.ByteWidth = sizeof(FCubemapTransformBuffer);
    ConstDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ConstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = Device->CreateBuffer(&ConstDesc, nullptr, &TransformBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create transform constant buffer for IBL");
        return hr;
    }

    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    hr = Device->CreateSamplerState(&SamplerDesc, &IBL_SAMPLERState);
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create IBL sampler state");
    }

    D3D11_RASTERIZER_DESC RasterDesc = {};
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.FrontCounterClockwise = FALSE;
    RasterDesc.DepthClipEnable = TRUE;
    
    hr = Device->CreateRasterizerState(&RasterDesc, &IBL_RasterizerState);
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create IBL rasterizer state");
    }

    D3D11_DEPTH_STENCIL_DESC DepthDesc = {};
    DepthDesc.DepthEnable = FALSE;
    DepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    
    hr = Device->CreateDepthStencilState(&DepthDesc, &IBL_DepthState);
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create IBL depth stencil state");
    }

    hr = CreateBRDFLUT();
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create BRDF LUT, IBL will be limited");
    }

    bInitialized = true;
    LOG_INFO("IBL System initialized successfully");
    return S_OK;
}

void KIBLSystem::Cleanup()
{
    IBLTextures.IrradianceMap.reset();
    IBLTextures.PrefilteredMap.reset();
    IBLTextures.BRDFLUT.reset();
    EnvironmentHDRTexture.reset();
    
    CubemapVS.Reset();
    IrradiancePS.Reset();
    PrefilterPS.Reset();
    FullscreenVS.Reset();
    BRDFLUTPS.Reset();
    CubemapLayout.Reset();
    CubemapVB.Reset();
    TransformBuffer.Reset();
    IBL_SAMPLERState.Reset();
    IBL_RasterizerState.Reset();
    IBL_DepthState.Reset();
    
    bInitialized = false;
    bHasEnvironmentMap = false;
    GraphicsDevice = nullptr;
}

HRESULT KIBLSystem::LoadEnvironmentMap(const std::wstring& HDRPath)
{
    if (!bInitialized)
    {
        LOG_ERROR("IBL system not initialized");
        return E_FAIL;
    }

    EnvironmentHDRTexture = std::make_shared<KTexture>();
    HRESULT hr = EnvironmentHDRTexture->LoadFromFile(GraphicsDevice->GetDevice(), HDRPath);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to load environment map: " + StringUtils::WideToMultiByte(HDRPath));
        return hr;
    }

    bHasEnvironmentMap = true;
    LOG_INFO("Environment map loaded: " + StringUtils::WideToMultiByte(HDRPath));
    return S_OK;
}

HRESULT KIBLSystem::GenerateIBLTextures()
{
    if (!bInitialized || !bHasEnvironmentMap)
    {
        LOG_ERROR("Cannot generate IBL textures: system not ready");
        return E_FAIL;
    }

    LOG_INFO("Generating IBL textures...");

    HRESULT hr = CreateIrradianceCubemap();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create irradiance cubemap");
        return hr;
    }

    hr = CreatePrefilteredCubemap();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create prefiltered cubemap");
        return hr;
    }

    LOG_INFO("IBL textures generated successfully");
    return S_OK;
}

void KIBLSystem::Bind(ID3D11DeviceContext* Context, uint32 StartSlot)
{
    ID3D11ShaderResourceView* SRVs[3] = {};
    
    if (IBLTextures.IrradianceMap)
        SRVs[0] = IBLTextures.IrradianceMap->GetShaderResourceView();
    if (IBLTextures.PrefilteredMap)
        SRVs[1] = IBLTextures.PrefilteredMap->GetShaderResourceView();
    if (IBLTextures.BRDFLUT)
        SRVs[2] = IBLTextures.BRDFLUT->GetShaderResourceView();

    Context->PSSetShaderResources(StartSlot, 3, SRVs);

    if (IBL_SAMPLERState)
    {
        Context->PSSetSamplers(2, 1, IBL_SAMPLERState.GetAddressOf());
    }
}

void KIBLSystem::Unbind(ID3D11DeviceContext* Context, uint32 StartSlot)
{
    ID3D11ShaderResourceView* nullSRVs[3] = {};
    Context->PSSetShaderResources(StartSlot, 3, nullSRVs);
}

HRESULT KIBLSystem::CreateBRDFLUT()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();
    
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = 512;
    TexDesc.Height = 512;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = 1;
    TexDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    
    ComPtr<ID3D11Texture2D> BRDFTexture;
    HRESULT hr = Device->CreateTexture2D(&TexDesc, nullptr, &BRDFTexture);
    if (FAILED(hr)) return hr;

    ComPtr<ID3D11RenderTargetView> RTV;
    hr = Device->CreateRenderTargetView(BRDFTexture.Get(), nullptr, &RTV);
    if (FAILED(hr)) return hr;

    ComPtr<ID3D11ShaderResourceView> SRV;
    hr = Device->CreateShaderResourceView(BRDFTexture.Get(), nullptr, &SRV);
    if (FAILED(hr)) return hr;

    IBLTextures.BRDFLUT = std::make_shared<KTexture>();
    IBLTextures.BRDFLUT->SetFromExisting(BRDFTexture.Get(), SRV.Get());

    const std::string BRDFShaderSource = R"(
        cbuffer Params : register(b0)
        {
            uint2 Resolution;
            float2 Padding;
        };
        
        struct VSInput
        {
            float2 Pos : POSITION;
            float2 Tex : TEXCOORD0;
        };
        
        struct PSInput
        {
            float4 Pos : SV_POSITION;
            float2 Tex : TEXCOORD0;
        };
        
        static const float PI = 3.14159265359f;
        
        float RadicalInverse_VdC(uint bits)
        {
            bits = (bits << 16u) | (bits >> 16u);
            bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
            bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
            bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
            bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
            return float(bits) * 2.3283064365386963e-10f;
        }
        
        float2 Hammersley(uint i, uint N)
        {
            return float2(float(i) / float(N), RadicalInverse_VdC(i));
        }
        
        float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
        {
            float a = roughness * roughness;
            float phi = 2.0f * PI * Xi.x;
            float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
            
            float3 H;
            H.x = cos(phi) * sinTheta;
            H.y = sin(phi) * sinTheta;
            H.z = cosTheta;
            
            float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
            float3 tangent = normalize(cross(up, N));
            float3 bitangent = cross(N, tangent);
            
            float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
            return normalize(sampleVec);
        }
        
        float2 IntegrateBRDF(float NdotV, float roughness)
        {
            float3 V;
            V.x = sqrt(1.0f - NdotV * NdotV);
            V.y = 0.0f;
            V.z = NdotV;
            
            float A = 0.0f;
            float B = 0.0f;
            
            float3 N = float3(0.0f, 0.0f, 1.0f);
            
            const uint SAMPLE_COUNT = 1024u;
            for (uint i = 0u; i < SAMPLE_COUNT; ++i)
            {
                float2 Xi = Hammersley(i, SAMPLE_COUNT);
                float3 H = ImportanceSampleGGX(Xi, N, roughness);
                float3 L = normalize(2.0f * dot(V, H) * H - V);
                
                float NdotL = max(L.z, 0.0f);
                float NdotH = max(H.z, 0.0f);
                float VdotH = max(dot(V, H), 0.0f);
                
                if (NdotL > 0.0f)
                {
                    float G = NdotV / max(NdotH * NdotH / VdotH, 0.001f);
                    float G_Vis = (G * VdotH) / max(NdotH * NdotV, 0.001f);
                    float Fc = pow(1.0f - VdotH, 5.0f);
                    
                    A += (1.0f - Fc) * G_Vis;
                    B += Fc * G_Vis;
                }
            }
            
            A /= float(SAMPLE_COUNT);
            B /= float(SAMPLE_COUNT);
            
            return float2(A, B);
        }
        
        PSInput VS_Main(VSInput input)
        {
            PSInput output;
            output.Pos = float4(input.Pos, 0.0f, 1.0f);
            output.Tex = input.Tex;
            return output;
        }
        
        float4 PS_Main(PSInput input) : SV_Target
        {
            float2 integratedBRDF = IntegrateBRDF(input.Tex.x, input.Tex.y);
            return float4(integratedBRDF, 0.0f, 1.0f);
        }
    )";

    KShaderProgram BRDFShader;
    hr = BRDFShader.CompileFromSource(Device, BRDFShaderSource, BRDFShaderSource, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile BRDF LUT shader");
        return hr;
    }

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();
    
    D3D11_VIEWPORT Viewport = {};
    Viewport.Width = 512.0f;
    Viewport.Height = 512.0f;
    Viewport.MaxDepth = 1.0f;
    Context->RSSetViewports(1, &Viewport);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    Context->ClearRenderTargetView(RTV.Get(), clearColor);
    Context->OMSetRenderTargets(1, RTV.GetAddressOf(), nullptr);

    BRDFShader.Bind(Context);
    
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Context->Draw(4, 0);
    
    BRDFShader.Unbind(Context);
    
    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);

    return S_OK;
}

HRESULT KIBLSystem::CreateConvolutionShaders()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();

    const std::string CubemapVSSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix ViewProjection;
            float Roughness;
            uint MipLevel;
            float2 Padding;
        };

        struct VSInput
        {
            float3 Position : POSITION;
        };

        struct PSInput
        {
            float4 Position : SV_POSITION;
            float3 LocalPos : TEXCOORD0;
        };

        PSInput VS_Main(VSInput input)
        {
            PSInput output;
            output.Position = mul(float4(input.Position, 1.0f), ViewProjection);
            output.LocalPos = input.Position;
            return output;
        }
    )";

    const std::string IrradiancePSSource = R"(
        static const float PI = 3.14159265359f;

        struct PSInput
        {
            float4 Position : SV_POSITION;
            float3 LocalPos : TEXCOORD0;
        };

        TextureCube EnvironmentMap : register(t0);
        SamplerState IBL_Sampler : register(s0);

        float3 GetIrradiance(float3 normal)
        {
            float3 up = abs(normal.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
            float3 right = normalize(cross(up, normal));
            up = cross(normal, right);

            float3 irradiance = float3(0.0f, 0.0f, 0.0f);
            float sampleCount = 0.0f;

            float phiStep = 2.0f * PI / 64.0f;
            float thetaStep = PI / 64.0f;

            for (float phi = 0.0f; phi < 2.0f * PI; phi += phiStep)
            {
                for (float theta = 0.0f; theta < 0.5f * PI; theta += thetaStep)
                {
                    float3 tangentVector = cos(phi) * right + sin(phi) * up;
                    float3 sampleVector = cos(theta) * normal + sin(theta) * tangentVector;

                    irradiance += EnvironmentMap.Sample(IBL_Sampler, sampleVector).rgb *
                                  cos(theta) * sin(theta);
                    sampleCount++;
                }
            }

            irradiance = PI * irradiance / sampleCount;
            return irradiance;
        }

        float4 PS_Main(PSInput input) : SV_Target
        {
            float3 normal = normalize(input.LocalPos);
            float3 irradiance = GetIrradiance(normal);
            return float4(irradiance, 1.0f);
        }
    )";

    const std::string PrefilterPSSource = R"(
        static const float PI = 3.14159265359f;

        cbuffer TransformBuffer : register(b0)
        {
            matrix ViewProjection;
            float Roughness;
            uint MipLevel;
            float2 Padding;
        };

        struct PSInput
        {
            float4 Position : SV_POSITION;
            float3 LocalPos : TEXCOORD0;
        };

        TextureCube EnvironmentMap : register(t0);
        SamplerState IBL_Sampler : register(s0);

        float RadicalInverse_VdC(uint bits)
        {
            bits = (bits << 16u) | (bits >> 16u);
            bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
            bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
            bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
            bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
            return float(bits) * 2.3283064365386963e-10f;
        }

        float2 Hammersley(uint i, uint N)
        {
            return float2(float(i) / float(N), RadicalInverse_VdC(i));
        }

        float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
        {
            float a = roughness * roughness;
            float phi = 2.0f * PI * Xi.x;
            float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

            float3 H;
            H.x = cos(phi) * sinTheta;
            H.y = sin(phi) * sinTheta;
            H.z = cosTheta;

            float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
            float3 tangent = normalize(cross(up, N));
            float3 bitangent = cross(N, tangent);

            float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
            return normalize(sampleVec);
        }

        float DistributionGGX(float3 N, float3 H, float roughness)
        {
            float a = roughness * roughness;
            float a2 = a * a;
            float NdotH = max(dot(N, H), 0.0f);
            float NdotH2 = NdotH * NdotH;

            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
            denom = PI * denom * denom;

            return nom / max(denom, 0.0001f);
        }

        float4 PS_Main(PSInput input) : SV_Target
        {
            float3 N = normalize(input.LocalPos);
            float3 R = N;
            float3 V = R;

            const uint SAMPLE_COUNT = 512u;
            float totalWeight = 0.0f;
            float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);

            for (uint i = 0u; i < SAMPLE_COUNT; ++i)
            {
                float2 Xi = Hammersley(i, SAMPLE_COUNT);
                float3 H = ImportanceSampleGGX(Xi, N, Roughness);
                float3 L = normalize(2.0f * dot(V, H) * H - V);

                float NdotL = max(dot(N, L), 0.0f);
                if (NdotL > 0.0f)
                {
                    float D = DistributionGGX(N, H, Roughness);
                    float NdotH = max(dot(N, H), 0.0f);
                    float HdotV = max(dot(H, V), 0.0f);
                    float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

                    float resolution = 512.0f;
                    float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
                    float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

                    float mipLevel = Roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);

                    prefilteredColor += EnvironmentMap.SampleLevel(IBL_Sampler, L, mipLevel).rgb * NdotL;
                    totalWeight += NdotL;
                }
            }

            prefilteredColor = prefilteredColor / max(totalWeight, 0.0001f);
            return float4(prefilteredColor, 1.0f);
        }
    )";

    KShaderProgram CubemapShader;
    HRESULT hr = CubemapShader.CompileFromSource(Device, CubemapVSSource, IrradiancePSSource, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile cubemap vertex shader");
        return hr;
    }

    std::shared_ptr<KShader> vs = CubemapShader.GetShader(EShaderType::Vertex);
    if (vs)
    {
        ComPtr<ID3DBlob> vsBlob = vs->GetBlob();
        hr = Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &CubemapVS);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create cubemap vertex shader");
            return hr;
        }
    }

    KShaderProgram IrradianceShader;
    hr = IrradianceShader.CompileFromSource(Device, CubemapVSSource, IrradiancePSSource, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile irradiance pixel shader");
        return hr;
    }

    std::shared_ptr<KShader> irPS = IrradianceShader.GetShader(EShaderType::Pixel);
    if (irPS)
    {
        ComPtr<ID3DBlob> psBlob = irPS->GetBlob();
        hr = Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &IrradiancePS);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create irradiance pixel shader");
            return hr;
        }
    }

    KShaderProgram PrefilterShader;
    hr = PrefilterShader.CompileFromSource(Device, CubemapVSSource, PrefilterPSSource, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile prefilter pixel shader");
        return hr;
    }

    std::shared_ptr<KShader> pfPS = PrefilterShader.GetShader(EShaderType::Pixel);
    if (pfPS)
    {
        ComPtr<ID3DBlob> psBlob = pfPS->GetBlob();
        hr = Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PrefilterPS);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create prefilter pixel shader");
            return hr;
        }
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    std::shared_ptr<KShader> recompileVS = CubemapShader.GetShader(EShaderType::Vertex);
    if (recompileVS && recompileVS->GetBlob())
    {
        hr = Device->CreateInputLayout(layout, 1,
            recompileVS->GetBlob()->GetBufferPointer(),
            recompileVS->GetBlob()->GetBufferSize(), &CubemapLayout);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create cubemap input layout");
            return hr;
        }
    }

    LOG_INFO("IBL convolution shaders created successfully");
    return S_OK;
}

XMMATRIX CaptureProjection()
{
    return XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.1f, 10.0f);
}

XMMATRIX CaptureView(float x, float y, float z, const XMFLOAT3& Up)
{
    return XMMatrixLookAtLH(
        XMVectorSet(x, y, z, 0.0f),
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        XMLoadFloat3(&Up)
    );
}

HRESULT KIBLSystem::CreateIrradianceCubemap()
{
    return ConvoluteToIrradiance();
}

HRESULT KIBLSystem::ConvoluteToIrradiance()
{
    if (!EnvironmentHDRTexture)
    {
        LOG_ERROR("No environment map loaded for irradiance convolution");
        return E_FAIL;
    }

    ID3D11Device* Device = GraphicsDevice->GetDevice();
    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = IrradianceSize;
    TexDesc.Height = IrradianceSize;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = 6;
    TexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    ComPtr<ID3D11Texture2D> IrradianceTexture;
    HRESULT hr = Device->CreateTexture2D(&TexDesc, nullptr, &IrradianceTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create irradiance cubemap texture");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TexDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SRVDesc.TextureCube.MipLevels = 1;
    SRVDesc.TextureCube.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> IrradianceSRV;
    hr = Device->CreateShaderResourceView(IrradianceTexture.Get(), &SRVDesc, &IrradianceSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create irradiance cubemap SRV");
        return hr;
    }

    ComPtr<ID3D11RenderTargetView> RenderTargets[6];
    for (int i = 0; i < 6; ++i)
    {
        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = TexDesc.Format;
        RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = i;
        RTVDesc.Texture2DArray.ArraySize = 1;

        hr = Device->CreateRenderTargetView(IrradianceTexture.Get(), &RTVDesc, &RenderTargets[i]);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create irradiance render target for face");
            return hr;
        }
    }

    XMMATRIX Projection = XMMatrixTranspose(CaptureProjection());

    XMMATRIX Views[6];
    Views[0] = XMMatrixTranspose(CaptureView( 1.0f,  0.0f,  0.0f, CubemapUps[0]));
    Views[1] = XMMatrixTranspose(CaptureView(-1.0f,  0.0f,  0.0f, CubemapUps[1]));
    Views[2] = XMMatrixTranspose(CaptureView( 0.0f,  1.0f,  0.0f, CubemapUps[2]));
    Views[3] = XMMatrixTranspose(CaptureView( 0.0f, -1.0f,  0.0f, CubemapUps[3]));
    Views[4] = XMMatrixTranspose(CaptureView( 0.0f,  0.0f,  1.0f, CubemapUps[4]));
    Views[5] = XMMatrixTranspose(CaptureView( 0.0f,  0.0f, -1.0f, CubemapUps[5]));

    D3D11_VIEWPORT Viewport = {};
    Viewport.Width = static_cast<float>(IrradianceSize);
    Viewport.Height = static_cast<float>(IrradianceSize);
    Viewport.MaxDepth = 1.0f;

    Context->RSSetViewports(1, &Viewport);
    Context->IASetInputLayout(CubemapLayout.Get());
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(float) * 3;
    UINT offset = 0;
    Context->IASetVertexBuffers(0, 1, CubemapVB.GetAddressOf(), &stride, &offset);

    Context->VSSetShader(CubemapVS.Get(), nullptr, 0);
    Context->PSSetShader(IrradiancePS.Get(), nullptr, 0);

    if (IBL_SAMPLERState)
    {
        Context->PSSetSamplers(0, 1, IBL_SAMPLERState.GetAddressOf());
    }

    EnvironmentHDRTexture->Bind(Context, 0);

    if (IBL_RasterizerState) Context->RSSetState(IBL_RasterizerState.Get());
    if (IBL_DepthState) Context->OMSetDepthStencilState(IBL_DepthState.Get(), 0);

    for (int face = 0; face < 6; ++face)
    {
        FCubemapTransformBuffer TransformData;
        TransformData.ViewProjection = Views[face] * Projection;
        TransformData.Roughness = 0.0f;
        TransformData.MipLevel = 0;
        TransformData.Padding = XMFLOAT2(0.0f, 0.0f);

        Context->UpdateSubresource(TransformBuffer.Get(), 0, nullptr, &TransformData, 0, 0);
        Context->VSSetConstantBuffers(0, 1, TransformBuffer.GetAddressOf());
        Context->PSSetConstantBuffers(0, 1, TransformBuffer.GetAddressOf());

        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        Context->ClearRenderTargetView(RenderTargets[face].Get(), clearColor);
        Context->OMSetRenderTargets(1, RenderTargets[face].GetAddressOf(), nullptr);
        Context->Draw(36, 0);
    }

    ID3D11RenderTargetView* nullRTV = nullptr;
    Context->OMSetRenderTargets(1, &nullRTV, nullptr);

    EnvironmentHDRTexture->Unbind(Context, 0);

    IBLTextures.IrradianceMap = std::make_shared<KTexture>();
    IBLTextures.IrradianceMap->SetFromExisting(IrradianceTexture.Get(), IrradianceSRV.Get());

    LOG_INFO("Irradiance cubemap generated successfully");
    return S_OK;
}

HRESULT KIBLSystem::CreatePrefilteredCubemap()
{
    return ConvoluteToPrefiltered();
}

HRESULT KIBLSystem::ConvoluteToPrefiltered()
{
    if (!EnvironmentHDRTexture)
    {
        LOG_ERROR("No environment map loaded for prefiltered convolution");
        return E_FAIL;
    }

    ID3D11Device* Device = GraphicsDevice->GetDevice();
    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    uint32 maxMipLevels = IBLTextures.PrefilteredMipLevels;
    uint32 texSize = PrefilterSize;

    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = texSize;
    TexDesc.Height = texSize;
    TexDesc.MipLevels = maxMipLevels;
    TexDesc.ArraySize = 6;
    TexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    ComPtr<ID3D11Texture2D> PrefilteredTexture;
    HRESULT hr = Device->CreateTexture2D(&TexDesc, nullptr, &PrefilteredTexture);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create prefiltered cubemap texture");
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TexDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SRVDesc.TextureCube.MipLevels = maxMipLevels;
    SRVDesc.TextureCube.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> PrefilteredSRV;
    hr = Device->CreateShaderResourceView(PrefilteredTexture.Get(), &SRVDesc, &PrefilteredSRV);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create prefiltered cubemap SRV");
        return hr;
    }

    XMMATRIX Projection = XMMatrixTranspose(CaptureProjection());

    XMMATRIX Views[6];
    Views[0] = XMMatrixTranspose(CaptureView( 1.0f,  0.0f,  0.0f, CubemapUps[0]));
    Views[1] = XMMatrixTranspose(CaptureView(-1.0f,  0.0f,  0.0f, CubemapUps[1]));
    Views[2] = XMMatrixTranspose(CaptureView( 0.0f,  1.0f,  0.0f, CubemapUps[2]));
    Views[3] = XMMatrixTranspose(CaptureView( 0.0f, -1.0f,  0.0f, CubemapUps[3]));
    Views[4] = XMMatrixTranspose(CaptureView( 0.0f,  0.0f,  1.0f, CubemapUps[4]));
    Views[5] = XMMatrixTranspose(CaptureView( 0.0f,  0.0f, -1.0f, CubemapUps[5]));

    Context->IASetInputLayout(CubemapLayout.Get());
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(float) * 3;
    UINT offset = 0;
    Context->IASetVertexBuffers(0, 1, CubemapVB.GetAddressOf(), &stride, &offset);

    Context->VSSetShader(CubemapVS.Get(), nullptr, 0);
    Context->PSSetShader(PrefilterPS.Get(), nullptr, 0);

    if (IBL_SAMPLERState)
    {
        Context->PSSetSamplers(0, 1, IBL_SAMPLERState.GetAddressOf());
    }

    EnvironmentHDRTexture->Bind(Context, 0);

    if (IBL_RasterizerState) Context->RSSetState(IBL_RasterizerState.Get());
    if (IBL_DepthState) Context->OMSetDepthStencilState(IBL_DepthState.Get(), 0);

    for (uint32 mip = 0; mip < maxMipLevels; ++mip)
    {
        uint32 mipWidth = static_cast<uint32>(texSize * std::pow(0.5f, mip));
        uint32 mipHeight = mipWidth;

        D3D11_VIEWPORT Viewport = {};
        Viewport.Width = static_cast<float>(mipWidth);
        Viewport.Height = static_cast<float>(mipHeight);
        Viewport.MaxDepth = 1.0f;
        Context->RSSetViewports(1, &Viewport);

        float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);

        ComPtr<ID3D11RenderTargetView> RenderTargets[6];
        for (int face = 0; face < 6; ++face)
        {
            D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
            RTVDesc.Format = TexDesc.Format;
            RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            RTVDesc.Texture2DArray.MipSlice = mip;
            RTVDesc.Texture2DArray.FirstArraySlice = face;
            RTVDesc.Texture2DArray.ArraySize = 1;

            hr = Device->CreateRenderTargetView(PrefilteredTexture.Get(), &RTVDesc, &RenderTargets[face]);
            if (FAILED(hr))
            {
                LOG_ERROR("Failed to create prefiltered render target");
                return hr;
            }
        }

        for (int face = 0; face < 6; ++face)
        {
            FCubemapTransformBuffer TransformData;
            TransformData.ViewProjection = Views[face] * Projection;
            TransformData.Roughness = roughness;
            TransformData.MipLevel = mip;
            TransformData.Padding = XMFLOAT2(0.0f, 0.0f);

            Context->UpdateSubresource(TransformBuffer.Get(), 0, nullptr, &TransformData, 0, 0);
            Context->VSSetConstantBuffers(0, 1, TransformBuffer.GetAddressOf());
            Context->PSSetConstantBuffers(0, 1, TransformBuffer.GetAddressOf());

            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            Context->ClearRenderTargetView(RenderTargets[face].Get(), clearColor);
            Context->OMSetRenderTargets(1, RenderTargets[face].GetAddressOf(), nullptr);
            Context->Draw(36, 0);
        }

        ID3D11RenderTargetView* nullRTV = nullptr;
        Context->OMSetRenderTargets(1, &nullRTV, nullptr);
    }

    EnvironmentHDRTexture->Unbind(Context, 0);

    IBLTextures.PrefilteredMap = std::make_shared<KTexture>();
    IBLTextures.PrefilteredMap->SetFromExisting(PrefilteredTexture.Get(), PrefilteredSRV.Get());

    LOG_INFO("Prefiltered cubemap generated successfully");
    return S_OK;
}
