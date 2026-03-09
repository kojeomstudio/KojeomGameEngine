#pragma once

#include "UIElement.h"

class KUIPanel : public KUIElement
{
public:
    KUIPanel();
    virtual ~KUIPanel();

    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;

    void SetBackgroundColor(const FColor& Color) { BackgroundColor = Color; }
    const FColor& GetBackgroundColor() const { return BackgroundColor; }

    void SetBorderColor(const FColor& Color) { BorderColor = Color; }
    const FColor& GetBorderColor() const { return BorderColor; }

    void SetBorderWidth(float Width) { BorderWidth = Width; }
    float GetBorderWidth() const { return BorderWidth; }

    void SetCornerRadius(float Radius) { CornerRadius = Radius; }
    float GetCornerRadius() const { return CornerRadius; }

private:
    FColor BackgroundColor;
    FColor BorderColor;
    float BorderWidth;
    float CornerRadius;
};
