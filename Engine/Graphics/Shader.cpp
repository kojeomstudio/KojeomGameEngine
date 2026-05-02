#include "Shader.h"

// UShader class implementation

HRESULT KShader::LoadFromFile(ID3D11Device* Device, const std::wstring& Filename, 
                            const std::string& EntryPoint, EShaderType InType)
{
    if (PathUtils::ContainsTraversal(Filename) || !PathUtils::IsPathSafe(Filename, L"."))
    {
        LOG_ERROR("Shader path rejected (unsafe path): " + StringUtils::WideToMultiByte(Filename));
        return E_INVALIDARG;
    }

    Type = InType;

    DWORD ShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    ShaderFlags |= D3DCOMPILE_DEBUG;
    ShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> ErrorBlob;
    std::string Profile = GetProfileString(InType);

    // Compile shader from file
    HRESULT hr = D3DCompileFromFile(
        Filename.c_str(),
        nullptr,
        nullptr,
        EntryPoint.c_str(),
        Profile.c_str(),
        ShaderFlags,
        0,
        &Blob,
        &ErrorBlob
    );

    if (FAILED(hr))
    {
        if (ErrorBlob)
        {
            std::string ErrorMsg = "Shader Compilation Error: ";
            ErrorMsg += static_cast<const char*>(ErrorBlob->GetBufferPointer());
            LOG_ERROR(ErrorMsg);
        }
        KLogger::HResultError(hr, "Shader file compilation failed");
        return hr;
    }

    // Create shader object
    hr = CreateShaderFromBlob(Device);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Shader object creation failed");
        return hr;
    }

    LOG_INFO("Shader loaded successfully");
    return S_OK;
}

HRESULT KShader::CompileFromString(ID3D11Device* Device, const std::string& Source,
                                 const std::string& EntryPoint, EShaderType InType)
{
    Type = InType;

    DWORD ShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    ShaderFlags |= D3DCOMPILE_DEBUG;
    ShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> ErrorBlob;
    std::string Profile = GetProfileString(InType);

    // Compile shader from string
    HRESULT hr = D3DCompile(
        Source.c_str(),
        Source.length(),
        nullptr,
        nullptr,
        nullptr,
        EntryPoint.c_str(),
        Profile.c_str(),
        ShaderFlags,
        0,
        &Blob,
        &ErrorBlob
    );

    if (FAILED(hr))
    {
        if (ErrorBlob)
        {
            std::string ErrorMsg = "Shader Compilation Error: ";
            ErrorMsg += static_cast<const char*>(ErrorBlob->GetBufferPointer());
            LOG_ERROR(ErrorMsg);
        }
        KLogger::HResultError(hr, "Shader string compilation failed");
        return hr;
    }

    // Create shader object
    hr = CreateShaderFromBlob(Device);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Shader object creation failed");
        return hr;
    }

    LOG_INFO("Shader string compilation completed");
    return S_OK;
}

void KShader::Bind(ID3D11DeviceContext* Context) const
{
    switch (Type)
    {
    case EShaderType::Vertex:
        Context->VSSetShader(VertexShader.Get(), nullptr, 0);
        break;
    case EShaderType::Pixel:
        Context->PSSetShader(PixelShader.Get(), nullptr, 0);
        break;
    case EShaderType::Geometry:
        Context->GSSetShader(GeometryShader.Get(), nullptr, 0);
        break;
    case EShaderType::Hull:
        Context->HSSetShader(HullShader.Get(), nullptr, 0);
        break;
    case EShaderType::Domain:
        Context->DSSetShader(DomainShader.Get(), nullptr, 0);
        break;
    case EShaderType::Compute:
        Context->CSSetShader(ComputeShader.Get(), nullptr, 0);
        break;
    }
}

void KShader::Unbind(ID3D11DeviceContext* Context) const
{
    switch (Type)
    {
    case EShaderType::Vertex:
        Context->VSSetShader(nullptr, nullptr, 0);
        break;
    case EShaderType::Pixel:
        Context->PSSetShader(nullptr, nullptr, 0);
        break;
    case EShaderType::Geometry:
        Context->GSSetShader(nullptr, nullptr, 0);
        break;
    case EShaderType::Hull:
        Context->HSSetShader(nullptr, nullptr, 0);
        break;
    case EShaderType::Domain:
        Context->DSSetShader(nullptr, nullptr, 0);
        break;
    case EShaderType::Compute:
        Context->CSSetShader(nullptr, nullptr, 0);
        break;
    }
}

std::string KShader::GetProfileString(EShaderType InType) const
{
    switch (InType)
    {
    case EShaderType::Vertex:   return "vs_5_0";
    case EShaderType::Pixel:    return "ps_5_0";
    case EShaderType::Geometry: return "gs_5_0";
    case EShaderType::Hull:     return "hs_5_0";
    case EShaderType::Domain:   return "ds_5_0";
    case EShaderType::Compute:  return "cs_5_0";
    default:                    return "vs_5_0";
    }
}

HRESULT KShader::CreateShaderFromBlob(ID3D11Device* Device)
{
    HRESULT hr = S_OK;

    switch (Type)
    {
    case EShaderType::Vertex:
        hr = Device->CreateVertexShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &VertexShader
        );
        break;

    case EShaderType::Pixel:
        hr = Device->CreatePixelShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &PixelShader
        );
        break;

    case EShaderType::Geometry:
        hr = Device->CreateGeometryShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &GeometryShader
        );
        break;

    case EShaderType::Hull:
        hr = Device->CreateHullShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &HullShader
        );
        break;

    case EShaderType::Domain:
        hr = Device->CreateDomainShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &DomainShader
        );
        break;

    case EShaderType::Compute:
        hr = Device->CreateComputeShader(
            Blob->GetBufferPointer(),
            Blob->GetBufferSize(),
            nullptr,
            &ComputeShader
        );
        break;

    default:
        return E_INVALIDARG;
    }

    return hr;
}

// UShaderProgram class implementation

