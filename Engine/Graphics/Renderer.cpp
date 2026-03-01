#include "Renderer.h"

HRESULT KRenderer::Initialize(KGraphicsDevice* InGraphicsDevice)
{
    if (!InGraphicsDevice)
    {
        LOG_ERROR("Invalid graphics device");
        return E_INVALIDARG;
    }

    GraphicsDevice = InGraphicsDevice;

    LOG_INFO("Initializing Renderer...");

    HRESULT hr = InitializeDefaultResources();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize default resources");
        return hr;
    }

    LOG_INFO("Renderer initialized successfully");
    return S_OK;
}

void KRenderer::BeginFrame(KCamera* InCamera, const float ClearColor[4])
{
    if (!GraphicsDevice || !InCamera)
    {
        return;
    }

    CurrentCamera = InCamera;
    bInFrame = true;

    CurrentCamera->UpdateMatrices();

    XMMATRIX viewProjection = CurrentCamera->GetViewMatrix() * CurrentCamera->GetProjectionMatrix();
    Frustum.UpdateFromMatrix(viewProjection);

    GPUTimer.BeginFrame(GraphicsDevice->GetContext());

    if (bOcclusionCullingEnabled && OcclusionCuller.IsInitialized())
    {
        OcclusionCuller.BeginFrame(GraphicsDevice->GetContext());
    }

    if (bShadowsEnabled && ShadowRenderer.IsInitialized())
    {
        ShadowRenderer.BeginShadowPass(
            GraphicsDevice->GetContext(),
            DirectionalLight,
            ShadowSceneCenter,
            ShadowSceneRadius
        );
        ShadowRenderer.ClearShadowCasters();
    }

    GraphicsDevice->BeginFrame(ClearColor);

    UpdateLightBuffer();
    
    if (bShadowsEnabled)
    {
        UpdateShadowBuffer();
    }
}

void KRenderer::EndFrame(bool bVSync)
{
    if (!GraphicsDevice || !bInFrame)
    {
        return;
    }

    if (bOcclusionCullingEnabled && OcclusionCuller.IsInitialized())
    {
        OcclusionCuller.EndFrame(GraphicsDevice->GetContext());
    }

    GPUTimer.EndFrame(GraphicsDevice->GetContext());

    GraphicsDevice->EndFrame(bVSync);

    bInFrame = false;
    CurrentCamera = nullptr;
}

void KRenderer::RenderObject(const FRenderObject& RenderObject)
{
    if (!GraphicsDevice || !CurrentCamera || !bInFrame)
    {
        return;
    }

    if (!RenderObject.Mesh || !RenderObject.Shader)
    {
        return;
    }

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    RenderObject.Shader->Bind(Context);

    if (RenderObject.Texture)
    {
        RenderObject.Texture->Bind(Context, 0);
    }

    RenderObject.Mesh->UpdateConstantBuffer(
        Context,
        RenderObject.WorldMatrix,
        CurrentCamera->GetViewMatrix(),
        CurrentCamera->GetProjectionMatrix()
    );

    RenderObject.Mesh->Render(Context);

    if (RenderObject.Texture)
    {
        RenderObject.Texture->Unbind(Context, 0);
    }

    RenderObject.Shader->Unbind(Context);
}

void KRenderer::RenderMesh(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix, 
                          std::shared_ptr<KTexture> InTexture)
{
    if (!BasicShader)
    {
        return;
    }

    FRenderObject RenderObject(InMesh, BasicShader, InTexture);
    RenderObject.WorldMatrix = WorldMatrix;
    KRenderer::RenderObject(RenderObject);
}

void KRenderer::RenderMeshBasic(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix)
{
    RenderMesh(InMesh, WorldMatrix, nullptr);
}

void KRenderer::RenderMeshLit(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                               std::shared_ptr<KTexture> InTexture)
{
    if (!LightShader)
    {
        RenderMesh(InMesh, WorldMatrix, InTexture);
        return;
    }

    std::shared_ptr<KShaderProgram> ShaderToUse = LightShader;
    
    if (bShadowsEnabled && ShadowLitShader)
    {
        ShaderToUse = ShadowLitShader;
        
        if (ShadowRenderer.IsInitialized())
        {
            FShadowCaster Caster;
            Caster.Mesh = InMesh;
            Caster.WorldMatrix = WorldMatrix;
            ShadowRenderer.AddShadowCaster(Caster);
        }
    }

    FRenderObject RO(InMesh, ShaderToUse, InTexture);
    RO.WorldMatrix = WorldMatrix;
    KRenderer::RenderObject(RO);
}

