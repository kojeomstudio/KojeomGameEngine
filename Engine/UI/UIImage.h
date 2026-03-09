#pragma once

#include "UIElement.h"
#include <d3d11.h>

class KUIImage : public KUIElement
{
public:
    KUIImage();
    virtual ~KUIImage();

    HRESULT Initialize(ID3D11Device* Device, const std::wstring& TexturePath);
    void SetTexture(ID3D11ShaderResourceView* InTextureSRV);

    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;

    void SetTint(const FColor& InTint) { Tint = InTint; }
    const FColor& GetTint() const { return Tint; }

    void SetUVRect(const FRect& InUVRect) { UVRect = InUVRect; }
    const FRect& GetUVRect() const { return UVRect; }

    void SetPreserveAspectRatio(bool bPreserve) { bPreserveAspectRatio = bPreserve; }
    bool IsPreserveAspectRatio() const { return bPreserveAspectRatio; }

    ID3D11ShaderResourceView* GetTextureSRV() const { return TextureSRV.Get(); }

private:
    ComPtr<ID3D11ShaderResourceView> TextureSRV;
    FColor Tint;
    FRect UVRect;
    bool bPreserveAspectRatio;
    float TextureWidth;
    float TextureHeight;
};
