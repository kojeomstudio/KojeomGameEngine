#pragma once

#include "UIElement.h"
#include "UIPanel.h"
#include <functional>

class KUISlider : public KUIElement
{
public:
    KUISlider();
    virtual ~KUISlider();

    virtual void Update(float DeltaTime) override;
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;
    
    virtual void OnMouseDown(float X, float Y, int Button) override;
    virtual void OnMouseUp(float X, float Y, int Button) override;
    virtual bool HitTest(float X, float Y) const override;

    void SetValue(float InValue);
    float GetValue() const { return Value; }
    
    void SetMinValue(float InMin) { MinValue = InMin; UpdateHandlePosition(); }
    float GetMinValue() const { return MinValue; }
    
    void SetMaxValue(float InMax) { MaxValue = InMax; UpdateHandlePosition(); }
    float GetMaxValue() const { return MaxValue; }
    
    void SetHandleSize(float Width, float Height);
    
    void SetTrackColor(const FColor& Color) { TrackColor = Color; }
    void SetHandleColor(const FColor& Color) { HandleColor = Color; }
    void SetFillColor(const FColor& Color) { FillColor = Color; }
    
    using SliderCallback = std::function<void(float)>;
    void SetOnValueChangedCallback(SliderCallback Callback) { OnValueChangedCallback = Callback; }

private:
    void UpdateHandlePosition();
    float ValueFromPosition(float X) const;
    float PositionFromValue() const;

    float Value;
    float MinValue;
    float MaxValue;
    float HandleWidth;
    float HandleHeight;
    float HandleX;
    float HandleY;
    
    FColor TrackColor;
    FColor HandleColor;
    FColor FillColor;
    
    bool bIsDragging;
    
    SliderCallback OnValueChangedCallback;
};
