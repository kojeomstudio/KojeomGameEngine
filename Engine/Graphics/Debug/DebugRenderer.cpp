#include "DebugRenderer.h"

namespace
{
    const char* DebugVertexShaderSource = R"(
cbuffer DebugConstantBuffer : register(b0)
{
    matrix WorldViewProjection;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.Color = input.Color;
    return output;
}
)";

    const char* DebugPixelShaderSource = R"(
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.Color;
}
)";
}

HRESULT KDebugRenderer::Initialize(KGraphicsDevice* InGraphicsDevice)
{
    if (!InGraphicsDevice)
    {
        LOG_ERROR("Invalid graphics device");
        return E_INVALIDARG;
    }

    GraphicsDevice = InGraphicsDevice;
    ID3D11Device* Device = GraphicsDevice->GetDevice();

    HRESULT hr = CreateShaders();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create debug shaders");
        return hr;
    }

    hr = CreateBuffers();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create debug buffers");
        return hr;
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthDesc.StencilEnable = FALSE;
    
    hr = Device->CreateDepthStencilState(&depthDesc, &DepthStencilState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create depth stencil state");
        return hr;
    }

    D3D11_DEPTH_STENCIL_DESC depthDisabledDesc = {};
    depthDisabledDesc.DepthEnable = FALSE;
    depthDisabledDesc.StencilEnable = FALSE;
    
    hr = Device->CreateDepthStencilState(&depthDisabledDesc, &DepthDisabledState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create disabled depth stencil state");
        return hr;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = Device->CreateBlendState(&blendDesc, &BlendState);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create blend state");
        return hr;
    }

    FrameVertices.reserve(MaxVertices);
    bInitialized = true;
    LOG_INFO("DebugRenderer initialized successfully");
    return S_OK;
}

HRESULT KDebugRenderer::CreateShaders()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();
    
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompile(DebugVertexShaderSource, strlen(DebugVertexShaderSource),
                           nullptr, nullptr, nullptr, "VSMain", "vs_5_0", flags, 0,
                           &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string err = "VS compile error: " + std::string((char*)errorBlob->GetBufferPointer());
            LOG_ERROR(err.c_str());
        }
        return hr;
    }

    hr = D3DCompile(DebugPixelShaderSource, strlen(DebugPixelShaderSource),
                   nullptr, nullptr, nullptr, "PSMain", "ps_5_0", flags, 0,
                   &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string err = "PS compile error: " + std::string((char*)errorBlob->GetBufferPointer());
            LOG_ERROR(err.c_str());
        }
        return hr;
    }

    hr = Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                    nullptr, &VertexShader);
    if (FAILED(hr)) return hr;

    hr = Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                   nullptr, &PixelShader);
    if (FAILED(hr)) return hr;

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = Device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(),
                                   vsBlob->GetBufferSize(), &InputLayout);
    return hr;
}

HRESULT KDebugRenderer::CreateBuffers()
{
    ID3D11Device* Device = GraphicsDevice->GetDevice();

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = MaxVertices * sizeof(FDebugVertex);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&vbDesc, nullptr, &VertexBuffer);
    if (FAILED(hr)) return hr;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(XMMATRIX);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = Device->CreateBuffer(&cbDesc, nullptr, &ConstantBuffer);
    return hr;
}

void KDebugRenderer::BeginFrame()
{
    FrameVertices.clear();
}

void KDebugRenderer::Render(ID3D11DeviceContext* Context, KCamera* Camera)
{
    if (!bInitialized || !bEnabled || FrameVertices.empty() || !Camera)
        return;

    XMMATRIX view = Camera->GetViewMatrix();
    XMMATRIX proj = Camera->GetProjectionMatrix();
    XMMATRIX wvp = XMMatrixTranspose(view * proj);

    Context->UpdateSubresource(ConstantBuffer.Get(), 0, nullptr, &wvp, 0, 0);

    UpdateDynamicBuffer(Context);

    Context->IASetInputLayout(InputLayout.Get());
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    
    UINT stride = sizeof(FDebugVertex);
    UINT offset = 0;
    Context->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &stride, &offset);
    
    Context->VSSetShader(VertexShader.Get(), nullptr, 0);
    Context->VSSetConstantBuffers(0, 1, ConstantBuffer.GetAddressOf());
    Context->PSSetShader(PixelShader.Get(), nullptr, 0);

    Context->OMSetDepthStencilState(DepthStencilState.Get(), 0);
    
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    Context->OMSetBlendState(BlendState.Get(), blendFactor, 0xFFFFFFFF);

    Context->Draw(static_cast<UINT>(FrameVertices.size()), 0);

    ID3D11DepthStencilState* nullDS = nullptr;
    Context->OMSetDepthStencilState(nullDS, 0);
    ID3D11BlendState* nullBlend = nullptr;
    Context->OMSetBlendState(nullBlend, blendFactor, 0xFFFFFFFF);
}