void KRenderer::RenderMeshPBR(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                               KMaterial* Material)
{
    if (!GraphicsDevice || !CurrentCamera || !bInFrame)
    {
        return;
    }

    if (!InMesh || !PBRShader)
    {
        RenderMeshLit(InMesh, WorldMatrix, nullptr);
        return;
    }

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    if (ShadowRenderer.IsInitialized() && bShadowsEnabled)
    {
        FShadowCaster Caster;
        Caster.Mesh = InMesh;
        Caster.WorldMatrix = WorldMatrix;
        ShadowRenderer.AddShadowCaster(Caster);
    }

    PBRShader->Bind(Context);

    InMesh->UpdateConstantBuffer(
        Context,
        WorldMatrix,
        CurrentCamera->GetViewMatrix(),
        CurrentCamera->GetProjectionMatrix()
    );

    Context->PSSetConstantBuffers(1, 1, LightConstantBuffer.GetAddressOf());

    if (bShadowsEnabled)
    {
        Context->PSSetConstantBuffers(2, 1, ShadowConstantBuffer.GetAddressOf());
        if (ShadowSamplerState)
        {
            Context->PSSetSamplers(0, 1, ShadowSamplerState.GetAddressOf());
        }
        ShadowRenderer.BindShadowMap(Context, 3);
    }

    if (MaterialSamplerState)
    {
        Context->PSSetSamplers(1, 1, MaterialSamplerState.GetAddressOf());
    }

    if (Material)
    {
        Material->UpdateConstantBuffer(Context);
        Context->PSSetConstantBuffers(4, 1, Material->GetConstantBufferAddress());
        Material->BindTextures(Context);
    }
    else
    {
        FPBRMaterialParams DefaultParams;
        ID3D11Buffer* pDefaultBuffer = nullptr;
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(FPBRMaterialParams);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &DefaultParams;
        
        GraphicsDevice->GetDevice()->CreateBuffer(&desc, &initData, &pDefaultBuffer);
        if (pDefaultBuffer)
        {
            Context->PSSetConstantBuffers(4, 1, &pDefaultBuffer);
            pDefaultBuffer->Release();
        }
    }

    InMesh->Render(Context);

    ID3D11ShaderResourceView* nullSRVs[7] = {};
    Context->PSSetShaderResources(0, 7, nullSRVs);

    PBRShader->Unbind(Context);
}

void KRenderer::AddPointLight(const FPointLight& Light)
{
    if (PointLights.size() < MAX_POINT_LIGHTS)
    {
        PointLights.push_back(Light);
    }
    else
    {
        LOG_WARNING("Maximum point lights reached");
    }
}

void KRenderer::RemovePointLight(UINT32 Index)
{
    if (Index < PointLights.size())
    {
        PointLights.erase(PointLights.begin() + Index);
    }
}

void KRenderer::ClearPointLights()
{
    PointLights.clear();
}

void KRenderer::SetPointLight(UINT32 Index, const FPointLight& Light)
{
    if (Index < PointLights.size())
    {
        PointLights[Index] = Light;
    }
}

void KRenderer::AddSpotLight(const FSpotLight& Light)
{
    if (SpotLights.size() < MAX_SPOT_LIGHTS)
    {
        SpotLights.push_back(Light);
    }
    else
    {
        LOG_WARNING("Maximum spot lights reached");
    }
}

void KRenderer::RemoveSpotLight(UINT32 Index)
{
    if (Index < SpotLights.size())
    {
        SpotLights.erase(SpotLights.begin() + Index);
    }
}

void KRenderer::ClearSpotLights()
{
    SpotLights.clear();
}

void KRenderer::SetSpotLight(UINT32 Index, const FSpotLight& Light)
{
    if (Index < SpotLights.size())
    {
        SpotLights[Index] = Light;
    }
}

void KRenderer::ClearAllLights()
{
    ClearPointLights();
    ClearSpotLights();
}

