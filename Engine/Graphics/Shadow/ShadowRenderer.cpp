#include "ShadowRenderer.h"
#include "../../Assets/SkeletalMeshComponent.h"

HRESULT KShadowRenderer::Initialize(ID3D11Device* Device, const FShadowConfig& Config)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for shadow renderer");
        return E_INVALIDARG;
    }

    HRESULT hr = ShadowMap.Initialize(Device, Config);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize shadow map");
        return hr;
    }

    hr = CreateShadowShader(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow shader");
        return hr;
    }

    hr = CreateSkinnedShadowShader(Device);
    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create skinned shadow shader, skeletal mesh shadows will be disabled");
    }

    hr = CreateShadowBuffer(Device);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow constant buffer");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Shadow renderer initialized");
    return S_OK;
}

HRESULT KShadowRenderer::CreateShadowShader(ID3D11Device* Device)
{
    ShadowShader = std::make_shared<KShaderProgram>();
    
    std::string vsCode = R"(
cbuffer ShadowTransform : register(b0)
{
    matrix LightViewProjection;
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
    float4 projPos = mul(float4(input.Position, 1.0f), LightViewProjection);
    output.Position = projPos;
    output.Depth = projPos.z / projPos.w;
    return output;
}
)";

    std::string psCode = R"(
struct PSInput
{
    float4 Position : SV_POSITION;
    float Depth : TEXCOORD0;
};

void PS_Main(PSInput input)
{
}
)";

    HRESULT hr = ShadowShader->CompileFromSource(Device, vsCode, psCode, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile shadow shader from source");
        return hr;
    }

    return S_OK;
}

HRESULT KShadowRenderer::CreateSkinnedShadowShader(ID3D11Device* Device)
{
    SkinnedShadowShader = std::make_shared<KShaderProgram>();

    std::string vsCode = R"(
cbuffer ShadowTransform : register(b0)
{
    matrix LightViewProjection;
};

#define MAX_BONES 256
cbuffer BoneTransforms : register(b5)
{
    matrix BoneMatrices[MAX_BONES];
};

struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float Depth : TEXCOORD0;
};

PSInput VS_Main(VSInput input)
{
    PSInput output;

    float4x4 skinMatrix = (float4x4)0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        if (input.BoneWeights[i] > 0.0f)
        {
            skinMatrix += BoneMatrices[input.BoneIndices[i]] * input.BoneWeights[i];
        }
    }

    float4 skinnedPos = mul(float4(input.Position, 1.0f), skinMatrix);
    float4 projPos = mul(skinnedPos, LightViewProjection);
    output.Position = projPos;
    output.Depth = projPos.z / projPos.w;
    return output;
}
)";

    std::string psCode = R"(
struct PSInput
{
    float4 Position : SV_POSITION;
    float Depth : TEXCOORD0;
};

void PS_Main(PSInput input)
{
}
)";

    HRESULT hr = SkinnedShadowShader->CompileFromSource(Device, vsCode, psCode, "VS_Main", "PS_Main");
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to compile skinned shadow shader from source");
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 88, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = SkinnedShadowShader->CreateInputLayout(Device, Layout, ARRAYSIZE(Layout));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create skinned shadow shader input layout");
        return hr;
    }

    return S_OK;
}

HRESULT KShadowRenderer::CreateShadowBuffer(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FShadowBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &ShadowConstantBuffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create shadow constant buffer");
        return hr;
    }

    return S_OK;
}

void KShadowRenderer::BeginShadowPass(ID3D11DeviceContext* Context, const FDirectionalLight& Light,
                                       const XMFLOAT3& SceneCenter, float SceneRadius)
{
    if (!Context || !bInitialized) return;

    XMVECTOR lightDir = XMVectorSet(Light.Direction.x, Light.Direction.y, Light.Direction.z, 0.0f);
    XMVECTOR sceneCenterVec = XMVectorSet(SceneCenter.x, SceneCenter.y, SceneCenter.z, 0.0f);
    XMVECTOR lightPos = XMVectorSubtract(sceneCenterVec, XMVectorScale(lightDir, SceneRadius * 2.0f));

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, sceneCenterVec, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(
        -SceneRadius, SceneRadius,
        -SceneRadius, SceneRadius,
        0.1f, SceneRadius * 4.0f
    );

    LightViewProjection = lightView * lightProj;

    ShadowMap.ClearDepth(Context);
    ShadowMap.BeginShadowPass(Context);

    UpdateShadowBuffer(Context);
}

void KShadowRenderer::UpdateShadowBuffer(ID3D11DeviceContext* Context)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(ShadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        FShadowBuffer* buffer = static_cast<FShadowBuffer*>(mapped.pData);
        buffer->LightViewProjection = XMMatrixTranspose(LightViewProjection);
        buffer->ShadowMapSize = XMFLOAT2(static_cast<float>(ShadowMap.GetResolution()), 
                                         static_cast<float>(ShadowMap.GetResolution()));
        buffer->DepthBias = ShadowMap.GetConfig().DepthBias;
        buffer->PCFKernelSize = ShadowMap.GetConfig().PCFKernelSize;
        Context->Unmap(ShadowConstantBuffer.Get(), 0);
    }
}

