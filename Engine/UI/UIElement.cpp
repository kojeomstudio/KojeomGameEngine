#include "UIElement.h"
#include "UICanvas.h"

KUIElement::KUIElement()
    : PositionX(0)
    , PositionY(0)
    , Width(100)
    , Height(100)
    , PivotX(0)
    , PivotY(0)
    , Anchor(EUIAnchor::TopLeft)
    , HorizontalAlignment(EUIHorizontalAlignment::Left)
    , VerticalAlignment(EUIVerticalAlignment::Top)
    , bIsVisible(true)
    , bIsEnabled(true)
    , bIsInteractable(true)
    , bIsHovered(false)
    , bIsPressed(false)
    , bIsFocused(false)
    , UserData(nullptr)
{
}

KUIElement::~KUIElement()
{
}

void KUIElement::Update(float DeltaTime)
{
}

void KUIElement::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
}

void KUIElement::OnHoverEnter()
{
    bIsHovered = true;
}

void KUIElement::OnHoverLeave()
{
    bIsHovered = false;
}

void KUIElement::OnMouseDown(float X, float Y, int Button)
{
    bIsPressed = true;
}

void KUIElement::OnMouseUp(float X, float Y, int Button)
{
    bIsPressed = false;
}

void KUIElement::OnClick()
{
    if (OnClickCallback)
    {
        OnClickCallback();
    }
}

void KUIElement::OnFocus()
{
    bIsFocused = true;
}

void KUIElement::OnFocusLost()
{
    bIsFocused = false;
}

void KUIElement::OnKeyDown(int KeyCode)
{
}

void KUIElement::OnKeyUp(int KeyCode)
{
}

bool KUIElement::HitTest(float X, float Y) const
{
    if (!bIsInteractable) return false;
    return X >= PositionX && X <= PositionX + Width && Y >= PositionY && Y <= PositionY + Height;
}

void KUIElement::SetPosition(float X, float Y)
{
    PositionX = X;
    PositionY = Y;
    UpdateTransform();
}

void KUIElement::SetSize(float InWidth, float InHeight)
{
    Width = InWidth;
    Height = InHeight;
    UpdateTransform();
}

void KUIElement::SetRect(const FRect& InRect)
{
    PositionX = InRect.X;
    PositionY = InRect.Y;
    Width = InRect.Width;
    Height = InRect.Height;
    UpdateTransform();
}

void KUIElement::SetAlignment(EUIHorizontalAlignment InHAlign, EUIVerticalAlignment InVAlign)
{
    HorizontalAlignment = InHAlign;
    VerticalAlignment = InVAlign;
    UpdateTransform();
}

void KUIElement::UpdateTransform()
{
}
