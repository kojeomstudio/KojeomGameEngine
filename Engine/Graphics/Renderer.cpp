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

    GraphicsDevice->BeginFrame(ClearColor);

    UpdateLightBuffer();
}

void KRenderer::EndFrame(bool bVSync)
{
    if (!GraphicsDevice || !bInFrame)
    {
        return;
    }

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

    FRenderObject RO(InMesh, LightShader, InTexture);
    RO.WorldMatrix = WorldMatrix;
    KRenderer::RenderObject(RO);
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

    LightConstantBuffer.Reset();
    WireframeRasterizerState.Reset();
    DebugSphereMesh.reset();
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

    LOG_INFO("Default resources initialized successfully");
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