void KDebugRenderer::EndFrame(float DeltaTime)
{
    auto UpdateLifetime = [DeltaTime](float& remaining) {
        if (remaining > 0.0f)
        {
            remaining -= DeltaTime;
            return remaining > 0.0f;
        }
        return true;
    };

    std::deque<FDebugLine> remainingLines;
    for (const auto& line : Lines)
    {
        if (line.bPersistent || UpdateLifetime(const_cast<float&>(line.RemainingLifeTime)))
        {
            remainingLines.push_back(line);
        }
    }
    Lines = std::move(remainingLines);

    std::deque<FDebugSphere> remainingSpheres;
    for (const auto& sphere : Spheres)
    {
        float remaining = sphere.RemainingLifeTime;
        if (UpdateLifetime(remaining))
        {
            FDebugSphere s = sphere;
            s.RemainingLifeTime = remaining;
            remainingSpheres.push_back(s);
        }
    }
    Spheres = std::move(remainingSpheres);

    std::deque<FDebugBox> remainingBoxes;
    for (const auto& box : Boxes)
    {
        float remaining = box.RemainingLifeTime;
        if (UpdateLifetime(remaining))
        {
            FDebugBox b = box;
            b.RemainingLifeTime = remaining;
            remainingBoxes.push_back(b);
        }
    }
    Boxes = std::move(remainingBoxes);

    std::deque<FDebugCapsule> remainingCapsules;
    for (const auto& capsule : Capsules)
    {
        float remaining = capsule.RemainingLifeTime;
        if (UpdateLifetime(remaining))
        {
            FDebugCapsule c = capsule;
            c.RemainingLifeTime = remaining;
            remainingCapsules.push_back(c);
        }
    }
    Capsules = std::move(remainingCapsules);

    std::deque<FDebugArrow> remainingArrows;
    for (const auto& arrow : Arrows)
    {
        float remaining = arrow.RemainingLifeTime;
        if (UpdateLifetime(remaining))
        {
            FDebugArrow a = arrow;
            a.RemainingLifeTime = remaining;
            remainingArrows.push_back(a);
        }
    }
    Arrows = std::move(remainingArrows);
}

void KDebugRenderer::UpdateDynamicBuffer(ID3D11DeviceContext* Context)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, FrameVertices.data(), FrameVertices.size() * sizeof(FDebugVertex));
        Context->Unmap(VertexBuffer.Get(), 0);
    }
}

void KDebugRenderer::AddLineVertices(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color)
{
    if (FrameVertices.size() + 2 > MaxVertices) return;
    
    FrameVertices.push_back({ Start, Color });
    FrameVertices.push_back({ End, Color });
}

void KDebugRenderer::DrawLine(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color,
                               float LifeTime, bool bPersistent)
{
    if (LifeTime > 0.0f)
    {
        FDebugLine line;
        line.Start = Start;
        line.End = End;
        line.Color = Color;
        line.LifeTime = LifeTime;
        line.RemainingLifeTime = LifeTime;
        line.bPersistent = bPersistent;
        Lines.push_back(line);
    }
    else
    {
        AddLineVertices(Start, End, Color);
    }
}

void KDebugRenderer::DrawRay(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float Length,
                              const XMFLOAT4& Color, float LifeTime)
{
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&Direction));
    XMFLOAT3 end;
    XMStoreFloat3(&end, XMLoadFloat3(&Origin) + dir * Length);
    DrawLine(Origin, end, Color, LifeTime);
}

