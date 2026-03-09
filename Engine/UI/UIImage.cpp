#include "UIImage.h"
#include "UICanvas.h"
#include <vector>

KUIImage::KUIImage()
    : Tint(FColor::White())
    , UVRect(0, 0, 1, 1)
    , bPreserveAspectRatio(false)
    , TextureWidth(0)
    , TextureHeight(0)
{
}

KUIImage::~KUIImage()
{
    TextureSRV.Reset();
}

HRESULT KUIImage::Initialize(ID3D11Device* Device, const std::wstring& TexturePath)
{
    return E_NOTIMPL;
}

void KUIImage::SetTexture(ID3D11ShaderResourceView* InTextureSRV)
{
    TextureSRV = InTextureSRV;
}

void KUIImage::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!IsVisible() || !TextureSRV) return;

    std::vector<FUIVertex> vertices;
    std::vector<uint32> indices;

    XMFLOAT4 color = Tint.ToFloat4();

    float x = PositionX;
    float y = PositionY;
    float w = Width;
    float h = Height;

    if (bPreserveAspectRatio && TextureWidth > 0 && TextureHeight > 0)
    {
        float aspectRatio = TextureWidth / TextureHeight;
        float newWidth = h * aspectRatio;
        float newHeight = w / aspectRatio;

        if (newWidth <= w)
        {
            x += (w - newWidth) * 0.5f;
            w = newWidth;
        }
        else
        {
            y += (h - newHeight) * 0.5f;
            h = newHeight;
        }
    }

    float u0 = UVRect.X;
    float v0 = UVRect.Y;
    float u1 = UVRect.X + UVRect.Width;
    float v1 = UVRect.Y + UVRect.Height;

    uint32 baseIndex = 0;

    vertices.push_back(FUIVertex(XMFLOAT3(x, y, 0), XMFLOAT2(u0, v0), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x + w, y, 0), XMFLOAT2(u1, v0), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x + w, y + h, 0), XMFLOAT2(u1, v1), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x, y + h, 0), XMFLOAT2(u0, v1), color));

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    Context->PSSetShaderResources(0, 1, TextureSRV.GetAddressOf());

    UINT stride = sizeof(FUIVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
}