void KRenderer::Cleanup()
{
    LOG_INFO("Cleaning up Renderer...");

    OcclusionCuller.Cleanup();
    GPUTimer.Cleanup();
    InstancedRenderer.Cleanup();
    PostProcessor.Cleanup();
    DeferredRenderer.Cleanup();
    ShadowRenderer.Cleanup();
    ShadowConstantBuffer.Reset();
    ShadowSamplerState.Reset();
    MaterialSamplerState.Reset();
    LightConstantBuffer.Reset();
    WireframeRasterizerState.Reset();
    DebugSphereMesh.reset();
    PBRShader.reset();
    ShadowLitShader.reset();
    LightShader.reset();
    BasicShader.reset();
    TextureManager.Cleanup();

    ClearAllLights();

    GraphicsDevice = nullptr;
    CurrentCamera = nullptr;
    bInFrame = false;

    LOG_INFO("Renderer cleanup completed");
}

std::shared_ptr<KMesh> KRenderer::CreateTriangleMesh()
{
    if (!GraphicsDevice)
    {
        return nullptr;
    }

    return KMesh::CreateTriangle(GraphicsDevice->GetDevice());
}

std::shared_ptr<KMesh> KRenderer::CreateQuadMesh()
{
    if (!GraphicsDevice)
    {
        return nullptr;
    }

    return KMesh::CreateQuad(GraphicsDevice->GetDevice());
}

std::shared_ptr<KMesh> KRenderer::CreateCubeMesh()
{
    if (!GraphicsDevice)
    {
        return nullptr;
    }

    return KMesh::CreateCube(GraphicsDevice->GetDevice());
}

std::shared_ptr<KMesh> KRenderer::CreateSphereMesh(UINT32 Slices, UINT32 Stacks)
{
    if (!GraphicsDevice)
    {
        return nullptr;
    }

    return KMesh::CreateSphere(GraphicsDevice->GetDevice(), Slices, Stacks);
}

void KRenderer::UpdateLightBuffer()
{
    if (!LightConstantBuffer || !CurrentCamera)
    {
        return;
    }

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    FMultipleLightBuffer LightData = {};
    LightData.CameraPosition = CurrentCamera->GetPosition();
    LightData.NumPointLights = static_cast<int32>(PointLights.size() > MAX_POINT_LIGHTS ? MAX_POINT_LIGHTS : PointLights.size());
    LightData.DirLightDirection = DirectionalLight.Direction;
    LightData.NumSpotLights = static_cast<int32>(SpotLights.size() > MAX_SPOT_LIGHTS ? MAX_SPOT_LIGHTS : SpotLights.size());
    LightData.DirLightColor = DirectionalLight.Color;
    LightData.AmbientColor = DirectionalLight.AmbientColor;

    for (size_t i = 0; i < LightData.NumPointLights; ++i)
    {
        LightData.PointLights[i].Position  = PointLights[i].Position;
        LightData.PointLights[i].Intensity = PointLights[i].Intensity;
        LightData.PointLights[i].Color     = PointLights[i].Color;
        LightData.PointLights[i].Radius    = PointLights[i].Radius;
        LightData.PointLights[i].Falloff   = PointLights[i].Falloff;
    }

    for (size_t i = 0; i < LightData.NumSpotLights; ++i)
    {
        LightData.SpotLights[i].Position   = SpotLights[i].Position;
        LightData.SpotLights[i].Intensity  = SpotLights[i].Intensity;
        LightData.SpotLights[i].Direction  = SpotLights[i].Direction;
        LightData.SpotLights[i].InnerCone  = SpotLights[i].InnerCone;
        LightData.SpotLights[i].Color      = SpotLights[i].Color;
        LightData.SpotLights[i].OuterCone  = SpotLights[i].OuterCone;
        LightData.SpotLights[i].Radius     = SpotLights[i].Radius;
        LightData.SpotLights[i].Falloff    = SpotLights[i].Falloff;
    }

    Context->UpdateSubresource(LightConstantBuffer.Get(), 0, nullptr, &LightData, 0, 0);
    Context->PSSetConstantBuffers(1, 1, LightConstantBuffer.GetAddressOf());
}