HRESULT KShaderProgram::CreateBasicColorShader(ID3D11Device* Device)
{
    // Basic color shader source code (improved version of original TutorialShader.fxh)
    const std::string ShaderSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        struct VS_INPUT
        {
            float4 Pos : POSITION;
            float4 Color : COLOR;
        };

        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float4 Color : COLOR;
        };

        // Vertex Shader
        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            output.Pos = mul(input.Pos, World);
            output.Pos = mul(output.Pos, View);
            output.Pos = mul(output.Pos, Projection);
            output.Color = input.Color;
            return output;
        }

        // Pixel Shader
        float4 PS(PS_INPUT input) : SV_Target
        {
            return input.Color;
        }
    )";

    // Create vertex shader
    auto VertexShader = std::make_shared<KShader>();
    HRESULT hr = VertexShader->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    // Create pixel shader
    auto PixelShader = std::make_shared<KShader>();
    hr = PixelShader->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    // Add shaders to program
    AddShader(VertexShader);
    AddShader(PixelShader);

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Basic color shader creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreatePhongShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        #define MAX_POINT_LIGHTS 8
        #define MAX_SPOT_LIGHTS 4

        struct FPointLightData
        {
            float3 Position;
            float  Intensity;
            float4 Color;
            float  Radius;
            float  Falloff;
            float  Padding0;
            float  Padding1;
        };

        struct FSpotLightData
        {
            float3 Position;
            float  Intensity;
            float3 Direction;
            float  InnerCone;
            float4 Color;
            float  OuterCone;
            float  Radius;
            float  Falloff;
            float  Padding0;
        };

        cbuffer LightBuffer : register(b1)
        {
            float3 CameraPosition;
            int    NumPointLights;
            float3 DirLightDirection;
            int    NumSpotLights;
            float4 DirLightColor;
            float4 AmbientColor;
            FPointLightData PointLights[MAX_POINT_LIGHTS];
            FSpotLightData  SpotLights[MAX_SPOT_LIGHTS];
        }

        struct VS_INPUT
        {
            float3 Pos    : POSITION;
            float4 Color  : COLOR;
            float3 Normal : NORMAL;
        };

        struct PS_INPUT
        {
            float4 Pos      : SV_POSITION;
            float4 Color    : COLOR;
            float3 Normal   : NORMAL;
            float3 WorldPos : TEXCOORD0;
        };

        float3 CalculatePointLight(FPointLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        float3 CalculateSpotLight(FSpotLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float3 spotDir = normalize(-light.Direction);
            float cosAngle = dot(lightDir, spotDir);
            float cosInner = cos(light.InnerCone);
            float cosOuter = cos(light.OuterCone);
            
            if (cosAngle < cosOuter)
                return float3(0, 0, 0);
            
            float spotFactor = saturate((cosAngle - cosOuter) / (cosInner - cosOuter));
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity * spotFactor;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            float4 worldPos  = mul(float4(input.Pos, 1.0f), World);
            float4 viewPos   = mul(worldPos, View);
            output.Pos       = mul(viewPos, Projection);
            output.Color     = input.Color;
            output.Normal    = mul(input.Normal, (float3x3)World);
            output.WorldPos  = worldPos.xyz;
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float3 normal   = normalize(input.Normal);
            float3 viewDir  = normalize(CameraPosition - input.WorldPos);
            
            float3 ambient  = AmbientColor.rgb * input.Color.rgb;
            
            float3 lightDir = normalize(-DirLightDirection);
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * DirLightColor.rgb * input.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * DirLightColor.rgb * 0.4f;
            
            float3 result = ambient + diffuse + specular;
            
            [unroll]
            for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
            {
                if (i >= NumPointLights) break;
                result += CalculatePointLight(PointLights[i], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            [unroll]
            for (int j = 0; j < MAX_SPOT_LIGHTS; ++j)
            {
                if (j >= NumSpotLights) break;
                result += CalculateSpotLight(SpotLights[j], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            return saturate(float4(result, input.Color.a));
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Phong shader with multiple lights creation completed");
    return S_OK;
}

void KShaderProgram::AddShader(std::shared_ptr<KShader> InShader)
{
    Shaders.push_back(InShader);
}

HRESULT KShaderProgram::CreateInputLayout(ID3D11Device* Device, 
                                        const D3D11_INPUT_ELEMENT_DESC* InputElements, 
                                        UINT32 NumElements)
{
    // Vertex shader bytecode is required
    auto VertexShader = GetShader(EShaderType::Vertex);
    if (!VertexShader)
    {
        LOG_ERROR("Vertex shader not found for input layout creation");
        return E_FAIL;
    }

    HRESULT hr = Device->CreateInputLayout(
        InputElements,
        NumElements,
        VertexShader->GetBlob()->GetBufferPointer(),
        VertexShader->GetBlob()->GetBufferSize(),
        &InputLayout
    );

    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Input layout creation failed");
        return hr;
    }

    return S_OK;
}

void KShaderProgram::Bind(ID3D11DeviceContext* Context) const
{
    // Set input layout
    if (InputLayout)
    {
        Context->IASetInputLayout(InputLayout.Get());
    }

    // Bind all shaders
    for (const auto& Shader : Shaders)
    {
        Shader->Bind(Context);
    }
}

void KShaderProgram::Unbind(ID3D11DeviceContext* Context) const
{
    // Unbind all shaders
    for (const auto& Shader : Shaders)
    {
        Shader->Unbind(Context);
    }

    // Remove input layout
    Context->IASetInputLayout(nullptr);
}

std::shared_ptr<KShader> KShaderProgram::GetShader(EShaderType Type) const
{
    for (const auto& Shader : Shaders)
    {
        if (Shader->GetType() == Type)
        {
            return Shader;
        }
    }
    return nullptr;
}

HRESULT KShaderProgram::CompileFromSource(ID3D11Device* Device, const std::string& VSSource,
                                          const std::string& PSSource, const std::string& VSEntry,
                                          const std::string& PSEntry)
{
    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, VSSource, VSEntry, EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, PSSource, PSEntry, EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    return S_OK;
}

HRESULT KShaderProgram::CreateDepthOnlyShader(ID3D11Device* Device)
{
    const std::string VSSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix LightView;
            matrix LightProjection;
        };

        struct VSInput
        {
            float3 Position : POSITION;
        };

        struct PSInput
        {
            float4 Position : SV_POSITION;
            float Depth : TEXCOORD0;
        };

        PSInput VS_Main(VSInput input)
        {
            PSInput output;
            float4 worldPos = mul(float4(input.Position, 1.0f), World);
            float4 viewPos = mul(worldPos, LightView);
            float4 projPos = mul(viewPos, LightProjection);
            output.Position = projPos;
            output.Depth = projPos.z / projPos.w;
            return output;
        }
    )";

    const std::string PSSource = R"(
        struct PSInput
        {
            float4 Position : SV_POSITION;
            float Depth : TEXCOORD0;
        };

        void PS_Main(PSInput input)
        {
        }
    )";

    HRESULT hr = CompileFromSource(Device, VSSource, PSSource, "VS_Main", "PS_Main");
    if (FAILED(hr)) return hr;

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Depth-only shader creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreatePhongShadowShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        #define MAX_POINT_LIGHTS 8
        #define MAX_SPOT_LIGHTS 4

        struct FPointLightData
        {
            float3 Position;
            float  Intensity;
            float4 Color;
            float  Radius;
            float  Falloff;
            float  Padding0;
            float  Padding1;
        };

        struct FSpotLightData
        {
            float3 Position;
            float  Intensity;
            float3 Direction;
            float  InnerCone;
            float4 Color;
            float  OuterCone;
            float  Radius;
            float  Falloff;
            float  Padding0;
        };

        cbuffer LightBuffer : register(b1)
        {
            float3 CameraPosition;
            int    NumPointLights;
            float3 DirLightDirection;
            int    NumSpotLights;
            float4 DirLightColor;
            float4 AmbientColor;
            FPointLightData PointLights[MAX_POINT_LIGHTS];
            FSpotLightData  SpotLights[MAX_SPOT_LIGHTS];
        }

        cbuffer ShadowBuffer : register(b2)
        {
            matrix LightViewProjection;
            float2 ShadowMapSize;
            float  DepthBias;
            int    PCFKernelSize;
            int    bShadowEnabled;
            float3 Padding;
        };

        Texture2D ShadowMap : register(t3);
        SamplerComparisonState ShadowSampler : register(s0);

        struct VS_INPUT
        {
            float3 Pos    : POSITION;
            float4 Color  : COLOR;
            float3 Normal : NORMAL;
            float2 TexCoord : TEXCOORD0;
        };

        struct PS_INPUT
        {
            float4 Pos       : SV_POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float3 WorldPos  : TEXCOORD0;
            float4 ShadowPos : TEXCOORD1;
        };

        float CalculateShadow(float4 shadowPos)
        {
            if (bShadowEnabled == 0)
                return 1.0f;

            float3 projCoords = shadowPos.xyz / shadowPos.w;
            projCoords.x = projCoords.x * 0.5f + 0.5f;
            projCoords.y = -projCoords.y * 0.5f + 0.5f;

            if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
                projCoords.y < 0.0f || projCoords.y > 1.0f ||
                projCoords.z > 1.0f)
                return 1.0f;

            float currentDepth = projCoords.z;
            float shadow = 0.0f;
            float2 texelSize = 1.0f / ShadowMapSize;

            int kernelSize = max(PCFKernelSize, 1);
            int halfKernel = kernelSize / 2;

            for (int x = -halfKernel; x <= halfKernel; ++x)
            {
                for (int y = -halfKernel; y <= halfKernel; ++y)
                {
                    float2 offset = float2(x, y) * texelSize;
                    shadow += ShadowMap.SampleCmpLevelZero(ShadowSampler, 
                        projCoords.xy + offset, currentDepth - DepthBias);
                }
            }

            shadow /= (float)(kernelSize * kernelSize);
            return shadow;
        }

        float3 CalculatePointLight(FPointLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        float3 CalculateSpotLight(FSpotLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float3 spotDir = normalize(-light.Direction);
            float cosAngle = dot(lightDir, spotDir);
            float cosInner = cos(light.InnerCone);
            float cosOuter = cos(light.OuterCone);
            
            if (cosAngle < cosOuter)
                return float3(0, 0, 0);
            
            float spotFactor = saturate((cosAngle - cosOuter) / (cosInner - cosOuter));
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity * spotFactor;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            float4 worldPos  = mul(float4(input.Pos, 1.0f), World);
            float4 viewPos   = mul(worldPos, View);
            output.Pos       = mul(viewPos, Projection);
            output.Color     = input.Color;
            output.Normal    = mul(input.Normal, (float3x3)World);
            output.WorldPos  = worldPos.xyz;
            output.ShadowPos = mul(worldPos, LightViewProjection);
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float3 normal   = normalize(input.Normal);
            float3 viewDir  = normalize(CameraPosition - input.WorldPos);
            
            float shadow = CalculateShadow(input.ShadowPos);
            
            float3 ambient  = AmbientColor.rgb * input.Color.rgb;
            
            float3 lightDir = normalize(-DirLightDirection);
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * DirLightColor.rgb * input.Color.rgb * shadow;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * DirLightColor.rgb * 0.4f * shadow;
            
            float3 result = ambient + diffuse + specular;
            
            [unroll]
            for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
            {
                if (i >= NumPointLights) break;
                result += CalculatePointLight(PointLights[i], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            [unroll]
            for (int j = 0; j < MAX_SPOT_LIGHTS; ++j)
            {
                if (j >= NumSpotLights) break;
                result += CalculateSpotLight(SpotLights[j], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            return saturate(float4(result, input.Color.a));
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Phong shader with shadows creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreatePBRShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        #define MAX_POINT_LIGHTS 8
        #define MAX_SPOT_LIGHTS 4

        struct FPointLightData
        {
            float3 Position;
            float  Intensity;
            float4 Color;
            float  Radius;
            float  Falloff;
            float  Padding0;
            float  Padding1;
        };

        struct FSpotLightData
        {
            float3 Position;
            float  Intensity;
            float3 Direction;
            float  InnerCone;
            float4 Color;
            float  OuterCone;
            float  Radius;
            float  Falloff;
            float  Padding0;
        };

        cbuffer LightBuffer : register(b1)
        {
            float3 CameraPosition;
            int    NumPointLights;
            float3 DirLightDirection;
            int    NumSpotLights;
            float4 DirLightColor;
            float4 AmbientColor;
            FPointLightData PointLights[MAX_POINT_LIGHTS];
            FSpotLightData  SpotLights[MAX_SPOT_LIGHTS];
        }

        cbuffer MaterialBuffer : register(b4)
        {
            float4 AlbedoColor;
            float  Metallic;
            float  Roughness;
            float  AO;
            float  EmissiveIntensity;
            float4 EmissiveColor;
            float  NormalIntensity;
            float  HeightScale;
            float  Padding0;
            float  Padding1;
        };

        cbuffer ShadowBuffer : register(b2)
        {
            matrix LightViewProjection;
            float2 ShadowMapSize;
            float  DepthBias;
            int    PCFKernelSize;
            int    bShadowEnabled;
            float3 ShadowPadding;
        };

        Texture2D ShadowMap : register(t3);
        SamplerComparisonState ShadowSampler : register(s0);

        Texture2D AlbedoMap    : register(t4);
        Texture2D NormalMap    : register(t5);
        Texture2D MetallicMap  : register(t6);
        Texture2D RoughnessMap : register(t7);
        Texture2D AOMap        : register(t8);
        Texture2D EmissiveMap  : register(t9);

        TextureCube IrradianceMap  : register(t10);
        TextureCube PrefilteredMap : register(t11);
        Texture2D   BRDFLUTMap     : register(t12);

        SamplerState MaterialSampler : register(s1);
        SamplerState IBL_Sampler     : register(s2);

        struct VS_INPUT
        {
            float3 Pos       : POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
        };

        struct PS_INPUT
        {
            float4 Pos       : SV_POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 WorldPos  : TEXCOORD1;
            float4 ShadowPos : TEXCOORD2;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
        };

        static const float PI = 3.14159265359f;

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

        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0f);
            float k = (r * r) / 8.0f;

            float nom = NdotV;
            float denom = NdotV * (1.0f - k) + k;

            return nom / max(denom, 0.0001f);
        }

        float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0f);
            float NdotL = max(dot(N, L), 0.0f);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);

            return ggx1 * ggx2;
        }

        float3 fresnelSchlick(float cosTheta, float3 F0)
        {
            return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
        }

        float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
        {
            return F0 + (max((1.0f - roughness).rrr, F0) - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
        }

        float3 CalculateIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness, float ao)
        {
            float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
            float3 R = reflect(-V, N);
            float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);

            float3 kS = F;
            float3 kD = (1.0f - kS) * (1.0f - metallic);

            float3 irradiance = IrradianceMap.Sample(IBL_Sampler, N).rgb;
            float3 diffuse = irradiance * albedo;

            const float MAX_REFLECTION_LOD = 4.0f;
            float3 prefilteredColor = PrefilteredMap.SampleLevel(IBL_Sampler, R, roughness * MAX_REFLECTION_LOD).rgb;
            float2 brdf = BRDFLUTMap.Sample(MaterialSampler, float2(max(dot(N, V), 0.0f), roughness)).rg;
            float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

            return (kD * diffuse + specular) * ao;
        }

        float CalculateShadow(float4 shadowPos)
        {
            if (bShadowEnabled == 0)
                return 1.0f;

            float3 projCoords = shadowPos.xyz / shadowPos.w;
            projCoords.x = projCoords.x * 0.5f + 0.5f;
            projCoords.y = -projCoords.y * 0.5f + 0.5f;

            if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
                projCoords.y < 0.0f || projCoords.y > 1.0f ||
                projCoords.z > 1.0f)
                return 1.0f;

            float currentDepth = projCoords.z;
            float shadow = 0.0f;
            float2 texelSize = 1.0f / ShadowMapSize;

            int kernelSize = max(PCFKernelSize, 1);
            int halfKernel = kernelSize / 2;

            for (int x = -halfKernel; x <= halfKernel; ++x)
            {
                for (int y = -halfKernel; y <= halfKernel; ++y)
                {
                    float2 offset = float2(x, y) * texelSize;
                    shadow += ShadowMap.SampleCmpLevelZero(ShadowSampler, 
                        projCoords.xy + offset, currentDepth - DepthBias);
                }
            }

            shadow /= (float)(kernelSize * kernelSize);
            return shadow;
        }

        float3 GetNormalFromMap(PS_INPUT input, float3 normal)
        {
            float3 tangentNormal = NormalMap.Sample(MaterialSampler, input.TexCoord).xyz * 2.0f - 1.0f;
            tangentNormal.xy *= NormalIntensity;

            float3 N = normalize(normal);
            float3 T = normalize(input.Tangent);
            float3 B = normalize(input.Bitangent);

            float3x3 TBN = float3x3(T, B, N);
            return normalize(mul(tangentNormal, TBN));
        }

        float3 CalculatePBRLighting(float3 N, float3 V, float3 worldPos, float3 albedo, float metallic, float roughness, float ao, float shadow)
        {
            float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

            float3 Lo = float3(0.0f, 0.0f, 0.0f);

            {
                float3 L = normalize(-DirLightDirection);
                float3 H = normalize(V + L);

                float NdotL = max(dot(N, L), 0.0f);

                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;

                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);

                float3 radiance = DirLightColor.rgb * shadow;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            [unroll]
            for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
            {
                if (i >= NumPointLights) break;

                float3 lightDir = PointLights[i].Position - worldPos;
                float distance = length(lightDir);

                if (distance > PointLights[i].Radius) continue;

                float3 L = normalize(lightDir);
                float3 H = normalize(V + L);

                float attenuation = 1.0f / (1.0f + PointLights[i].Falloff * distance * distance);
                attenuation *= PointLights[i].Intensity;

                float NdotL = max(dot(N, L), 0.0f);

                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;

                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);

                float3 radiance = PointLights[i].Color.rgb * attenuation;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            [unroll]
            for (int j = 0; j < MAX_SPOT_LIGHTS; ++j)
            {
                if (j >= NumSpotLights) break;

                float3 lightDir = SpotLights[j].Position - worldPos;
                float distance = length(lightDir);

                if (distance > SpotLights[j].Radius) continue;

                float3 L = normalize(lightDir);
                float3 H = normalize(V + L);

                float3 spotDir = normalize(-SpotLights[j].Direction);
                float cosAngle = dot(L, spotDir);
                float cosInner = cos(SpotLights[j].InnerCone);
                float cosOuter = cos(SpotLights[j].OuterCone);

                if (cosAngle < cosOuter) continue;

                float spotFactor = saturate((cosAngle - cosOuter) / (cosInner - cosOuter));
                float attenuation = 1.0f / (1.0f + SpotLights[j].Falloff * distance * distance);
                attenuation *= SpotLights[j].Intensity * spotFactor;

                float NdotL = max(dot(N, L), 0.0f);

                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;

                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);

                float3 radiance = SpotLights[j].Color.rgb * attenuation;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            float3 ambient = CalculateIBL(N, V, albedo, metallic, roughness, ao);
            float3 emissive = EmissiveColor.rgb * EmissiveIntensity;
            float3 color = ambient + Lo + emissive;

            return color;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            float4 worldPos  = mul(float4(input.Pos, 1.0f), World);
            float4 viewPos   = mul(worldPos, View);
            output.Pos       = mul(viewPos, Projection);
            output.Color     = input.Color;
            output.Normal    = mul(input.Normal, (float3x3)World);
            output.TexCoord  = input.TexCoord;
            output.WorldPos  = worldPos.xyz;
            output.ShadowPos = mul(worldPos, LightViewProjection);
            output.Tangent   = mul(input.Tangent, (float3x3)World);
            output.Bitangent = mul(input.Bitangent, (float3x3)World);
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float3 albedo = AlbedoMap.Sample(MaterialSampler, input.TexCoord).rgb * AlbedoColor.rgb;
            albedo = pow(albedo, float3(2.2f, 2.2f, 2.2f));

            float metallic = MetallicMap.Sample(MaterialSampler, input.TexCoord).r * Metallic;
            float roughness = RoughnessMap.Sample(MaterialSampler, input.TexCoord).r * Roughness;
            float ao = AOMap.Sample(MaterialSampler, input.TexCoord).r * AO;

            float3 normal = normalize(input.Normal);
            normal = GetNormalFromMap(input, normal);

            float3 V = normalize(CameraPosition - input.WorldPos);

            float shadow = CalculateShadow(input.ShadowPos);

            float3 color = CalculatePBRLighting(normal, V, input.WorldPos, albedo, metallic, roughness, ao, shadow);

            float3 emissive = EmissiveMap.Sample(MaterialSampler, input.TexCoord).rgb * EmissiveColor.rgb * EmissiveIntensity;
            color += emissive;

            return float4(color, AlbedoColor.a);
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("PBR shader creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreateSkinnedShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        #define MAX_BONES 256
        cbuffer BoneBuffer : register(b5)
        {
            matrix BoneMatrices[MAX_BONES];
        }

        #define MAX_POINT_LIGHTS 8
        #define MAX_SPOT_LIGHTS 4

        struct FPointLightData
        {
            float3 Position;
            float  Intensity;
            float4 Color;
            float  Radius;
            float  Falloff;
            float  Padding0;
            float  Padding1;
        };

        struct FSpotLightData
        {
            float3 Position;
            float  Intensity;
            float3 Direction;
            float  InnerCone;
            float4 Color;
            float  OuterCone;
            float  Radius;
            float  Falloff;
            float  Padding0;
        };

        cbuffer LightBuffer : register(b1)
        {
            float3 CameraPosition;
            int    NumPointLights;
            float3 DirLightDirection;
            int    NumSpotLights;
            float4 DirLightColor;
            float4 AmbientColor;
            FPointLightData PointLights[MAX_POINT_LIGHTS];
            FSpotLightData  SpotLights[MAX_SPOT_LIGHTS];
        }

        struct VS_INPUT
        {
            float3 Pos       : POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
            uint4  BoneIndices : BONEINDICES;
            float4 BoneWeights : BONEWEIGHTS;
        };

        struct PS_INPUT
        {
            float4 Pos       : SV_POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 WorldPos  : TEXCOORD1;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
        };

        float3 SkinPosition(float3 pos, uint4 boneIndices, float4 boneWeights)
        {
            float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            float4 weights = weightSum > 0.0f ? boneWeights / weightSum : boneWeights;

            float3 skinnedPos = float3(0, 0, 0);
            
            [unroll]
            for (int i = 0; i < 4; ++i)
            {
                if (weights[i] > 0.0f)
                {
                    skinnedPos += mul(float4(pos, 1.0f), BoneMatrices[boneIndices[i]]).xyz * weights[i];
                }
            }
            
            return skinnedPos;
        }

        float3 SkinNormal(float3 normal, uint4 boneIndices, float4 boneWeights)
        {
            float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            float4 weights = weightSum > 0.0f ? boneWeights / weightSum : boneWeights;

            float3 skinnedNormal = float3(0, 0, 0);
            
            [unroll]
            for (int i = 0; i < 4; ++i)
            {
                if (weights[i] > 0.0f)
                {
                    skinnedNormal += mul(normal, (float3x3)BoneMatrices[boneIndices[i]]) * weights[i];
                }
            }
            
            return skinnedNormal;
        }

        float3 CalculatePointLight(FPointLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        float3 CalculateSpotLight(FSpotLightData light, float3 normal, float3 worldPos, float3 viewDir)
        {
            float3 lightDir = light.Position - worldPos;
            float  distance = length(lightDir);
            
            if (distance > light.Radius)
                return float3(0, 0, 0);
            
            lightDir = normalize(lightDir);
            
            float3 spotDir = normalize(-light.Direction);
            float cosAngle = dot(lightDir, spotDir);
            float cosInner = cos(light.InnerCone);
            float cosOuter = cos(light.OuterCone);
            
            if (cosAngle < cosOuter)
                return float3(0, 0, 0);
            
            float spotFactor = saturate((cosAngle - cosOuter) / (cosInner - cosOuter));
            
            float attenuation = 1.0f / (1.0f + light.Falloff * distance * distance);
            attenuation *= light.Intensity * spotFactor;
            
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * light.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * light.Color.rgb * 0.3f;
            
            return (diffuse + specular) * attenuation;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            
            float3 skinnedPos = SkinPosition(input.Pos, input.BoneIndices, input.BoneWeights);
            float3 skinnedNormal = SkinNormal(input.Normal, input.BoneIndices, input.BoneWeights);
            float3 skinnedTangent = SkinNormal(input.Tangent, input.BoneIndices, input.BoneWeights);
            float3 skinnedBitangent = SkinNormal(input.Bitangent, input.BoneIndices, input.BoneWeights);
            
            float4 worldPos = mul(float4(skinnedPos, 1.0f), World);
            float4 viewPos = mul(worldPos, View);
            output.Pos = mul(viewPos, Projection);
            output.Color = input.Color;
            output.Normal = mul(skinnedNormal, (float3x3)World);
            output.TexCoord = input.TexCoord;
            output.WorldPos = worldPos.xyz;
            output.Tangent = mul(skinnedTangent, (float3x3)World);
            output.Bitangent = mul(skinnedBitangent, (float3x3)World);
            
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float3 normal = normalize(input.Normal);
            float3 viewDir = normalize(CameraPosition - input.WorldPos);
            
            float3 ambient = AmbientColor.rgb * input.Color.rgb;
            
            float3 lightDir = normalize(-DirLightDirection);
            float diff = max(dot(normal, lightDir), 0.0f);
            float3 diffuse = diff * DirLightColor.rgb * input.Color.rgb;
            
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 32.0f);
            float3 specular = spec * DirLightColor.rgb * 0.4f;
            
            float3 result = ambient + diffuse + specular;
            
            [unroll]
            for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
            {
                if (i >= NumPointLights) break;
                result += CalculatePointLight(PointLights[i], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            [unroll]
            for (int j = 0; j < MAX_SPOT_LIGHTS; ++j)
            {
                if (j >= NumSpotLights) break;
                result += CalculateSpotLight(SpotLights[j], normal, input.WorldPos, viewDir) * input.Color.rgb;
            }
            
            return saturate(float4(result, input.Color.a));
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 88, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Skinned mesh shader creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreateSkinnedPBRShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer TransformBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        }

        #define MAX_BONES 256
        cbuffer BoneBuffer : register(b5)
        {
            matrix BoneMatrices[MAX_BONES];
        }

        #define MAX_POINT_LIGHTS 8
        #define MAX_SPOT_LIGHTS 4

        struct FPointLightData
        {
            float3 Position;
            float  Intensity;
            float4 Color;
            float  Radius;
            float  Falloff;
            float  Padding0;
            float  Padding1;
        };

        struct FSpotLightData
        {
            float3 Position;
            float  Intensity;
            float3 Direction;
            float  InnerCone;
            float4 Color;
            float  OuterCone;
            float  Radius;
            float  Falloff;
            float  Padding0;
        };

        cbuffer LightBuffer : register(b1)
        {
            float3 CameraPosition;
            int    NumPointLights;
            float3 DirLightDirection;
            int    NumSpotLights;
            float4 DirLightColor;
            float4 AmbientColor;
            FPointLightData PointLights[MAX_POINT_LIGHTS];
            FSpotLightData  SpotLights[MAX_SPOT_LIGHTS];
        }

        cbuffer ShadowBuffer : register(b2)
        {
            matrix LightViewProjection;
            float2 ShadowMapSize;
            float  DepthBias;
            int    PCFKernelSize;
            int    bShadowEnabled;
            float3 ShadowPadding;
        };

        Texture2D ShadowMap : register(t3);
        SamplerComparisonState ShadowSampler : register(s0);

        cbuffer MaterialBuffer : register(b4)
        {
            float4 AlbedoColor;
            float  Metallic;
            float  Roughness;
            float  AO;
            float  EmissiveIntensity;
            float4 EmissiveColor;
            float  NormalIntensity;
            float  HeightScale;
            float  Padding0;
            float  Padding1;
        };

        Texture2D AlbedoMap    : register(t4);
        Texture2D NormalMap    : register(t5);
        Texture2D MetallicMap  : register(t6);
        Texture2D RoughnessMap : register(t7);
        Texture2D AOMap        : register(t8);
        Texture2D EmissiveMap  : register(t9);

        TextureCube IrradianceMap  : register(t10);
        TextureCube PrefilteredMap : register(t11);
        Texture2D   BRDFLUTMap     : register(t12);

        SamplerState MaterialSampler : register(s1);
        SamplerState IBL_Sampler     : register(s2);

        struct VS_INPUT
        {
            float3 Pos       : POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
            uint4  BoneIndices : BONEINDICES;
            float4 BoneWeights : BONEWEIGHTS;
        };

        struct PS_INPUT
        {
            float4 Pos       : SV_POSITION;
            float4 Color     : COLOR;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 WorldPos  : TEXCOORD1;
            float4 ShadowPos : TEXCOORD2;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
        };

        float3 SkinPosition(float3 pos, uint4 boneIndices, float4 boneWeights)
        {
            float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            float4 weights = weightSum > 0.0f ? boneWeights / weightSum : boneWeights;
            float3 skinnedPos = float3(0, 0, 0);
            [unroll]
            for (int i = 0; i < 4; ++i)
            {
                if (weights[i] > 0.0f)
                    skinnedPos += mul(float4(pos, 1.0f), BoneMatrices[boneIndices[i]]).xyz * weights[i];
            }
            return skinnedPos;
        }

        float3 SkinNormal(float3 normal, uint4 boneIndices, float4 boneWeights)
        {
            float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            float4 weights = weightSum > 0.0f ? boneWeights / weightSum : boneWeights;
            float3 skinnedNormal = float3(0, 0, 0);
            [unroll]
            for (int i = 0; i < 4; ++i)
            {
                if (weights[i] > 0.0f)
                    skinnedNormal += mul(normal, (float3x3)BoneMatrices[boneIndices[i]]) * weights[i];
            }
            return skinnedNormal;
        }

        float3 SkinTangent(float3 tangent, uint4 boneIndices, float4 boneWeights)
        {
            float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            float4 weights = weightSum > 0.0f ? boneWeights / weightSum : boneWeights;
            float3 skinnedTangent = float3(0, 0, 0);
            [unroll]
            for (int i = 0; i < 4; ++i)
            {
                if (weights[i] > 0.0f)
                    skinnedTangent += mul(tangent, (float3x3)BoneMatrices[boneIndices[i]]) * weights[i];
            }
            return skinnedTangent;
        }

        static const float PI = 3.14159265359f;

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

        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0f);
            float k = (r * r) / 8.0f;
            float nom = NdotV;
            float denom = NdotV * (1.0f - k) + k;
            return nom / max(denom, 0.0001f);
        }

        float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0f);
            float NdotL = max(dot(N, L), 0.0f);
            return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
        }

        float3 fresnelSchlick(float cosTheta, float3 F0)
        {
            return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
        }

        float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
        {
            return F0 + (max((1.0f - roughness).rrr, F0) - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
        }

        float3 CalculateIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness, float ao)
        {
            float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
            float3 R = reflect(-V, N);
            float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);

            float3 kS = F;
            float3 kD = (1.0f - kS) * (1.0f - metallic);

            float3 irradiance = IrradianceMap.Sample(IBL_Sampler, N).rgb;
            float3 diffuse = irradiance * albedo;

            const float MAX_REFLECTION_LOD = 4.0f;
            float3 prefilteredColor = PrefilteredMap.SampleLevel(IBL_Sampler, R, roughness * MAX_REFLECTION_LOD).rgb;
            float2 brdf = BRDFLUTMap.Sample(MaterialSampler, float2(max(dot(N, V), 0.0f), roughness)).rg;
            float3 specular = prefilteredColor * (F * brdf.x + brdf.y);

            return (kD * diffuse + specular) * ao;
        }

        float CalculateShadow(float4 shadowPos)
        {
            if (bShadowEnabled == 0)
                return 1.0f;

            float3 projCoords = shadowPos.xyz / shadowPos.w;
            projCoords.x = projCoords.x * 0.5f + 0.5f;
            projCoords.y = -projCoords.y * 0.5f + 0.5f;

            if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
                projCoords.y < 0.0f || projCoords.y > 1.0f ||
                projCoords.z > 1.0f)
                return 1.0f;

            float currentDepth = projCoords.z;
            float shadow = 0.0f;
            float2 texelSize = 1.0f / ShadowMapSize;

            int kernelSize = max(PCFKernelSize, 1);
            int halfKernel = kernelSize / 2;

            for (int x = -halfKernel; x <= halfKernel; ++x)
            {
                for (int y = -halfKernel; y <= halfKernel; ++y)
                {
                    float2 offset = float2(x, y) * texelSize;
                    shadow += ShadowMap.SampleCmpLevelZero(ShadowSampler,
                        projCoords.xy + offset, currentDepth - DepthBias);
                }
            }

            shadow /= (float)(kernelSize * kernelSize);
            return shadow;
        }

        float3 GetNormalFromMap(PS_INPUT input, float3 normal)
        {
            float3 tangentNormal = NormalMap.Sample(MaterialSampler, input.TexCoord).xyz * 2.0f - 1.0f;
            tangentNormal.xy *= NormalIntensity;
            float3 N = normalize(normal);
            float3 T = normalize(input.Tangent);
            float3 B = normalize(input.Bitangent);
            float3x3 TBN = float3x3(T, B, N);
            return normalize(mul(tangentNormal, TBN));
        }

        float3 CalculatePBRLighting(float3 N, float3 V, float3 worldPos, float3 albedo, float metallic, float roughness, float ao, float shadow)
        {
            float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
            float3 Lo = float3(0.0f, 0.0f, 0.0f);

            {
                float3 L = normalize(-DirLightDirection);
                float3 H = normalize(V + L);
                float NdotL = max(dot(N, L), 0.0f);
                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);
                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;
                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);
                float3 radiance = DirLightColor.rgb * shadow;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            [unroll]
            for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
            {
                if (i >= NumPointLights) break;
                float3 lightDir = PointLights[i].Position - worldPos;
                float distance = length(lightDir);
                if (distance > PointLights[i].Radius) continue;
                float3 L = normalize(lightDir);
                float3 H = normalize(V + L);
                float attenuation = 1.0f / (1.0f + PointLights[i].Falloff * distance * distance);
                attenuation *= PointLights[i].Intensity;
                float NdotL = max(dot(N, L), 0.0f);
                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);
                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;
                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);
                float3 radiance = PointLights[i].Color.rgb * attenuation;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            [unroll]
            for (int j = 0; j < MAX_SPOT_LIGHTS; ++j)
            {
                if (j >= NumSpotLights) break;
                float3 lightDir = SpotLights[j].Position - worldPos;
                float distance = length(lightDir);
                if (distance > SpotLights[j].Radius) continue;
                float3 L = normalize(lightDir);
                float3 spotDir = normalize(-SpotLights[j].Direction);
                float cosAngle = dot(L, spotDir);
                float cosInner = cos(SpotLights[j].InnerCone);
                float cosOuter = cos(SpotLights[j].OuterCone);
                if (cosAngle < cosOuter) continue;
                float spotFactor = saturate((cosAngle - cosOuter) / (cosInner - cosOuter));
                float attenuation = 1.0f / (1.0f + SpotLights[j].Falloff * distance * distance);
                attenuation *= SpotLights[j].Intensity * spotFactor;
                float3 H = normalize(V + L);
                float NdotL = max(dot(N, L), 0.0f);
                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);
                float3 numerator = NDF * G * F;
                float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
                float3 specular = numerator / denominator;
                float3 kS = F;
                float3 kD = (1.0f - kS) * (1.0f - metallic);
                float3 radiance = SpotLights[j].Color.rgb * attenuation;
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }

            float3 ambient = CalculateIBL(N, V, albedo, metallic, roughness, ao);
            float3 emissive = EmissiveColor.rgb * EmissiveIntensity;
            float3 color = ambient + Lo + emissive;
            return color;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            float3 skinnedPos = SkinPosition(input.Pos, input.BoneIndices, input.BoneWeights);
            float3 skinnedNormal = SkinNormal(input.Normal, input.BoneIndices, input.BoneWeights);
            float3 skinnedTangent = SkinTangent(input.Tangent, input.BoneIndices, input.BoneWeights);
            float3 skinnedBitangent = SkinTangent(input.Bitangent, input.BoneIndices, input.BoneWeights);
            float4 worldPos = mul(float4(skinnedPos, 1.0f), World);
            float4 viewPos = mul(worldPos, View);
            output.Pos = mul(viewPos, Projection);
            output.Color = input.Color;
            output.Normal = mul(skinnedNormal, (float3x3)World);
            output.TexCoord = input.TexCoord;
            output.WorldPos = worldPos.xyz;
            output.ShadowPos = mul(worldPos, LightViewProjection);
            output.Tangent = mul(skinnedTangent, (float3x3)World);
            output.Bitangent = mul(skinnedBitangent, (float3x3)World);
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float3 albedo = AlbedoMap.Sample(MaterialSampler, input.TexCoord).rgb * AlbedoColor.rgb;
            albedo = pow(albedo, float3(2.2f, 2.2f, 2.2f));

            float metallic = MetallicMap.Sample(MaterialSampler, input.TexCoord).r * Metallic;
            float roughness = RoughnessMap.Sample(MaterialSampler, input.TexCoord).r * Roughness;
            float ao = AOMap.Sample(MaterialSampler, input.TexCoord).r * AO;

            float3 normal = normalize(input.Normal);
            normal = GetNormalFromMap(input, normal);

            float3 V = normalize(CameraPosition - input.WorldPos);
            float shadow = CalculateShadow(input.ShadowPos);
            float3 color = CalculatePBRLighting(normal, V, input.WorldPos, albedo, metallic, roughness, ao, shadow);

            return float4(color, AlbedoColor.a);
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 88, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Skinned PBR shader creation completed");
    return S_OK;
}

