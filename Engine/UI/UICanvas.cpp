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
    AddQuad(X, Y, Width, Height, Color);
}

void KUICanvas::FlushDrawCommands(ID3D11DeviceContext* Context)
{
    if (BatchedVertices.empty() || BatchedIndices.empty())
        return;

    UpdateBuffers(Context, BatchedVertices, BatchedIndices);

    Context->DrawIndexed(static_cast<UINT>(BatchedIndices.size()), 0, 0);
    
    BatchedVertices.clear();
    BatchedIndices.clear();
}
