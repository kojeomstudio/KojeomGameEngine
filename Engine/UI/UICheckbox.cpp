#include "UICheckbox.h"
#include "UICanvas.h"

KUICheckbox::KUICheckbox()
    : bIsChecked(false)
    , BoxSize(24.0f)
    , BoxColor(0.3f, 0.3f, 0.3f, 1.0f)
    , CheckColor(0.4f, 0.6f, 1.0f, 1.0f)
    , HoverColor(0.4f, 0.4f, 0.4f, 1.0f)
{
    bIsInteractable = true;
    Width = BoxSize;
    Height = BoxSize;
}

KUICheckbox::~KUICheckbox()
{
}

void KUICheckbox::Update(float DeltaTime)
{
    KUIElement::Update(DeltaTime);
}

void KUICheckbox::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!bIsVisible || !Canvas)
        return;

    FColor currentBoxColor = BoxColor;
    if (bIsHovered)
    {
        currentBoxColor = HoverColor;
    }

    Canvas->AddQuad(PositionX, PositionY, BoxSize, BoxSize, currentBoxColor);

    float borderSize = 2.0f;
    Canvas->AddQuad(PositionX, PositionY, BoxSize, borderSize, FColor::White());
    Canvas->AddQuad(PositionX, PositionY + BoxSize - borderSize, BoxSize, borderSize, FColor::White());
    Canvas->AddQuad(PositionX, PositionY, borderSize, BoxSize, FColor::White());
    Canvas->AddQuad(PositionX + BoxSize - borderSize, PositionY, borderSize, BoxSize, FColor::White());

    if (bIsChecked)
    {
        float padding = BoxSize * 0.2f;
        float checkSize = BoxSize - padding * 2;
        
        Canvas->AddQuad(
            PositionX + padding,
            PositionY + padding,
            checkSize,
            checkSize * 0.2f,
            CheckColor
        );
        
        Canvas->AddQuad(
            PositionX + padding,
            PositionY + padding + checkSize * 0.3f,
            checkSize * 0.3f,
            checkSize * 0.2f,
            CheckColor
        );
    }
}

void KUICheckbox::OnMouseDown(float X, float Y, int Button)
{
    if (Button == 0)
    {
        bIsPressed = true;
    }
}

void KUICheckbox::OnClick()
{
    bIsChecked = !bIsChecked;
    
    if (OnValueChangedCallback)
    {
        OnValueChangedCallback(bIsChecked);
    }
    
    KUIElement::OnClick();
}

bool KUICheckbox::HitTest(float X, float Y) const
{
    return (X >= PositionX && X <= PositionX + BoxSize &&
            Y >= PositionY && Y <= PositionY + BoxSize);
}

void KUICheckbox::SetChecked(bool bChecked)
{
    bIsChecked = bChecked;
}
