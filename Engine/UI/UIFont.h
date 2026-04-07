#pragma once

#include "UITypes.h"
#include "../Utils/Common.h"
#include <d3d11.h>
#include <map>
#include <vector>

class KUIFont
{
public:
    KUIFont();
    ~KUIFont();

    HRESULT Initialize(ID3D11Device* Device, const std::wstring& FontPath);
    void Cleanup();

    void RenderText(ID3D11DeviceContext* Context,
                    const std::wstring& Text,
                    float X, float Y,
                    float Scale,
                    const FColor& Color,
                    std::vector<FUIVertex>& OutVertices,
                    std::vector<uint32>& OutIndices);

    float GetTextWidth(const std::wstring& Text, float Scale = 1.0f) const;
    float GetLineHeight(float Scale = 1.0f) const;

    bool IsInitialized() const { return bInitialized; }
    ID3D11ShaderResourceView* GetTextureSRV() const { return FontTextureSRV.Get(); }

private:
    HRESULT LoadFontFile(const std::wstring& FontPath);
    HRESULT CreateFontTexture(ID3D11Device* Device, const std::wstring& TexturePath);

    ComPtr<ID3D11ShaderResourceView> FontTextureSRV;
    std::map<wchar_t, FFontChar> Characters;
    std::vector<FFontPage> Pages;

    float FontSize;
    float LineHeight;
    float Base;
    float TextureWidth;
    float TextureHeight;

    bool bInitialized;
};