HRESULT KRenderer::InitializeDefaultResources()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();

    BasicShader = std::make_shared<KShaderProgram>();
    HRESULT hr = BasicShader->CreateBasicColorShader(Device);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Basic shader program creation failed");
        return hr;
    }

    LightShader = std::make_shared<KShaderProgram>();
    hr = LightShader->CreatePhongShader(Device);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Phong shader program creation failed");
        return hr;
    }

    ShadowLitShader = std::make_shared<KShaderProgram>();
    hr = ShadowLitShader->CreatePhongShadowShader(Device);
    if (FAILED(hr))
    {
        LOG_WARNING("Shadow shader creation failed, shadows will be disabled");
        ShadowLitShader.reset();
    }

    PBRShader = std::make_shared<KShaderProgram>();
    hr = PBRShader->CreatePBRShader(Device);
    if (FAILED(hr))
    {
        LOG_WARNING("PBR shader creation failed, PBR rendering will be disabled");
        PBRShader.reset();
    }

    D3D11_SAMPLER_DESC MaterialSamplerDesc = {};
    MaterialSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    MaterialSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    MaterialSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    MaterialSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    MaterialSamplerDesc.MinLOD = 0;
    MaterialSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = Device->CreateSamplerState(&MaterialSamplerDesc, &MaterialSamplerState);
    if (FAILED(hr))
    {
        LOG_WARNING("Material sampler state creation failed");
    }

    D3D11_BUFFER_DESC LightCBDesc = {};
    LightCBDesc.Usage          = D3D11_USAGE_DEFAULT;
    LightCBDesc.ByteWidth      = sizeof(FMultipleLightBuffer);
    LightCBDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    LightCBDesc.CPUAccessFlags = 0;

    hr = Device->CreateBuffer(&LightCBDesc, nullptr, &LightConstantBuffer);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Light constant buffer creation failed");
        return hr;
    }

    D3D11_BUFFER_DESC ShadowCBDesc = {};
    ShadowCBDesc.Usage = D3D11_USAGE_DYNAMIC;
    ShadowCBDesc.ByteWidth = sizeof(FShadowDataBuffer);
    ShadowCBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ShadowCBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = Device->CreateBuffer(&ShadowCBDesc, nullptr, &ShadowConstantBuffer);
    if (FAILED(hr))
    {
        LOG_WARNING("Shadow constant buffer creation failed");
    }

    D3D11_SAMPLER_DESC ShadowSamplerDesc = {};
    ShadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    ShadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    ShadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    ShadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    ShadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    ShadowSamplerDesc.BorderColor[0] = 1.0f;
    ShadowSamplerDesc.BorderColor[1] = 1.0f;
    ShadowSamplerDesc.BorderColor[2] = 1.0f;
    ShadowSamplerDesc.BorderColor[3] = 1.0f;
    ShadowSamplerDesc.MinLOD = 0;
    ShadowSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = Device->CreateSamplerState(&ShadowSamplerDesc, &ShadowSamplerState);
    if (FAILED(hr))
    {
        LOG_WARNING("Shadow sampler state creation failed");
    }

    hr = TextureManager.CreateDefaultTextures(Device);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Default texture creation failed");
        return hr;
    }

    hr = CreateDebugResources();
    if (FAILED(hr))
    {
        LOG_WARNING("Debug resources creation failed, debug drawing will be disabled");
    }

    hr = InitializeShadowSystem();
    if (FAILED(hr))
    {
        LOG_WARNING("Shadow system initialization failed, shadows will be disabled");
    }

    hr = DeferredRenderer.Initialize(GraphicsDevice->GetDevice(), GraphicsDevice->GetWidth(), GraphicsDevice->GetHeight());
    if (FAILED(hr))
    {
        LOG_WARNING("Deferred renderer initialization failed, deferred rendering will be disabled");
    }
    else
    {
        DeferredRenderer.SetDirectionalLight(DirectionalLight);
        LOG_INFO("Deferred renderer initialized successfully");
    }

    hr = PostProcessor.Initialize(GraphicsDevice->GetDevice(), GraphicsDevice->GetWidth(), GraphicsDevice->GetHeight());
    if (FAILED(hr))
    {
        LOG_WARNING("PostProcessor initialization failed, post-processing will be disabled");
    }
    else
    {
        bPostProcessEnabled = true;
        LOG_INFO("PostProcessor initialized successfully");
    }

    hr = InstancedRenderer.Initialize(Device, 1024);
    if (FAILED(hr))
    {
        LOG_WARNING("Instanced renderer initialization failed");
    }
    else
    {
        LOG_INFO("Instanced renderer initialized successfully");
    }

    hr = GPUTimer.Initialize(Device, 16);
    if (FAILED(hr))
    {
        LOG_WARNING("GPU timer initialization failed");
    }
    else
    {
        GPUTimer.CreateTimer("Frame");
        GPUTimer.CreateTimer("ShadowPass");
        GPUTimer.CreateTimer("GeometryPass");
        GPUTimer.CreateTimer("LightingPass");
        LOG_INFO("GPU timer initialized successfully");
    }

    hr = OcclusionCuller.Initialize(Device);
    if (FAILED(hr))
    {
        LOG_WARNING("Occlusion culler initialization failed");
    }
    else
    {
        bOcclusionCullingEnabled = false;
        LOG_INFO("Occlusion culler initialized successfully");
    }

    LOG_INFO("Default resources initialized successfully");
    return S_OK;
}