void KDebugRenderer::DrawArrow(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color,
                                float HeadSize, float LifeTime)
{
    DrawLine(Start, End, Color, LifeTime);

    XMVECTOR startVec = XMLoadFloat3(&Start);
    XMVECTOR endVec = XMLoadFloat3(&End);
    XMVECTOR dir = XMVector3Normalize(endVec - startVec);
    
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR right = XMVector3Cross(up, dir);
    if (XMVectorGetX(XMVector3LengthSq(right)) < 0.001f)
    {
        up = XMVectorSet(1, 0, 0, 0);
        right = XMVector3Cross(up, dir);
    }
    up = XMVector3Cross(dir, right);

    XMVECTOR headBase = endVec - dir * HeadSize;
    float headWidth = HeadSize * 0.5f;
    
    for (int i = 0; i < 4; ++i)
    {
        float angle = i * XM_PIDIV2;
        XMVECTOR offset = XMVectorScale(right, cosf(angle) * headWidth) + 
                          XMVectorScale(up, sinf(angle) * headWidth);
        XMFLOAT3 headPoint;
        XMStoreFloat3(&headPoint, headBase + offset);
        DrawLine(headPoint, End, Color, LifeTime);
    }

    if (LifeTime > 0.0f)
    {
        FDebugArrow arrow;
        arrow.Start = Start;
        arrow.End = End;
        arrow.Color = Color;
        arrow.LifeTime = LifeTime;
        arrow.RemainingLifeTime = LifeTime;
        arrow.HeadSize = HeadSize;
        Arrows.push_back(arrow);
    }
}

void KDebugRenderer::DrawSphere(const XMFLOAT3& Center, float Radius, const XMFLOAT4& Color,
                                 int32 Segments, float LifeTime)
{
    if (LifeTime > 0.0f)
    {
        FDebugSphere sphere;
        sphere.Center = Center;
        sphere.Radius = Radius;
        sphere.Color = Color;
        sphere.LifeTime = LifeTime;
        sphere.RemainingLifeTime = LifeTime;
        sphere.Segments = Segments;
        Spheres.push_back(sphere);
    }
    else
    {
        GenerateSphereVertices(FrameVertices, Center, Radius, Color, Segments);
    }
}

void KDebugRenderer::GenerateSphereVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                                             float Radius, const XMFLOAT4& Color, int32 Segments)
{
    float step = XM_2PI / Segments;
    
    for (int32 i = 0; i < Segments; ++i)
    {
        float angle0 = i * step;
        float angle1 = (i + 1) * step;
        
        for (int32 j = 0; j <= Segments / 2; ++j)
        {
            float phi0 = j * step;
            float phi1 = (j + 1) * step;
            
            float y0 = cosf(phi0);
            float y1 = cosf(phi1);
            float r0 = sinf(phi0);
            float r1 = sinf(phi1);
            
            XMFLOAT3 p0 = { Center.x + r0 * cosf(angle0) * Radius,
                           Center.y + y0 * Radius,
                           Center.z + r0 * sinf(angle0) * Radius };
            XMFLOAT3 p1 = { Center.x + r0 * cosf(angle1) * Radius,
                           Center.y + y0 * Radius,
                           Center.z + r0 * sinf(angle1) * Radius };
            XMFLOAT3 p2 = { Center.x + r1 * cosf(angle0) * Radius,
                           Center.y + y1 * Radius,
                           Center.z + r1 * sinf(angle0) * Radius };
            
            if (Vertices.size() + 4 > MaxVertices) return;
            Vertices.push_back({ p0, Color });
            Vertices.push_back({ p1, Color });
            Vertices.push_back({ p0, Color });
            Vertices.push_back({ p2, Color });
        }
    }
}

void KDebugRenderer::DrawBox(const XMFLOAT3& Center, const XMFLOAT3& Extent, const XMFLOAT4& Color,
                              float LifeTime)
{
    DrawBox(Center, Extent, XMMatrixIdentity(), Color, LifeTime);
}

void KDebugRenderer::DrawBox(const XMFLOAT3& Center, const XMFLOAT3& Extent, const XMMATRIX& Rotation,
                              const XMFLOAT4& Color, float LifeTime)
{
    if (LifeTime > 0.0f)
    {
        FDebugBox box;
        box.Center = Center;
        box.Extent = Extent;
        box.Rotation = Rotation;
        box.Color = Color;
        box.LifeTime = LifeTime;
        box.RemainingLifeTime = LifeTime;
        Boxes.push_back(box);
    }
    else
    {
        GenerateBoxVertices(FrameVertices, Center, Extent, Rotation, Color);
    }
}

void KDebugRenderer::DrawBox(const FBoundingBox& Box, const XMFLOAT4& Color, float LifeTime)
{
    XMFLOAT3 center = {
        (Box.Min.x + Box.Max.x) * 0.5f,
        (Box.Min.y + Box.Max.y) * 0.5f,
        (Box.Min.z + Box.Max.z) * 0.5f
    };
    XMFLOAT3 extent = {
        (Box.Max.x - Box.Min.x) * 0.5f,
        (Box.Max.y - Box.Min.y) * 0.5f,
        (Box.Max.z - Box.Min.z) * 0.5f
    };
    DrawBox(center, extent, Color, LifeTime);
}

