#include "UIText.h"
#include "UICanvas.h"

KUIText::KUIText()
    : Color(FColor::White())
    , FontSize(1.0f)
    , bHasDropShadow(false)
    , DropShadowColor(FColor::Black())
    , DropShadowOffsetX(2.0f)
    , DropShadowOffsetY(2.0f)
    , TextAlignment(EUIHorizontalAlignment::Left)
    , bWordWrap(false)
{
}

KUIText::~KUIText()
{
}

void KUIText::Update(float DeltaTime)
{
    KUIElement::Update(DeltaTime);
}

void KUIText::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!IsVisible() || Text.empty()) return;

    std::shared_ptr<KUIFont> font = Font;
    if (!font)
    {
        font = std::shared_ptr<KUIFont>(Canvas->GetDefaultFont());
    }

    if (!font || !font->IsInitialized()) return;

    float scale = FontSize;
    float textX = PositionX;

    if (TextAlignment == EUIHorizontalAlignment::Center)
    {
        float textWidth = font->GetTextWidth(Text, scale);
        textX = PositionX + (Width - textWidth) * 0.5f;
    }
    else if (TextAlignment == EUIHorizontalAlignment::Right)
    {
        float textWidth = font->GetTextWidth(Text, scale);
        textX = PositionX + Width - textWidth;
    }

    if (bHasDropShadow)
    {
        Canvas->AddText(font.get(), Text, textX + DropShadowOffsetX,
                        PositionY + DropShadowOffsetY, scale, DropShadowColor);
    }

    Canvas->AddText(font.get(), Text, textX, PositionY, scale, Color);
}

void KUIText::SetText(const std::wstring& InText)
{
    Text = InText;
}

float KUIText::GetTextWidth() const
{
    if (Font)
    {
        return Font->GetTextWidth(Text, FontSize);
    }
    return Text.length() * 12.0f * FontSize;
}

float KUIText::GetTextHeight() const
{
    if (Font)
    {
        return Font->GetLineHeight(FontSize);
    }
    return 16.0f * FontSize;
}
