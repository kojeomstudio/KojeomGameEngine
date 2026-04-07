#include "UIImage.h"
#include "UICanvas.h"

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

    Canvas->AddTexturedQuad(x, y, w, h, Tint, UVRect, TextureSRV.Get());
}