void KDebugRenderer::GenerateBoxVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                                          const XMFLOAT3& Extent, const XMMATRIX& Rotation, const XMFLOAT4& Color)
{
    XMFLOAT3 corners[8] = {
        { -Extent.x, -Extent.y, -Extent.z },
        {  Extent.x, -Extent.y, -Extent.z },
        {  Extent.x,  Extent.y, -Extent.z },
        { -Extent.x,  Extent.y, -Extent.z },
        { -Extent.x, -Extent.y,  Extent.z },
        {  Extent.x, -Extent.y,  Extent.z },
        {  Extent.x,  Extent.y,  Extent.z },
        { -Extent.x,  Extent.y,  Extent.z }
    };

    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR corner = XMLoadFloat3(&corners[i]);
        corner = XMVector3Transform(corner, Rotation);
        XMStoreFloat3(&corners[i], corner + XMLoadFloat3(&Center));
    }

    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    for (int i = 0; i < 12; ++i)
    {
        if (Vertices.size() + 2 > MaxVertices) return;
        Vertices.push_back({ corners[edges[i][0]], Color });
        Vertices.push_back({ corners[edges[i][1]], Color });
    }
}

void KDebugRenderer::DrawCapsule(const XMFLOAT3& Center, float Radius, float HalfHeight,
                                  const XMFLOAT4& Color, float LifeTime)
{
    if (LifeTime > 0.0f)
    {
        FDebugCapsule capsule;
        capsule.Center = Center;
        capsule.Radius = Radius;
        capsule.HalfHeight = HalfHeight;
        capsule.Color = Color;
        capsule.LifeTime = LifeTime;
        capsule.RemainingLifeTime = LifeTime;
        Capsules.push_back(capsule);
    }
    else
    {
        GenerateCapsuleVertices(FrameVertices, Center, Radius, HalfHeight, Color);
    }
}

void KDebugRenderer::GenerateCapsuleVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                                              float Radius, float HalfHeight, const XMFLOAT4& Color)
{
    int32 segments = 16;
    float step = XM_2PI / segments;

    XMFLOAT3 topCenter = { Center.x, Center.y + HalfHeight, Center.z };
    XMFLOAT3 bottomCenter = { Center.x, Center.y - HalfHeight, Center.z };

    for (int32 i = 0; i < segments; ++i)
    {
        float angle0 = i * step;
        float angle1 = (i + 1) * step;
        float cos0 = cosf(angle0) * Radius;
        float sin0 = sinf(angle0) * Radius;
        float cos1 = cosf(angle1) * Radius;
        float sin1 = sinf(angle1) * Radius;

        if (Vertices.size() + 2 > MaxVertices) return;
        Vertices.push_back({ { topCenter.x + cos0, topCenter.y, topCenter.z + sin0 }, Color });
        Vertices.push_back({ { topCenter.x + cos1, topCenter.y, topCenter.z + sin1 }, Color });

        if (Vertices.size() + 2 > MaxVertices) return;
        Vertices.push_back({ { bottomCenter.x + cos0, bottomCenter.y, bottomCenter.z + sin0 }, Color });
        Vertices.push_back({ { bottomCenter.x + cos1, bottomCenter.y, bottomCenter.z + sin1 }, Color });

        if (Vertices.size() + 2 > MaxVertices) return;
        Vertices.push_back({ { Center.x + cos0, topCenter.y, Center.z + sin0 }, Color });
        Vertices.push_back({ { Center.x + cos0, bottomCenter.y, Center.z + sin0 }, Color });

        for (int32 j = 0; j <= segments / 4; ++j)
        {
            float phi0 = j * step;
            float phi1 = (j + 1) * step;
            
            float y0 = cosf(phi0);
            float r0 = sinf(phi0);
            float y1 = cosf(phi1);
            float r1 = sinf(phi1);

            XMFLOAT3 p0 = { topCenter.x + r0 * cos0 * Radius,
                           topCenter.y + y0 * Radius,
                           topCenter.z + r0 * sin0 * Radius };
            XMFLOAT3 p1 = { topCenter.x + r0 * cos1 * Radius,
                           topCenter.y + y0 * Radius,
                           topCenter.z + r0 * sin1 * Radius };
            
            if (Vertices.size() + 4 > MaxVertices) return;
            Vertices.push_back({ p0, Color });
            Vertices.push_back({ p1, Color });

            XMFLOAT3 p2 = { bottomCenter.x + r0 * cos0 * Radius,
                           bottomCenter.y - y0 * Radius,
                           bottomCenter.z + r0 * sin0 * Radius };
            XMFLOAT3 p3 = { bottomCenter.x + r0 * cos1 * Radius,
                           bottomCenter.y - y0 * Radius,
                           bottomCenter.z + r0 * sin1 * Radius };
            
            Vertices.push_back({ p2, Color });
            Vertices.push_back({ p3, Color });
        }
    }
}

