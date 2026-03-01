#include "Shader.h"

// UShader class implementation

HRESULT KShader::LoadFromFile(ID3D11Device* Device, const std::wstring& Filename, 
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