#include "UICanvas.h"
#include "UIElement.h"
#include "../Utils/Logger.h"
#include "../Utils/Common.h"
#include <d3dcompiler.h>

struct FUIConstantBuffer
{
    XMMATRIX Projection;
};

KUICanvas::KUICanvas()
    : Device(nullptr)
    , CanvasWidth(0)
    , CanvasHeight(0)
    , HoveredElement(nullptr)
    , FocusedElement(nullptr)
    , PressedElement(nullptr)
    , MouseX(0)
    , MouseY(0)
    , bInitialized(false)
{
}

KUICanvas::~KUICanvas()
{
    Cleanup();
}

void KUICanvas::Cleanup()
{
    Elements.clear();
    DefaultFont.reset();

    ConstantBuffer.Reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    InputLayout.Reset();
    VertexShader.Reset();
    PixelShader.Reset();
    SamplerState.Reset();
    BlendState.Reset();
    RasterizerState.Reset();
    DepthStencilState.Reset();
    NullTextureSRV.Reset();

    Device = nullptr;
    bInitialized = false;
}

HRESULT KUICanvas::Initialize(ID3D11Device* InDevice, UINT32 Width, UINT32 Height)
{
    if (!InDevice)
    {
        LOG_ERROR("Invalid device for UI canvas");
        return E_INVALIDARG;
    }

    Device = InDevice;
    CanvasWidth = Width;
    CanvasHeight = Height;

    HRESULT hr = CreateConstantBuffer();
    if (FAILED(hr)) return hr;

    hr = CreateVertexBuffer();
    if (FAILED(hr)) return hr;

    hr = CreateIndexBuffer();
    if (FAILED(hr)) return hr;

    hr = CreateSamplerState();
    if (FAILED(hr)) return hr;

    hr = CreateBlendState();
    if (FAILED(hr)) return hr;

    hr = CreateRasterizerState();
    if (FAILED(hr)) return hr;

    hr = CreateDepthStencilState();
    if (FAILED(hr)) return hr;

    hr = CompileShaders();
    if (FAILED(hr)) return hr;

    hr = CreateNullTexture();
    if (FAILED(hr)) return hr;

    bInitialized = true;
    LOG_INFO("UI Canvas initialized");
    return S_OK;
}

HRESULT KUICanvas::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(FUIConstantBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return Device->CreateBuffer(&desc, nullptr, &ConstantBuffer);
}

HRESULT KUICanvas::CreateVertexBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(FUIVertex) * 65536;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return Device->CreateBuffer(&desc, nullptr, &VertexBuffer);
}

HRESULT KUICanvas::CreateIndexBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(uint32) * 98304;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return Device->CreateBuffer(&desc, nullptr, &IndexBuffer);
}

HRESULT KUICanvas::CreateSamplerState()
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MaxAnisotropy = 1;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;

    return Device->CreateSamplerState(&desc, &SamplerState);
}

HRESULT KUICanvas::CreateBlendState()
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return Device->CreateBlendState(&desc, &BlendState);
}

HRESULT KUICanvas::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthBiasClamp = 0.0f;
    desc.DepthClipEnable = FALSE;
    desc.ScissorEnable = FALSE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    return Device->CreateRasterizerState(&desc, &RasterizerState);
}

HRESULT KUICanvas::CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = FALSE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable = FALSE;

    return Device->CreateDepthStencilState(&desc, &DepthStencilState);
}

HRESULT KUICanvas::CreateNullTexture()
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    uint32 whitePixel = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &whitePixel;
    initData.SysMemPitch = sizeof(uint32);

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = Device->CreateTexture2D(&texDesc, &initData, &texture);
    if (FAILED(hr)) return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    return Device->CreateShaderResourceView(texture.Get(), &srvDesc, &NullTextureSRV);
}