HRESULT KRenderer::InitializeShadowSystem()
{
    FShadowConfig ShadowConfig;
    ShadowConfig.Resolution = 2048;
    ShadowConfig.DepthBias = 0.0001f;
    ShadowConfig.PCFKernelSize = 3;

    HRESULT hr = ShadowRenderer.Initialize(GraphicsDevice->GetDevice(), ShadowConfig);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to initialize shadow renderer");
        return hr;
    }

    bShadowsEnabled = true;
    LOG_INFO("Shadow system initialized successfully");
    return S_OK;
}

HRESULT KRenderer::CreateDebugResources()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();

    D3D11_RASTERIZER_DESC WireframeDesc = {};
    WireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    WireframeDesc.CullMode = D3D11_CULL_NONE;
    WireframeDesc.FrontCounterClockwise = FALSE;
    WireframeDesc.DepthClipEnable = TRUE;

    HRESULT hr = Device->CreateRasterizerState(&WireframeDesc, &WireframeRasterizerState);
    if (FAILED(hr))
    {
        KLogger::HResultError(hr, "Wireframe rasterizer state creation failed");
        return hr;
    }

    DebugSphereMesh = KMesh::CreateSphere(Device, 16, 16);
    if (!DebugSphereMesh)
    {
        LOG_ERROR("Debug sphere mesh creation failed");
        return E_FAIL;
    }

    LOG_INFO("Debug resources created successfully");
    return S_OK;
}

void KRenderer::DebugDrawLightVolumes()
{
    if (!bDebugDrawEnabled || !GraphicsDevice || !CurrentCamera || !BasicShader)
    {
        return;
    }

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();
    ComPtr<ID3D11RasterizerState> OldRasterizerState;
    Context->RSGetState(&OldRasterizerState);

    if (WireframeRasterizerState)
    {
        Context->RSSetState(WireframeRasterizerState.Get());
    }

    if (bDebugDrawPointLightVolumes && DebugSphereMesh)
    {
        for (const auto& PointLight : PointLights)
        {
            XMMATRIX WorldMatrix = XMMatrixScaling(PointLight.Radius, PointLight.Radius, PointLight.Radius) *
                                   XMMatrixTranslation(PointLight.Position.x, PointLight.Position.y, PointLight.Position.z);
            
            DebugSphereMesh->UpdateConstantBuffer(
                Context,
                WorldMatrix,
                CurrentCamera->GetViewMatrix(),
                CurrentCamera->GetProjectionMatrix()
            );

            BasicShader->Bind(Context);
            DebugSphereMesh->Render(Context);
            BasicShader->Unbind(Context);
        }
    }

    if (bDebugDrawSpotLightVolumes && DebugSphereMesh)
    {
        for (const auto& SpotLight : SpotLights)
        {
            XMMATRIX WorldMatrix = XMMatrixScaling(SpotLight.Radius, SpotLight.Radius, SpotLight.Radius) *
                                   XMMatrixTranslation(SpotLight.Position.x, SpotLight.Position.y, SpotLight.Position.z);
            
            DebugSphereMesh->UpdateConstantBuffer(
                Context,
                WorldMatrix,
                CurrentCamera->GetViewMatrix(),
                CurrentCamera->GetProjectionMatrix()
            );

            BasicShader->Bind(Context);
            DebugSphereMesh->Render(Context);
            BasicShader->Unbind(Context);
        }
    }

    Context->RSSetState(OldRasterizerState.Get());
}

void KRenderer::SetShadowEnabled(bool bEnabled)
{
    if (bEnabled && !ShadowRenderer.IsInitialized())
    {
        LOG_WARNING("Cannot enable shadows: shadow system not initialized");
        return;
    }
    bShadowsEnabled = bEnabled;
}

void KRenderer::SetShadowSceneBounds(const XMFLOAT3& Center, float Radius)
{
    ShadowSceneCenter = Center;
    ShadowSceneRadius = Radius;
}