HRESULT KShaderProgram::CreateWaterShader(ID3D11Device* Device)
{
    const std::string ShaderSource = R"(
        cbuffer WaterBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
            matrix ReflectionView;
            
            float4 DeepColor;
            float4 ShallowColor;
            float4 FoamColor;
            float4 CameraPosition;
            
            float4 WaveDirections[4];
            float4 WaveParams[4];
            
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
            
            uint WaveCount;
            float3 Padding2;
        };

        Texture2D NormalMap    : register(t0);
        Texture2D NormalMap2   : register(t1);
        Texture2D DuDvMap      : register(t2);
        Texture2D FoamTexture  : register(t3);
        Texture2D ReflectionTexture : register(t4);
        Texture2D RefractionTexture : register(t5);
        Texture2D DepthTexture : register(t6);

        SamplerState WaterSampler : register(s0);
        SamplerState ClampedSampler : register(s1);

        struct VS_INPUT
        {
            float3 Pos       : POSITION;
            float3 Normal    : NORMAL;
            float2 TexCoord  : TEXCOORD0;
            float3 Tangent   : TANGENT;
            float3 Bitangent : BINORMAL;
        };

        struct PS_INPUT
        {
            float4 Pos           : SV_POSITION;
            float3 WorldPos      : TEXCOORD0;
            float2 TexCoord      : TEXCOORD1;
            float4 ReflectionPos : TEXCOORD2;
            float4 RefractionPos : TEXCOORD3;
            float3 Normal        : NORMAL;
            float3 Tangent       : TANGENT;
            float3 Bitangent     : BINORMAL;
            float4 ScreenPos     : TEXCOORD4;
        };

        float3 GerstnerWave(float2 position, float4 waveDir, float4 waveParams)
        {
            float steepness = waveParams.w;
            float wavelength = waveParams.y;
            float speed = waveParams.z;
            float amplitude = waveParams.x;
            
            float k = 2.0f * 3.14159f / wavelength;
            float c = sqrt(9.8f / k);
            float2 d = normalize(waveDir.xy);
            float f = k * (dot(d, position) - c * Time * speed);
            float a = steepness / k;
            
            return float3(
                d.x * a * cos(f),
                amplitude * sin(f),
                d.y * a * cos(f)
            );
        }

        float3 CalculateWavePosition(float3 pos)
        {
            float3 waveOffset = float3(0, 0, 0);
            
            [unroll]
            for (uint i = 0; i < 4; ++i)
            {
                if (i >= WaveCount) break;
                waveOffset += GerstnerWave(pos.xz, WaveDirections[i], WaveParams[i]);
            }
            
            return pos + waveOffset;
        }

        float3 CalculateWaveNormal(float3 pos)
        {
            float eps = 0.1f;
            float3 p0 = CalculateWavePosition(pos);
            float3 px = CalculateWavePosition(pos + float3(eps, 0, 0));
            float3 pz = CalculateWavePosition(pos + float3(0, 0, eps));
            
            float3 tangent = normalize(px - p0);
            float3 bitangent = normalize(pz - p0);
            
            return normalize(cross(tangent, bitangent));
        }

        float Fresnel(float3 viewDir, float3 normal)
        {
            float cosTheta = saturate(dot(viewDir, normal));
            return FresnelBias + (1.0f - FresnelBias) * pow(1.0f - cosTheta, FresnelPower);
        }

        float2 CalculateDuDv(float2 uv, float time)
        {
            float2 duDv = DuDvMap.Sample(WaterSampler, uv + time * 0.02f).rg * 2.0f - 1.0f;
            duDv += DuDvMap.Sample(WaterSampler, uv * 2.0f - time * 0.01f).rg * 2.0f - 1.0f;
            return duDv * RefractionScale;
        }

        PS_INPUT VS(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            
            float3 wavePos = CalculateWavePosition(input.Pos);
            
            float4 worldPos = mul(float4(wavePos, 1.0f), World);
            float4 viewPos = mul(worldPos, View);
            output.Pos = mul(viewPos, Projection);
            
            output.WorldPos = worldPos.xyz;
            output.TexCoord = input.TexCoord * NormalMapTiling;
            
            float4 reflectionPos = mul(worldPos, ReflectionView);
            output.ReflectionPos = mul(reflectionPos, Projection);
            
            output.RefractionPos = output.Pos;
            output.ScreenPos = output.Pos;
            
            output.Normal = CalculateWaveNormal(input.Pos);
            output.Tangent = input.Tangent;
            output.Bitangent = input.Bitangent;
            
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target
        {
            float2 ndc = input.ScreenPos.xy / input.ScreenPos.w;
            float2 screenUV = ndc * 0.5f + 0.5f;
            screenUV.y = 1.0f - screenUV.y;
            
            float2 duDv = CalculateDuDv(input.TexCoord, Time);
            
            float2 distortedUV = clamp(screenUV + duDv, 0.001f, 0.999f);
            
            float4 reflectionColor = ReflectionTexture.Sample(ClampedSampler, distortedUV);
            float4 refractionColor = RefractionTexture.Sample(ClampedSampler, distortedUV);
            
            float3 normal1 = NormalMap.Sample(WaterSampler, input.TexCoord + Time * 0.05f).rgb * 2.0f - 1.0f;
            float3 normal2 = NormalMap2.Sample(WaterSampler, input.TexCoord - Time * 0.03f).rgb * 2.0f - 1.0f;
            float3 normal = normalize(normal1 + normal2);
            
            float3 viewDir = normalize(CameraPosition.xyz - input.WorldPos);
            float fresnel = Fresnel(viewDir, input.Normal);
            
            float depthValue = DepthTexture.Sample(ClampedSampler, distortedUV).r;
            float floorDepth = depthValue * DepthMaxDistance;
            float surfaceDepth = input.ScreenPos.z * DepthMaxDistance;
            float waterDepth = floorDepth - surfaceDepth;
            
            float depthFactor = saturate(waterDepth / DepthMaxDistance);
            float4 waterColor = lerp(ShallowColor, DeepColor, depthFactor);
            
            float4 finalColor = lerp(refractionColor * waterColor, reflectionColor, fresnel * ReflectionScale);
            finalColor = lerp(finalColor, waterColor, Transparency * (1.0f - depthFactor));
            
            float foamFactor = saturate((1.0f - waterDepth / FoamThreshold) * FoamIntensity);
            float4 foam = FoamTexture.Sample(WaterSampler, input.TexCoord);
            finalColor = lerp(finalColor, foam * FoamColor, foamFactor * foam.r);
            
            float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0f), 256.0f);
            finalColor.rgb += spec * 0.5f;
            
            return saturate(finalColor);
        }
    )";

    auto VS = std::make_shared<KShader>();
    HRESULT hr = VS->CompileFromString(Device, ShaderSource, "VS", EShaderType::Vertex);
    if (FAILED(hr)) return hr;

    auto PS = std::make_shared<KShader>();
    hr = PS->CompileFromString(Device, ShaderSource, "PS", EShaderType::Pixel);
    if (FAILED(hr)) return hr;

    AddShader(VS);
    AddShader(PS);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr)) return hr;

    LOG_INFO("Water shader creation completed");
    return S_OK;
}