HRESULT KUICanvas::CompileShaders()
{
    const char* vsSource = R"(
        cbuffer UICBuffer : register(b0)
        {
            matrix Projection;
        };
        
        struct VS_INPUT
        {
            float3 Position : POSITION;
            float2 TexCoord : TEXCOORD0;
            float4 Color : COLOR0;
        };
        
        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float2 TexCoord : TEXCOORD0;
            float4 Color : COLOR0;
        };
        
        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT output;
            output.Position = mul(Projection, float4(input.Position.xy, 0.0, 1.0));
            output.TexCoord = input.TexCoord;
            output.Color = input.Color;
            return output;
        }
    )";

    const char* psSource = R"(
        Texture2D DiffuseTexture : register(t0);
        SamplerState TextureSampler : register(s0);
        
        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float2 TexCoord : TEXCOORD0;
            float4 Color : COLOR0;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            float4 texColor = DiffuseTexture.Sample(TextureSampler, input.TexCoord);
            return texColor * input.Color;
        }
    )";

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), "UIVS", nullptr, nullptr,
                            "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            LOG_ERROR("UI Vertex Shader compilation failed");
            errorBlob.Reset();
        }
        return hr;
    }

    hr = D3DCompile(psSource, strlen(psSource), "UIPS", nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            LOG_ERROR("UI Pixel Shader compilation failed");
            errorBlob.Reset();
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
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = Device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(),
                                   vsBlob->GetBufferSize(), &InputLayout);
    return hr;
}

void KUICanvas::SetSize(UINT32 Width, UINT32 Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;
}

void KUICanvas::Update(float DeltaTime)
{
    for (auto& element : Elements)
    {
        if (element->IsVisible())
        {
            element->Update(DeltaTime);
        }
    }
}

void KUICanvas::Render(ID3D11DeviceContext* Context)
{
    if (!bInitialized || !Context) return;

    BatchedVertices.clear();
    BatchedIndices.clear();
    TexturedBatches.clear();

    XMMATRIX projection = XMMatrixOrthographicOffCenterLH(
        0.0f, static_cast<float>(CanvasWidth),
        static_cast<float>(CanvasHeight), 0.0f,
        0.0f, 1.0f
    );

    UpdateConstantBuffer(Context, projection);

    Context->IASetInputLayout(InputLayout.Get());
    Context->VSSetShader(VertexShader.Get(), nullptr, 0);
    Context->PSSetShader(PixelShader.Get(), nullptr, 0);
    Context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());

    ID3D11Buffer* cb = ConstantBuffer.Get();
    Context->VSSetConstantBuffers(0, 1, &cb);

    Context->OMSetBlendState(BlendState.Get(), nullptr, 0xFFFFFFFF);
    Context->OMSetDepthStencilState(DepthStencilState.Get(), 0);
    Context->RSSetState(RasterizerState.Get());

    for (auto& element : Elements)
    {
        if (element->IsVisible())
        {
            element->Render(Context, this);
        }
    }
    
    FlushDrawCommands(Context);
}

void KUICanvas::UpdateConstantBuffer(ID3D11DeviceContext* Context, const XMMATRIX& Transform)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = Context->Map(ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        FUIConstantBuffer* cb = reinterpret_cast<FUIConstantBuffer*>(mapped.pData);
        cb->Projection = XMMatrixTranspose(Transform);
        Context->Unmap(ConstantBuffer.Get(), 0);
    }
}

