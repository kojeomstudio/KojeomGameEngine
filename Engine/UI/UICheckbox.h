#pragma once

#include "UIElement.h"
#include <functional>

class KUICheckbox : public KUIElement
{
public:
    KUICheckbox();
    virtual ~KUICheckbox();

    virtual void Update(float DeltaTime) override;
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;
    
    virtual void OnMouseDown(float X, float Y, int Button) override;
    virtual void OnClick() override;
    virtual bool HitTest(float X, float Y) const override;

    void SetChecked(bool bChecked);
    bool IsChecked() const { return bIsChecked; }
    
    void SetBoxSize(float Size) { BoxSize = Size; }
    float GetBoxSize() const { return BoxSize; }
    
    void SetBoxColor(const FColor& Color) { BoxColor = Color; }
    void SetCheckColor(const FColor& Color) { CheckColor = Color; }
    void SetHoverColor(const FColor& Color) { HoverColor = Color; }
    
    using CheckboxCallback = std::function<void(bool)>;
    void SetOnValueChangedCallback(CheckboxCallback Callback) { OnValueChangedCallback = Callback; }

private:
    bool bIsChecked;
    float BoxSize;
    
    FColor BoxColor;
    FColor CheckColor;
    FColor HoverColor;
    
    CheckboxCallback OnValueChangedCallback;
};
