#include "UISlider.h"
#include "UICanvas.h"
#include <algorithm>

KUISlider::KUISlider()
    : Value(0.5f)
    , MinValue(0.0f)
    , MaxValue(1.0f)
    , HandleWidth(20.0f)
    , HandleHeight(20.0f)
    , HandleX(0.0f)
    , HandleY(0.0f)
    , TrackColor(0.3f, 0.3f, 0.3f, 1.0f)
    , HandleColor(0.8f, 0.8f, 0.8f, 1.0f)
    , FillColor(0.4f, 0.6f, 1.0f, 1.0f)
    , bIsDragging(false)
{
    bIsInteractable = true;
}

KUISlider::~KUISlider()
{
}

void KUISlider::Update(float DeltaTime)
{
    KUIElement::Update(DeltaTime);
    UpdateHandlePosition();
}

void KUISlider::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!bIsVisible || !Canvas)
        return;

    float trackHeight = Height * 0.3f;
    float trackY = PositionY + (Height - trackHeight) * 0.5f;
    
    FColor currentHandleColor = HandleColor;
    if (bIsHovered)
    {
        currentHandleColor = FColor(0.9f, 0.9f, 0.9f, 1.0f);
    }
    if (bIsDragging)
    {
        currentHandleColor = FColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    Canvas->AddQuad(PositionX, trackY, Width, trackHeight, TrackColor);

    float fillWidth = PositionFromValue();
    if (fillWidth > 0)
    {
        Canvas->AddQuad(PositionX, trackY, fillWidth, trackHeight, FillColor);
    }

    Canvas->AddQuad(HandleX, HandleY, HandleWidth, HandleHeight, currentHandleColor);
}

void KUISlider::OnMouseDown(float X, float Y, int Button)
{
    if (Button == 0)
    {
        bIsDragging = true;
        bIsPressed = true;
        
        Value = ValueFromPosition(X);
        Value = std::max(MinValue, std::min(MaxValue, Value));
        UpdateHandlePosition();
        
        if (OnValueChangedCallback)
        {
            OnValueChangedCallback(Value);
        }
    }
}

void KUISlider::OnMouseUp(float X, float Y, int Button)
{
    if (Button == 0)
    {
        bIsDragging = false;
        bIsPressed = false;
    }
}

bool KUISlider::HitTest(float X, float Y) const
{
    return (X >= PositionX && X <= PositionX + Width &&
            Y >= PositionY && Y <= PositionY + Height);
}

void KUISlider::SetValue(float InValue)
{
    Value = std::max(MinValue, std::min(MaxValue, InValue));
    UpdateHandlePosition();
}

void KUISlider::SetHandleSize(float InWidth, float InHeight)
{
    HandleWidth = InWidth;
    HandleHeight = InHeight;
    UpdateHandlePosition();
}

void KUISlider::UpdateHandlePosition()
{
    float handlePosX = PositionFromValue();
    HandleX = PositionX + handlePosX - HandleWidth * 0.5f;
    HandleY = PositionY + (Height - HandleHeight) * 0.5f;
    
    HandleX = std::max(PositionX, std::min(PositionX + Width - HandleWidth, HandleX));
}

float KUISlider::ValueFromPosition(float X) const
{
    float relativeX = X - PositionX;
    float normalizedValue = relativeX / Width;
    return MinValue + normalizedValue * (MaxValue - MinValue);
}

float KUISlider::PositionFromValue() const
{
    float normalizedValue = (Value - MinValue) / (MaxValue - MinValue);
    return normalizedValue * Width;
}
