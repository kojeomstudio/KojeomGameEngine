#include "CascadedShadowRenderer.h"
#include <sstream>

HRESULT KCascadedShadowRenderer::Initialize(ID3D11Device* Device, const FCascadedShadowConfig& InConfig)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for cascaded shadow renderer");
        return E_INVALIDARG;
    }

    Config = InConfig;

    HRESULT hr = CascadedShadowMap.Initialize(Device, Config);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize cascaded shadow map");
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
        LOG_ERROR("Failed to create shadow buffer");
        return hr;
    }

    bInitialized = true;
    LOG_INFO("Cascaded shadow renderer initialized successfully");
    return S_OK;
}

void KCascadedShadowRenderer::Cleanup()
{
    CascadedShadowMap.Cleanup();
    ShadowShader.reset();
    ShadowConstantBuffer.Reset();
    ShadowCasters.clear();
    bInitialized = false;
}

HRESULT KCascadedShadowRenderer::CreateShadowShader(ID3D11Device* Device)
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
        LOG_ERROR("Failed to compile cascaded shadow shader from source");
        return hr;
    }

    return S_OK;
}

HRESULT KCascadedShadowRenderer::CreateShadowBuffer(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FCascadedShadowBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return Device->CreateBuffer(&bufferDesc, nullptr, &ShadowConstantBuffer);
}

void KCascadedShadowRenderer::CalculateCascadeSplits(const FDirectionalLight& Light, KCamera* Camera)
{
    if (!Camera)
        return;

    float nearPlane = Camera->GetNearZ();
    float farPlane = Camera->GetFarZ();
    
    CascadedShadowMap.UpdateSplitDistances(nearPlane, farPlane);
    
    const auto& splitDistances = CascadedShadowMap.GetConfig().SplitDistances;
    
    XMVECTOR lightDir = XMLoadFloat3(&Light.Direction);
    lightDir = XMVector3Normalize(lightDir);
    
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetX(XMVector3Dot(lightDir, lightUp))) > 0.99f)
    {
        lightUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }
    
    XMMATRIX viewMatrix = Camera->GetViewMatrix();
    XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);
    XMMATRIX proj = Camera->GetProjectionMatrix();
    
    for (UINT32 i = 0; i < CascadedShadowMap.GetCascadeCount(); ++i)
    {
        float nearDist = splitDistances[i];
        float farDist = splitDistances[i + 1];
        
        CascadeSplits[i].NearDistance = nearDist;
        CascadeSplits[i].FarDistance = farDist;
        
        XMVECTOR frustumCorners[8];
        float proj11 = XMVectorGetX(proj.r[0]);
        float proj22 = XMVectorGetX(proj.r[1]);
        float nearX = nearDist / proj11;
        float nearY = nearDist / proj22;
        float farX = farDist / proj11;
        float farY = farDist / proj22;
        
        frustumCorners[0] = XMVectorSet(-nearX, -nearY, nearDist, 1.0f);
        frustumCorners[1] = XMVectorSet( nearX, -nearY, nearDist, 1.0f);
        frustumCorners[2] = XMVectorSet( nearX,  nearY, nearDist, 1.0f);
        frustumCorners[3] = XMVectorSet(-nearX,  nearY, nearDist, 1.0f);
        frustumCorners[4] = XMVectorSet(-farX, -farY, farDist, 1.0f);
        frustumCorners[5] = XMVectorSet( farX, -farY, farDist, 1.0f);
        frustumCorners[6] = XMVectorSet( farX,  farY, farDist, 1.0f);
        frustumCorners[7] = XMVectorSet(-farX,  farY, farDist, 1.0f);
        
        XMVECTOR minBounds = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR maxBounds = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);
        
        for (int j = 0; j < 8; ++j)
        {
            frustumCorners[j] = XMVector3TransformCoord(frustumCorners[j], invView);
            minBounds = XMVectorMin(minBounds, frustumCorners[j]);
            maxBounds = XMVectorMax(maxBounds, frustumCorners[j]);
        }
        
        XMVECTOR center = (minBounds + maxBounds) * 0.5f;
        XMMATRIX lightView = XMMatrixLookAtLH(center, center + lightDir, lightUp);
        
        for (int j = 0; j < 8; ++j)
        {
            frustumCorners[j] = XMVector3TransformCoord(frustumCorners[j], lightView);
        }
        
        minBounds = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        maxBounds = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);
        
        for (int j = 0; j < 8; ++j)
        {
            minBounds = XMVectorMin(minBounds, frustumCorners[j]);
            maxBounds = XMVectorMax(maxBounds, frustumCorners[j]);
        }
        
        float radius = XMVectorGetX(XMVector3Length(maxBounds - minBounds)) * 0.5f;
        XMVECTOR lightPos = center - lightDir * radius;
        
        lightView = XMMatrixLookAtLH(lightPos, center, lightUp);
        
        XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(
            XMVectorGetX(minBounds), XMVectorGetX(maxBounds),
            XMVectorGetY(minBounds), XMVectorGetY(maxBounds),
            XMVectorGetZ(minBounds) - radius, XMVectorGetZ(maxBounds) + radius
        );
        
        CascadeSplits[i].LightViewProjection = lightView * lightProj;
        XMStoreFloat3(&CascadeSplits[i].MinBounds, minBounds);
        XMStoreFloat3(&CascadeSplits[i].MaxBounds, maxBounds);
    }
}