void KDebugRenderer::DrawFrustum(const XMMATRIX& ViewProjection, const XMFLOAT4& Color, float LifeTime)
{
    XMMATRIX invVP = XMMatrixInverse(nullptr, ViewProjection);
    
    XMFLOAT3 corners[8];
    XMFLOAT3 ndc[8] = {
        { -1, -1, 0 }, { 1, -1, 0 }, { 1, 1, 0 }, { -1, 1, 0 },
        { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }
    };

    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR corner = XMLoadFloat3(&ndc[i]);
        corner = XMVector3TransformCoord(corner, invVP);
        XMStoreFloat3(&corners[i], corner);
    }

    DrawFrustum(corners, Color, LifeTime);
}

void KDebugRenderer::DrawFrustum(const XMFLOAT3* Corners, const XMFLOAT4& Color, float LifeTime)
{
    DrawLine(Corners[0], Corners[1], Color, LifeTime);
    DrawLine(Corners[1], Corners[2], Color, LifeTime);
    DrawLine(Corners[2], Corners[3], Color, LifeTime);
    DrawLine(Corners[3], Corners[0], Color, LifeTime);
    
    DrawLine(Corners[4], Corners[5], Color, LifeTime);
    DrawLine(Corners[5], Corners[6], Color, LifeTime);
    DrawLine(Corners[6], Corners[7], Color, LifeTime);
    DrawLine(Corners[7], Corners[4], Color, LifeTime);
    
    DrawLine(Corners[0], Corners[4], Color, LifeTime);
    DrawLine(Corners[1], Corners[5], Color, LifeTime);
    DrawLine(Corners[2], Corners[6], Color, LifeTime);
    DrawLine(Corners[3], Corners[7], Color, LifeTime);
}

void KDebugRenderer::DrawGrid(const XMFLOAT3& Center, float GridSize, float CellSize,
                               const XMFLOAT4& Color, int32 Divisions)
{
    float halfSize = GridSize * 0.5f;
    XMFLOAT4 majorColor = { Color.x * 0.5f, Color.y * 0.5f, Color.z * 0.5f, Color.w };
    
    for (int32 i = -Divisions; i <= Divisions; ++i)
    {
        float offset = i * CellSize;
        bool isMajor = (i % 5 == 0);
        XMFLOAT4 lineColor = isMajor ? Color : majorColor;
        
        DrawLine({ Center.x - halfSize, Center.y, Center.z + offset },
                 { Center.x + halfSize, Center.y, Center.z + offset }, lineColor);
        DrawLine({ Center.x + offset, Center.y, Center.z - halfSize },
                 { Center.x + offset, Center.y, Center.z + halfSize }, lineColor);
    }
}

void KDebugRenderer::DrawAxis(const XMFLOAT3& Origin, float AxisLength, float LifeTime)
{
    DrawLine(Origin, { Origin.x + AxisLength, Origin.y, Origin.z }, { 1, 0, 0, 1 }, LifeTime);
    DrawLine(Origin, { Origin.x, Origin.y + AxisLength, Origin.z }, { 0, 1, 0, 1 }, LifeTime);
    DrawLine(Origin, { Origin.x, Origin.y, Origin.z + AxisLength }, { 0, 0, 1, 1 }, LifeTime);
}

void KDebugRenderer::DrawCoordinateSystem(const XMMATRIX& Transform, float Scale, float LifeTime)
{
    XMVECTOR origin = XMVectorSet(0, 0, 0, 1);
    XMVECTOR xAxis = XMVectorSet(Scale, 0, 0, 0);
    XMVECTOR yAxis = XMVectorSet(0, Scale, 0, 0);
    XMVECTOR zAxis = XMVectorSet(0, 0, Scale, 0);

    origin = XMVector3Transform(origin, Transform);
    xAxis = XMVector3Transform(xAxis, Transform);
    yAxis = XMVector3Transform(yAxis, Transform);
    zAxis = XMVector3Transform(zAxis, Transform);

    XMFLOAT3 o, x, y, z;
    XMStoreFloat3(&o, origin);
    XMStoreFloat3(&x, xAxis);
    XMStoreFloat3(&y, yAxis);
    XMStoreFloat3(&z, zAxis);

    DrawLine(o, x, { 1, 0, 0, 1 }, LifeTime);
    DrawLine(o, y, { 0, 1, 0, 1 }, LifeTime);
    DrawLine(o, z, { 0, 0, 1, 1 }, LifeTime);
}