void KUICanvas::UpdateBuffers(ID3D11DeviceContext* Context,
                              const std::vector<FUIVertex>& Vertices,
                              const std::vector<uint32>& Indices)
{
    if (Vertices.empty() || Indices.empty()) return;

    D3D11_MAPPED_SUBRESOURCE mapped;

    HRESULT hr = Context->Map(VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, Vertices.data(), Vertices.size() * sizeof(FUIVertex));
        Context->Unmap(VertexBuffer.Get(), 0);
    }

    hr = Context->Map(IndexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, Indices.data(), Indices.size() * sizeof(uint32));
        Context->Unmap(IndexBuffer.Get(), 0);
    }

    UINT stride = sizeof(FUIVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = VertexBuffer.Get();
    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void KUICanvas::AddElement(std::shared_ptr<KUIElement> Element)
{
    if (Element)
    {
        Elements.push_back(Element);
    }
}

void KUICanvas::RemoveElement(std::shared_ptr<KUIElement> Element)
{
    Elements.remove(Element);
}

void KUICanvas::ClearElements()
{
    Elements.clear();
}

void KUICanvas::OnMouseMove(float X, float Y)
{
    MouseX = X;
    MouseY = Y;

    KUIElement* hovered = GetElementAt(X, Y);
    if (hovered != HoveredElement)
    {
        if (HoveredElement)
        {
            HoveredElement->OnHoverLeave();
        }
        HoveredElement = hovered;
        if (HoveredElement)
        {
            HoveredElement->OnHoverEnter();
        }
    }
}

void KUICanvas::OnMouseDown(float X, float Y, int Button)
{
    KUIElement* element = GetElementAt(X, Y);
    if (element)
    {
        PressedElement = element;
        element->OnMouseDown(X, Y, Button);
    }
}

void KUICanvas::OnMouseUp(float X, float Y, int Button)
{
    if (PressedElement)
    {
        PressedElement->OnMouseUp(X, Y, Button);

        KUIElement* releasedOn = GetElementAt(X, Y);
        if (releasedOn == PressedElement)
        {
            PressedElement->OnClick();
            SetFocusedElement(PressedElement);
        }
    }
    PressedElement = nullptr;
}

void KUICanvas::OnKeyDown(int KeyCode)
{
    if (FocusedElement)
    {
        FocusedElement->OnKeyDown(KeyCode);
    }
}

void KUICanvas::OnKeyUp(int KeyCode)
{
    if (FocusedElement)
    {
        FocusedElement->OnKeyUp(KeyCode);
    }
}

KUIElement* KUICanvas::GetElementAt(float X, float Y) const
{
    for (auto it = Elements.rbegin(); it != Elements.rend(); ++it)
    {
        auto& element = *it;
        if (element->IsVisible() && element->HitTest(X, Y))
        {
            return element.get();
        }
    }
    return nullptr;
}

void KUICanvas::SetFocusedElement(KUIElement* Element)
{
    if (FocusedElement != Element)
    {
        if (FocusedElement)
        {
            FocusedElement->OnFocusLost();
        }
        FocusedElement = Element;
        if (FocusedElement)
        {
            FocusedElement->OnFocus();
        }
    }
}

void KUICanvas::AddQuad(float X, float Y, float Width, float Height, const FColor& Color)
{
    XMFLOAT4 color = Color.ToFloat4();
    uint32 baseIndex = static_cast<uint32>(BatchedVertices.size());

    BatchedVertices.push_back(FUIVertex(XMFLOAT3(X, Y, 0), XMFLOAT2(0, 0), color));
    BatchedVertices.push_back(FUIVertex(XMFLOAT3(X + Width, Y, 0), XMFLOAT2(1, 0), color));
    BatchedVertices.push_back(FUIVertex(XMFLOAT3(X + Width, Y + Height, 0), XMFLOAT2(1, 1), color));
    BatchedVertices.push_back(FUIVertex(XMFLOAT3(X, Y + Height, 0), XMFLOAT2(0, 1), color));

    BatchedIndices.push_back(baseIndex + 0);
    BatchedIndices.push_back(baseIndex + 1);
    BatchedIndices.push_back(baseIndex + 2);
    BatchedIndices.push_back(baseIndex + 0);
    BatchedIndices.push_back(baseIndex + 2);
    BatchedIndices.push_back(baseIndex + 3);
}

void KUICanvas::AddQuad(float X, float Y, float Width, float Height, const FColor& Color, ID3D11ShaderResourceView* Texture)
{
    if (!Texture)
    {
        AddQuad(X, Y, Width, Height, Color);
        return;
    }

    AddTexturedQuad(X, Y, Width, Height, Color, FRect(0, 0, 1, 1), Texture);
}

void KUICanvas::AddTexturedQuad(float X, float Y, float Width, float Height, const FColor& Color, const FRect& UVRect, ID3D11ShaderResourceView* Texture)
{
    if (!Texture)
    {
        AddQuad(X, Y, Width, Height, Color);
        return;
    }

    FTexturedBatch* targetBatch = nullptr;
    if (!TexturedBatches.empty() && TexturedBatches.back().Texture == Texture)
    {
        targetBatch = &TexturedBatches.back();
    }
    else
    {
        TexturedBatches.push_back(FTexturedBatch());
        TexturedBatches.back().Texture = Texture;
        targetBatch = &TexturedBatches.back();
    }

    XMFLOAT4 color = Color.ToFloat4();
    uint32 baseIndex = static_cast<uint32>(targetBatch->Vertices.size());

    float u0 = UVRect.X;
    float v0 = UVRect.Y;
    float u1 = UVRect.X + UVRect.Width;
    float v1 = UVRect.Y + UVRect.Height;

    targetBatch->Vertices.push_back(FUIVertex(XMFLOAT3(X, Y, 0), XMFLOAT2(u0, v0), color));
    targetBatch->Vertices.push_back(FUIVertex(XMFLOAT3(X + Width, Y, 0), XMFLOAT2(u1, v0), color));
    targetBatch->Vertices.push_back(FUIVertex(XMFLOAT3(X + Width, Y + Height, 0), XMFLOAT2(u1, v1), color));
    targetBatch->Vertices.push_back(FUIVertex(XMFLOAT3(X, Y + Height, 0), XMFLOAT2(u0, v1), color));

    targetBatch->Indices.push_back(baseIndex + 0);
    targetBatch->Indices.push_back(baseIndex + 1);
    targetBatch->Indices.push_back(baseIndex + 2);
    targetBatch->Indices.push_back(baseIndex + 0);
    targetBatch->Indices.push_back(baseIndex + 2);
    targetBatch->Indices.push_back(baseIndex + 3);
}

void KUICanvas::AddText(KUIFont* Font, const std::wstring& Text, float X, float Y, float Scale, const FColor& Color)
{
    if (!Font || !Font->IsInitialized() || Text.empty()) return;

    ID3D11ShaderResourceView* fontTexture = Font->GetTextureSRV();

    FTexturedBatch* targetBatch = nullptr;
    if (!TexturedBatches.empty() && TexturedBatches.back().Texture == fontTexture)
    {
        targetBatch = &TexturedBatches.back();
    }
    else
    {
        TexturedBatches.push_back(FTexturedBatch());
        TexturedBatches.back().Texture = fontTexture;
        targetBatch = &TexturedBatches.back();
    }

    Font->RenderText(nullptr, Text, X, Y, Scale, Color, targetBatch->Vertices, targetBatch->Indices);
}

void KUICanvas::FlushDrawCommands(ID3D11DeviceContext* Context)
{
    if (!Context) return;

    if (!BatchedVertices.empty() && !BatchedIndices.empty())
    {
        UpdateBuffers(Context, BatchedVertices, BatchedIndices);

        Context->PSSetShaderResources(0, 1, NullTextureSRV.GetAddressOf());

        Context->DrawIndexed(static_cast<UINT>(BatchedIndices.size()), 0, 0);
    }

    BatchedVertices.clear();
    BatchedIndices.clear();

    for (auto& batch : TexturedBatches)
    {
        if (batch.Vertices.empty() || batch.Indices.empty()) continue;

        UpdateBuffers(Context, batch.Vertices, batch.Indices);

        Context->PSSetShaderResources(0, 1, &batch.Texture);

        Context->DrawIndexed(static_cast<UINT>(batch.Indices.size()), 0, 0);

        batch.Vertices.clear();
        batch.Indices.clear();
    }

    TexturedBatches.clear();

    ID3D11ShaderResourceView* nullSRV = nullptr;
    Context->PSSetShaderResources(0, 1, &nullSRV);
}