void KCascadedShadowRenderer::BeginShadowPass(ID3D11DeviceContext* Context, const FDirectionalLight& Light, KCamera* Camera)
{
    if (!Context || !Camera)
        return;
        
    CalculateCascadeSplits(Light, Camera);
    CascadedShadowMap.ClearAllDepths(Context);
}

void KCascadedShadowRenderer::BeginCascadePass(ID3D11DeviceContext* Context, UINT32 CascadeIndex)
{
    if (CascadeIndex >= CascadedShadowMap.GetCascadeCount())
        return;
        
    CascadedShadowMap.BeginCascadePass(Context, CascadeIndex);
    
    if (ShadowShader)
    {
        ShadowShader->Bind(Context);
    }
    
    Context->VSSetConstantBuffers(0, 1, ShadowConstantBuffer.GetAddressOf());
}

void KCascadedShadowRenderer::RenderShadowCasters(ID3D11DeviceContext* Context)
{
    if (!Context || !ShadowShader || ShadowCasters.empty())
        return;

    for (const auto& caster : ShadowCasters)
    {
        if (!caster.Mesh)
            continue;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = Context->Map(caster.Mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            FConstantBuffer* cb = static_cast<FConstantBuffer*>(mapped.pData);
            cb->WorldMatrix = XMMatrixTranspose(caster.WorldMatrix);
            cb->ViewMatrix = XMMatrixIdentity();
            cb->ProjectionMatrix = XMMatrixIdentity();
            Context->Unmap(caster.Mesh->GetConstantBuffer(), 0);
        }

        caster.Mesh->Render(Context);
    }
}

void KCascadedShadowRenderer::EndCascadePass(ID3D11DeviceContext* Context)
{
    CascadedShadowMap.EndCascadePass(Context);
}

void KCascadedShadowRenderer::EndShadowPass(ID3D11DeviceContext* Context)
{
    if (!Context)
        return;

    CascadedShadowMap.CopyCascadesToArray(Context);
    UpdateCascadeBuffer(Context);
}

void KCascadedShadowRenderer::UpdateCascadeBuffer(ID3D11DeviceContext* Context)
{
    if (!Context || !ShadowConstantBuffer)
        return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Context->Map(ShadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
        return;

    FCascadedShadowBuffer* bufferData = static_cast<FCascadedShadowBuffer*>(mappedResource.pData);
    ZeroMemory(bufferData, sizeof(FCascadedShadowBuffer));
    
    for (UINT32 i = 0; i < CascadedShadowMap.GetCascadeCount(); ++i)
    {
        bufferData->CascadeData.LightViewProjection[i] = XMMatrixTranspose(CascadeSplits[i].LightViewProjection);
        bufferData->CascadeData.SplitDistances[i] = XMFLOAT4(
            CascadeSplits[i].NearDistance, 
            CascadeSplits[i].FarDistance,
            0.0f, 0.0f
        );
    }
    
    bufferData->ShadowMapSize = XMFLOAT4(
        static_cast<float>(CascadedShadowMap.GetCascadeResolution()),
        static_cast<float>(CascadedShadowMap.GetCascadeResolution()),
        0.0f, 0.0f
    );
    bufferData->DepthBias = Config.DepthBias;
    bufferData->PCFKernelSize = Config.PCFKernelSize;
    bufferData->CascadeCount = CascadedShadowMap.GetCascadeCount();

    Context->Unmap(ShadowConstantBuffer.Get(), 0);
}

void KCascadedShadowRenderer::AddShadowCaster(const FShadowCaster& Caster)
{
    ShadowCasters.push_back(Caster);
}

void KCascadedShadowRenderer::ClearShadowCasters()
{
    ShadowCasters.clear();
}

void KCascadedShadowRenderer::BindShadowMaps(ID3D11DeviceContext* Context, UINT32 Slot)
{
    if (!Context)
        return;

    ID3D11ShaderResourceView* srv = CascadedShadowMap.GetCascadeArraySRV();
    Context->PSSetShaderResources(Slot, 1, &srv);
    
    Context->PSSetConstantBuffers(2, 1, ShadowConstantBuffer.GetAddressOf());
}

void KCascadedShadowRenderer::UnbindShadowMaps(ID3D11DeviceContext* Context, UINT32 Slot)
{
    if (!Context)
        return;

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(Slot, 1, &nullSRV);
}

XMMATRIX KCascadedShadowRenderer::CalculateLightViewProjection(const FDirectionalLight& Light,
                                                                const XMFLOAT3& MinBounds,
                                                                const XMFLOAT3& MaxBounds)
{
    XMVECTOR lightDir = XMLoadFloat3(&Light.Direction);
    lightDir = XMVector3Normalize(lightDir);
    
    XMVECTOR center = XMVectorSet(
        (MinBounds.x + MaxBounds.x) * 0.5f,
        (MinBounds.y + MaxBounds.y) * 0.5f,
        (MinBounds.z + MaxBounds.z) * 0.5f,
        0.0f
    );
    
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetX(XMVector3Dot(lightDir, up))) > 0.99f)
    {
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }
    
    XMMATRIX lightView = XMMatrixLookAtLH(center - lightDir * 100.0f, center, up);
    
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(
        MinBounds.x, MaxBounds.x,
        MinBounds.y, MaxBounds.y,
        MinBounds.z, MaxBounds.z
    );
    
    return lightView * lightProj;
}
