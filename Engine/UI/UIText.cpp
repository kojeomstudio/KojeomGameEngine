#include "UIText.h"
#include "UICanvas.h"
#include <vector>

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

    std::vector<FUIVertex> vertices;
    std::vector<uint32> indices;

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
        font->RenderText(Context, Text, textX + DropShadowOffsetX, PositionY + DropShadowOffsetY,
                         scale, DropShadowColor, vertices, indices);
    }

    font->RenderText(Context, Text, textX, PositionY, scale, Color, vertices, indices);

    if (!vertices.empty() && !indices.empty())
    {
        UINT stride = sizeof(FUIVertex);
        UINT offset = 0;
        ID3D11Buffer* vb = nullptr;
        ID3D11Buffer* ib = nullptr;
        Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    }
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
