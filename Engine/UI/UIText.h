#pragma once

#include "UIElement.h"
#include "UIFont.h"

class KUIText : public KUIElement
{
public:
    KUIText();
    virtual ~KUIText();

    virtual void Update(float DeltaTime) override;
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;

    void SetText(const std::wstring& InText);
    const std::wstring& GetText() const { return Text; }

    void SetFont(std::shared_ptr<KUIFont> InFont) { Font = InFont; }
    std::shared_ptr<KUIFont> GetFont() const { return Font; }

    void SetColor(const FColor& InColor) { Color = InColor; }
    const FColor& GetColor() const { return Color; }

    void SetFontSize(float Size) { FontSize = Size; }
    float GetFontSize() const { return FontSize; }

    void SetDropShadow(bool bEnabled) { bHasDropShadow = bEnabled; }
    bool HasDropShadow() const { return bHasDropShadow; }

    void SetDropShadowColor(const FColor& InColor) { DropShadowColor = InColor; }
    void SetDropShadowOffset(float X, float Y) { DropShadowOffsetX = X; DropShadowOffsetY = Y; }

    void SetTextAlignment(EUIHorizontalAlignment InAlignment) { TextAlignment = InAlignment; }
    EUIHorizontalAlignment GetTextAlignment() const { return TextAlignment; }

    void SetWordWrap(bool bEnabled) { bWordWrap = bEnabled; }
    bool IsWordWrap() const { return bWordWrap; }

    float GetTextWidth() const;
    float GetTextHeight() const;

private:
    std::wstring Text;
    std::shared_ptr<KUIFont> Font;
    FColor Color;
    float FontSize;
    bool bHasDropShadow;
    FColor DropShadowColor;
    float DropShadowOffsetX;
    float DropShadowOffsetY;
    EUIHorizontalAlignment TextAlignment;
    bool bWordWrap;
};