void KDebugRenderer::DrawCone(const XMFLOAT3& Apex, const XMFLOAT3& Direction, float Height,
                               float Radius, const XMFLOAT4& Color, int32 Segments, float LifeTime)
{
    if (LifeTime > 0.0f)
    {
        GenerateConeVertices(FrameVertices, Apex, Direction, Height, Radius, Color, Segments);
    }
    else
    {
        GenerateConeVertices(FrameVertices, Apex, Direction, Height, Radius, Color, Segments);
    }
}

void KDebugRenderer::GenerateConeVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Apex,
                                           const XMFLOAT3& Direction, float Height, float Radius,
                                           const XMFLOAT4& Color, int32 Segments)
{
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&Direction));
    XMVECTOR apexVec = XMLoadFloat3(&Apex);
    XMVECTOR baseCenter = apexVec + dir * Height;

    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (fabsf(XMVectorGetX(XMVector3Dot(up, dir))) > 0.99f)
    {
        up = XMVectorSet(1, 0, 0, 0);
    }
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
    up = XMVector3Normalize(XMVector3Cross(dir, right));

    float step = XM_2PI / Segments;
    for (int32 i = 0; i < Segments; ++i)
    {
        float angle0 = i * step;
        float angle1 = (i + 1) * step;
        
        XMVECTOR offset0 = right * cosf(angle0) * Radius + up * sinf(angle0) * Radius;
        XMVECTOR offset1 = right * cosf(angle1) * Radius + up * sinf(angle1) * Radius;
        
        XMFLOAT3 p0, p1;
        XMStoreFloat3(&p0, baseCenter + offset0);
        XMStoreFloat3(&p1, baseCenter + offset1);

        if (Vertices.size() + 6 > MaxVertices) return;
        
        Vertices.push_back({ Apex, Color });
        Vertices.push_back({ p0, Color });
        
        Vertices.push_back({ p0, Color });
        Vertices.push_back({ p1, Color });
    }
}

void KDebugRenderer::DrawCylinder(const XMFLOAT3& Start, const XMFLOAT3& End, float Radius,
                                   const XMFLOAT4& Color, int32 Segments, float LifeTime)
{
    XMVECTOR startVec = XMLoadFloat3(&Start);
    XMVECTOR endVec = XMLoadFloat3(&End);
    XMVECTOR dir = XMVector3Normalize(endVec - startVec);
    
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (fabsf(XMVectorGetX(XMVector3Dot(up, dir))) > 0.99f)
    {
        up = XMVectorSet(1, 0, 0, 0);
    }
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
    up = XMVector3Normalize(XMVector3Cross(dir, right));

    float step = XM_2PI / Segments;
    for (int32 i = 0; i < Segments; ++i)
    {
        float angle0 = i * step;
        float angle1 = (i + 1) * step;
        
        XMVECTOR offset0 = right * cosf(angle0) * Radius + up * sinf(angle0) * Radius;
        XMVECTOR offset1 = right * cosf(angle1) * Radius + up * sinf(angle1) * Radius;
        
        XMFLOAT3 p0Start, p1Start, p0End, p1End;
        XMStoreFloat3(&p0Start, startVec + offset0);
        XMStoreFloat3(&p1Start, startVec + offset1);
        XMStoreFloat3(&p0End, endVec + offset0);
        XMStoreFloat3(&p1End, endVec + offset1);

        DrawLine(p0Start, p1Start, Color, LifeTime);
        DrawLine(p0End, p1End, Color, LifeTime);
        DrawLine(p0Start, p0End, Color, LifeTime);
    }
}

void KDebugRenderer::ClearPersistent()
{
    auto isNotPersistent = [](const FDebugLine& line) { return !line.bPersistent; };
    Lines.erase(std::remove_if(Lines.begin(), Lines.end(), isNotPersistent), Lines.end());
}

void KDebugRenderer::ClearAll()
{
    Lines.clear();
    Spheres.clear();
    Boxes.clear();
    Capsules.clear();
    Arrows.clear();
    FrameVertices.clear();
}
