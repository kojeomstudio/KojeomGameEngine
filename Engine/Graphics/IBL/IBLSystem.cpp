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

HRESULT KIBLSystem::CreateIrradianceCubemap()
{
    return S_OK;
}

HRESULT KIBLSystem::CreatePrefilteredCubemap()
{
    return S_OK;
}

HRESULT KIBLSystem::CreateConvolutionShaders()
{
    return S_OK;
}

HRESULT KIBLSystem::ConvoluteToIrradiance()
{
    return S_OK;
}

HRESULT KIBLSystem::ConvoluteToPrefiltered()
{
    return S_OK;
}
