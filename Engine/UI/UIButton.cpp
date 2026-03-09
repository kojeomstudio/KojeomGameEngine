#include "UIButton.h"
#include "UICanvas.h"

KUIButton::KUIButton()
    : NormalColor(FColor::Gray(0.3f))
    , HoveredColor(FColor::Gray(0.4f))
    , PressedColor(FColor::Gray(0.2f))
    , DisabledColor(FColor::Gray(0.15f))
{
    BackgroundPanel = std::make_shared<KUIPanel>();
    BackgroundPanel->SetBackgroundColor(NormalColor);
    BackgroundPanel->SetBorderColor(FColor::Gray(0.5f));
    BackgroundPanel->SetBorderWidth(1.0f);

    TextElement = std::make_shared<KUIText>();
    TextElement->SetColor(FColor::White());
    TextElement->SetFontSize(1.0f);
}

KUIButton::~KUIButton()
{
}

void KUIButton::Update(float DeltaTime)
{
    KUIElement::Update(DeltaTime);

    if (BackgroundPanel)
    {
        BackgroundPanel->SetPosition(PositionX, PositionY);
        BackgroundPanel->SetSize(Width, Height);
        BackgroundPanel->SetBackgroundColor(GetCurrentColor());
        BackgroundPanel->Update(DeltaTime);
    }

    if (TextElement)
    {
        float textHeight = TextElement->GetTextHeight();
        float textY = PositionY + (Height - textHeight) * 0.5f;
        TextElement->SetPosition(PositionX + 10.0f, textY);
        TextElement->SetSize(Width - 20.0f, textHeight);
        TextElement->SetTextAlignment(EUIHorizontalAlignment::Center);
        TextElement->Update(DeltaTime);
    }
}

void KUIButton::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!IsVisible()) return;

    if (BackgroundPanel)
    {
        BackgroundPanel->Render(Context, Canvas);
    }

    if (TextElement)
    {
        TextElement->Render(Context, Canvas);
    }
}

void KUIButton::OnHoverEnter()
{
    KUIElement::OnHoverEnter();
    if (BackgroundPanel)
    {
        BackgroundPanel->SetBackgroundColor(HoveredColor);
    }
}

void KUIButton::OnHoverLeave()
{
    KUIElement::OnHoverLeave();
    if (BackgroundPanel)
    {
        BackgroundPanel->SetBackgroundColor(NormalColor);
    }
}

void KUIButton::OnMouseDown(float X, float Y, int Button)
{
    KUIElement::OnMouseDown(X, Y, Button);
    if (BackgroundPanel)
    {
        BackgroundPanel->SetBackgroundColor(PressedColor);
    }
}

void KUIButton::OnMouseUp(float X, float Y, int Button)
{
    KUIElement::OnMouseUp(X, Y, Button);
    if (BackgroundPanel)
    {
        BackgroundPanel->SetBackgroundColor(bIsHovered ? HoveredColor : NormalColor);
    }
}

void KUIButton::SetText(const std::wstring& InText)
{
    if (TextElement)
    {
        TextElement->SetText(InText);
    }
}

const std::wstring& KUIButton::GetText() const
{
    static const std::wstring empty;
    return TextElement ? TextElement->GetText() : empty;
}

void KUIButton::SetTextColor(const FColor& Color)
{
    if (TextElement)
    {
        TextElement->SetColor(Color);
    }
}

void KUIButton::SetFontSize(float Size)
{
    if (TextElement)
    {
        TextElement->SetFontSize(Size);
    }
}

FColor KUIButton::GetCurrentColor() const
{
    if (!bIsEnabled) return DisabledColor;
    if (bIsPressed) return PressedColor;
    if (bIsHovered) return HoveredColor;
    return NormalColor;
}
