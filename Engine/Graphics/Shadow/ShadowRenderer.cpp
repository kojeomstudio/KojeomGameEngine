#include "ShadowRenderer.h"

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
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = Context->Map(caster.Mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            FConstantBuffer* cb = static_cast<FConstantBuffer*>(mapped.pData);
            cb->WorldMatrix = XMMatrixTranspose(caster.WorldMatrix);
            cb->ViewMatrix = XMMatrixTranspose(LightViewProjection);
            cb->ProjectionMatrix = XMMatrixIdentity();
            Context->Unmap(caster.Mesh->GetConstantBuffer(), 0);
        }

        caster.Mesh->Render(Context);
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
    ShadowConstantBuffer.Reset();
    ShadowCasters.clear();
    bInitialized = false;
}
