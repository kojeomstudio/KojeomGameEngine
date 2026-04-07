#pragma once

#include "UITypes.h"
#include "UIFont.h"
#include "../Utils/Common.h"
#include <d3d11.h>
#include <memory>
#include <vector>
#include <list>

class KUIElement;

struct FTexturedBatch
{
    ID3D11ShaderResourceView* Texture = nullptr;
    std::vector<FUIVertex> Vertices;
    std::vector<uint32> Indices;
};

class KUICanvas
{
public:
    KUICanvas();
    ~KUICanvas();

    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Cleanup();

    void Update(float DeltaTime);
    void Render(ID3D11DeviceContext* Context);

    void SetSize(UINT32 Width, UINT32 Height);
    void GetSize(UINT32& OutWidth, UINT32& OutHeight) const { OutWidth = CanvasWidth; OutHeight = CanvasHeight; }

    void AddElement(std::shared_ptr<KUIElement> Element);
    void RemoveElement(std::shared_ptr<KUIElement> Element);
    void ClearElements();
    const std::list<std::shared_ptr<KUIElement>>& GetElements() const { return Elements; }

    void OnMouseMove(float X, float Y);
    void OnMouseDown(float X, float Y, int Button);
    void OnMouseUp(float X, float Y, int Button);
    void OnKeyDown(int KeyCode);
    void OnKeyUp(int KeyCode);

    KUIElement* GetElementAt(float X, float Y) const;
    KUIElement* GetFocusedElement() const { return FocusedElement; }
    void SetFocusedElement(KUIElement* Element);

    ID3D11Device* GetDevice() const { return Device; }
    KUIFont* GetDefaultFont() const { return DefaultFont.get(); }

    void SetDefaultFont(std::shared_ptr<KUIFont> Font) { DefaultFont = Font; }
    
    void AddQuad(float X, float Y, float Width, float Height, const FColor& Color);
    void AddQuad(float X, float Y, float Width, float Height, const FColor& Color, ID3D11ShaderResourceView* Texture);
    void AddTexturedQuad(float X, float Y, float Width, float Height, const FColor& Color, const FRect& UVRect, ID3D11ShaderResourceView* Texture);
    void AddText(KUIFont* Font, const std::wstring& Text, float X, float Y, float Scale, const FColor& Color);
    
    void FlushDrawCommands(ID3D11DeviceContext* Context);

private:
    HRESULT CreateConstantBuffer();
    HRESULT CreateVertexBuffer();
    HRESULT CreateIndexBuffer();
    HRESULT CreateSamplerState();
    HRESULT CreateBlendState();
    HRESULT CreateRasterizerState();
    HRESULT CreateDepthStencilState();
    HRESULT CompileShaders();
    HRESULT CreateNullTexture();

    void UpdateConstantBuffer(ID3D11DeviceContext* Context, const XMMATRIX& Transform);
    void UpdateBuffers(ID3D11DeviceContext* Context, const std::vector<FUIVertex>& Vertices, const std::vector<uint32>& Indices);

    ID3D11Device* Device;
    UINT32 CanvasWidth;
    UINT32 CanvasHeight;

    ComPtr<ID3D11Buffer> ConstantBuffer;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11SamplerState> SamplerState;
    ComPtr<ID3D11BlendState> BlendState;
    ComPtr<ID3D11RasterizerState> RasterizerState;
    ComPtr<ID3D11DepthStencilState> DepthStencilState;

    ComPtr<ID3D11ShaderResourceView> NullTextureSRV;

    std::list<std::shared_ptr<KUIElement>> Elements;
    std::shared_ptr<KUIFont> DefaultFont;

    KUIElement* HoveredElement;
    KUIElement* FocusedElement;
    KUIElement* PressedElement;

    float MouseX, MouseY;
    bool bInitialized;
    
    std::vector<FUIVertex> BatchedVertices;
    std::vector<uint32> BatchedIndices;
    std::vector<FTexturedBatch> TexturedBatches;
};
