#pragma once

#include "UITypes.h"
#include "../Utils/Common.h"
#include <d3d11.h>
#include <memory>
#include <string>

class KUICanvas;

class KUIElement : public std::enable_shared_from_this<KUIElement>
{
public:
    KUIElement();
    virtual ~KUIElement();

    virtual void Update(float DeltaTime);
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas);

    virtual void OnHoverEnter();
    virtual void OnHoverLeave();
    virtual void OnMouseDown(float X, float Y, int Button);
    virtual void OnMouseUp(float X, float Y, int Button);
    virtual void OnClick();
    virtual void OnFocus();
    virtual void OnFocusLost();
    virtual void OnKeyDown(int KeyCode);
    virtual void OnKeyUp(int KeyCode);

    virtual bool HitTest(float X, float Y) const;

    void SetPosition(float X, float Y);
    void GetPosition(float& OutX, float& OutY) const { OutX = PositionX; OutY = PositionY; }

    void SetSize(float Width, float Height);
    void GetSize(float& OutWidth, float& OutHeight) const { OutWidth = Width; OutHeight = Height; }

    void SetRect(const FRect& InRect);
    FRect GetRect() const { return FRect(PositionX, PositionY, Width, Height); }

    void SetAnchor(EUIAnchor InAnchor) { Anchor = InAnchor; UpdateTransform(); }
    EUIAnchor GetAnchor() const { return Anchor; }

    void SetAlignment(EUIHorizontalAlignment InHAlign, EUIVerticalAlignment InVAlign);
    void SetPivot(float X, float Y) { PivotX = X; PivotY = Y; }

    void SetVisible(bool bVisible) { bIsVisible = bVisible; }
    bool IsVisible() const { return bIsVisible; }

    void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }
    bool IsEnabled() const { return bIsEnabled; }

    void SetInteractable(bool bInteractable) { bIsInteractable = bInteractable; }
    bool IsInteractable() const { return bIsInteractable; }

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void SetUserData(void* Data) { UserData = Data; }
    void* GetUserData() const { return UserData; }

    void SetOnClickCallback(UICallback Callback) { OnClickCallback = Callback; }

protected:
    virtual void UpdateTransform();

    std::string Name;
    float PositionX, PositionY;
    float Width, Height;
    float PivotX, PivotY;
    EUIAnchor Anchor;
    EUIHorizontalAlignment HorizontalAlignment;
    EUIVerticalAlignment VerticalAlignment;
    FPadding Padding;
    FPadding Margin;

    bool bIsVisible;
    bool bIsEnabled;
    bool bIsInteractable;
    bool bIsHovered;
    bool bIsPressed;
    bool bIsFocused;

    void* UserData;
    UICallback OnClickCallback;
};