void KShadowRenderer::RenderShadowCasters(ID3D11DeviceContext* Context)
{
    if (!Context || !bInitialized || ShadowCasters.empty()) return;

    ShadowShader->Bind(Context);
    Context->VSSetConstantBuffers(0, 1, ShadowConstantBuffer.GetAddressOf());

    for (const auto& caster : ShadowCasters)
    {
        if (caster.bIsSkeletal)
        {
            if (!SkinnedShadowShader)
            {
                continue;
            }

            XMMATRIX worldLightVP = XMMatrixTranspose(caster.WorldMatrix) * LightViewProjection;
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = Context->Map(ShadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (SUCCEEDED(hr))
            {
                FShadowBuffer* buffer = static_cast<FShadowBuffer*>(mapped.pData);
                buffer->LightViewProjection = XMMatrixTranspose(worldLightVP);
                buffer->ShadowMapSize = XMFLOAT2(static_cast<float>(ShadowMap.GetResolution()),
                                                  static_cast<float>(ShadowMap.GetResolution()));
                buffer->DepthBias = ShadowMap.GetConfig().DepthBias;
                buffer->PCFKernelSize = ShadowMap.GetConfig().PCFKernelSize;
                Context->Unmap(ShadowConstantBuffer.Get(), 0);
            }

            SkinnedShadowShader->Bind(Context);
            Context->VSSetConstantBuffers(0, 1, ShadowConstantBuffer.GetAddressOf());
            Context->VSSetConstantBuffers(5, 1, &caster.BoneMatrixBuffer);

            UINT32 stride = sizeof(FSkinnedVertex);
            UINT32 offset = 0;
            ID3D11Buffer* nullBuffer = nullptr;
            Context->IASetVertexBuffers(0, 1, &caster.SkeletalVertexBuffer, &stride, &offset);
            Context->IASetIndexBuffer(caster.SkeletalIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Context->DrawIndexed(caster.SkeletalIndexCount, 0, 0);

            Context->IASetVertexBuffers(0, 1, &nullBuffer, &offset, &offset);
            Context->IASetIndexBuffer(nullBuffer, DXGI_FORMAT_R32_UINT, 0);
            ID3D11Buffer* nullCB = nullptr;
            Context->VSSetConstantBuffers(5, 1, &nullCB);

            ShadowShader->Bind(Context);
        }
        else
        {
            XMMATRIX worldLightVP = XMMatrixTranspose(caster.WorldMatrix) * LightViewProjection;
            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = Context->Map(ShadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (SUCCEEDED(hr))
            {
                FShadowBuffer* buffer = static_cast<FShadowBuffer*>(mapped.pData);
                buffer->LightViewProjection = XMMatrixTranspose(worldLightVP);
                buffer->ShadowMapSize = XMFLOAT2(static_cast<float>(ShadowMap.GetResolution()),
                                                  static_cast<float>(ShadowMap.GetResolution()));
                buffer->DepthBias = ShadowMap.GetConfig().DepthBias;
                buffer->PCFKernelSize = ShadowMap.GetConfig().PCFKernelSize;
                Context->Unmap(ShadowConstantBuffer.Get(), 0);
            }
            Context->VSSetConstantBuffers(0, 1, ShadowConstantBuffer.GetAddressOf());

            UINT32 stride = sizeof(FVertex);
            UINT32 offset = 0;
            ID3D11Buffer* vb = caster.Mesh->GetVertexBuffer();
            Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
            if (caster.Mesh->HasIndices())
            {
                Context->IASetIndexBuffer(caster.Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
                Context->DrawIndexed(caster.Mesh->GetIndexCount(), 0, 0);
            }
            else
            {
                Context->Draw(caster.Mesh->GetVertexCount(), 0);
            }
        }
    }
}

void KShadowRenderer::EndShadowPass(ID3D11DeviceContext* Context)
{
    if (!Context) return;
    ShadowMap.EndShadowPass(Context);
}

void KShadowRenderer::AddShadowCaster(const FShadowCaster& Caster)
{
    ShadowCasters.push_back(Caster);
}

void KShadowRenderer::ClearShadowCasters()
{
    ShadowCasters.clear();
}

void KShadowRenderer::BindShadowMap(ID3D11DeviceContext* Context, UINT32 Slot)
{
    if (!Context || !bInitialized) return;
    ID3D11ShaderResourceView* srv = ShadowMap.GetDepthSRV();
    Context->PSSetShaderResources(Slot, 1, &srv);
}

void KShadowRenderer::UnbindShadowMap(ID3D11DeviceContext* Context, UINT32 Slot)
{
    if (!Context) return;
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(Slot, 1, &nullSRV);
}

void KShadowRenderer::Cleanup()
{
    ShadowMap.Cleanup();
    ShadowShader.reset();
    SkinnedShadowShader.reset();
    ShadowConstantBuffer.Reset();
    ShadowCasters.clear();
    bInitialized = false;
}