void KRenderer::UpdateShadowBuffer()
{
    if (!ShadowConstantBuffer || !bShadowsEnabled)
        return;

    ID3D11DeviceContext* Context = GraphicsDevice->GetContext();

    D3D11_MAPPED_SUBRESOURCE Mapped;
    HRESULT hr = Context->Map(ShadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    if (SUCCEEDED(hr))
    {
        FShadowDataBuffer* Buffer = static_cast<FShadowDataBuffer*>(Mapped.pData);
        Buffer->LightViewProjection = XMMatrixTranspose(ShadowRenderer.GetLightViewProjection());
        Buffer->ShadowMapSize = XMFLOAT2(
            static_cast<float>(ShadowRenderer.GetShadowMap()->GetResolution()),
            static_cast<float>(ShadowRenderer.GetShadowMap()->GetResolution())
        );
        Buffer->DepthBias = ShadowRenderer.GetShadowMap()->GetConfig().DepthBias;
        Buffer->PCFKernelSize = ShadowRenderer.GetShadowMap()->GetConfig().PCFKernelSize;
        Buffer->bShadowEnabled = bShadowsEnabled ? 1 : 0;
        Context->Unmap(ShadowConstantBuffer.Get(), 0);
    }

    Context->PSSetConstantBuffers(2, 1, ShadowConstantBuffer.GetAddressOf());

    if (ShadowSamplerState)
    {
        Context->PSSetSamplers(0, 1, ShadowSamplerState.GetAddressOf());
    }

    ShadowRenderer.BindShadowMap(Context, 3);
}

void KRenderer::SetRenderPath(ERenderPath Path)
{
    if (Path == ERenderPath::Deferred && !DeferredRenderer.IsInitialized())
    {
        LOG_WARNING("Cannot switch to deferred rendering: deferred renderer not initialized");
        return;
    }
    
    CurrentRenderPath = Path;
    
    if (Path == ERenderPath::Deferred)
    {
        DeferredRenderer.SetDirectionalLight(DirectionalLight);
        for (const auto& Light : PointLights)
        {
            DeferredRenderer.AddPointLight(Light);
        }
        for (const auto& Light : SpotLights)
        {
            DeferredRenderer.AddSpotLight(Light);
        }
    }
    
    LOG_INFO("Render path changed");
}

void KRenderer::BeginHDRPass()
{
    if (!PostProcessor.IsInitialized() || !bPostProcessEnabled)
    {
        return;
    }

    PostProcessor.BeginHDRPass(GraphicsDevice->GetContext());
}

void KRenderer::EndHDRPass()
{
    if (!PostProcessor.IsInitialized() || !bPostProcessEnabled)
    {
        return;
    }

    PostProcessor.ApplyPostProcessing(GraphicsDevice->GetContext(), GraphicsDevice->GetRenderTargetView());
}

bool KRenderer::IsVisibleInFrustum(const FBoundingSphere& Sphere) const
{
    if (!bFrustumCullingEnabled)
    {
        return true;
    }
    return Frustum.ContainsSphere(Sphere);
}

bool KRenderer::IsVisibleInFrustum(const FBoundingBox& Box) const
{
    if (!bFrustumCullingEnabled)
    {
        return true;
    }
    return Frustum.ContainsBox(Box);
}

void KRenderer::BeginGPUTimer(const std::string& Name)
{
    if (GPUTimer.IsInitialized() && GPUTimer.HasTimer(Name))
    {
        GPUTimer.StartTimer(GraphicsDevice->GetContext(), Name);
    }
}

void KRenderer::EndGPUTimer(const std::string& Name)
{
    if (GPUTimer.IsInitialized() && GPUTimer.HasTimer(Name))
    {
        GPUTimer.EndTimer(GraphicsDevice->GetContext(), Name);
    }
}

void KRenderer::SetOcclusionCullingEnabled(bool bEnabled)
{
    if (bEnabled && !OcclusionCuller.IsInitialized())
    {
        LOG_WARNING("Cannot enable occlusion culling: occlusion culler not initialized");
        return;
    }
    bOcclusionCullingEnabled = bEnabled;
    OcclusionCuller.SetEnabled(bEnabled);
}

bool KRenderer::IsVisibleWithOcclusion(ID3D11DeviceContext* Context, const std::string& QueryName)
{
    if (!bOcclusionCullingEnabled || !OcclusionCuller.IsInitialized())
    {
        return true;
    }
    return OcclusionCuller.IsVisible(Context, QueryName);
} 
