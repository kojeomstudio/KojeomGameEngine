#pragma once

#include "UIPanel.h"
#include "UIText.h"
#include <memory>

class KUIButton : public KUIElement
{
public:
    KUIButton();
    virtual ~KUIButton();

    virtual void Update(float DeltaTime) override;
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;

    virtual void OnHoverEnter() override;
    virtual void OnHoverLeave() override;
    virtual void OnMouseDown(float X, float Y, int Button) override;
    virtual void OnMouseUp(float X, float Y, int Button) override;

    void SetText(const std::wstring& InText);
    const std::wstring& GetText() const;

    void SetNormalColor(const FColor& Color) { NormalColor = Color; }
    void SetHoveredColor(const FColor& Color) { HoveredColor = Color; }
    void SetPressedColor(const FColor& Color) { PressedColor = Color; }
    void SetDisabledColor(const FColor& Color) { DisabledColor = Color; }

    void SetTextColor(const FColor& Color);
    void SetFontSize(float Size);

    std::shared_ptr<KUIText> GetTextElement() const { return TextElement; }

private:
    FColor GetCurrentColor() const;

    std::shared_ptr<KUIPanel> BackgroundPanel;
    std::shared_ptr<KUIText> TextElement;

    FColor NormalColor;
    FColor HoveredColor;
    FColor PressedColor;
    FColor DisabledColor;
};
