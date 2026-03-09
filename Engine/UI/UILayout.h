#pragma once

#include "UIElement.h"
#include <vector>

class KUILayout : public KUIElement
{
public:
    KUILayout();
    virtual ~KUILayout();

    virtual void Update(float DeltaTime) override;
    virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas) override;

    virtual void AddChild(std::shared_ptr<KUIElement> Child);
    virtual void RemoveChild(std::shared_ptr<KUIElement> Child);
    virtual void ClearChildren();
    
    size_t GetChildCount() const { return Children.size(); }
    std::shared_ptr<KUIElement> GetChild(size_t Index) const;
    
    void SetSpacing(float InSpacing) { Spacing = InSpacing; UpdateLayout(); }
    float GetSpacing() const { return Spacing; }
    
    void SetPadding(const FPadding& InPadding) { Padding = InPadding; UpdateLayout(); }
    FPadding GetPadding() const { return Padding; }
    
    void SetChildAlignment(EUIHorizontalAlignment InHAlign, EUIVerticalAlignment InVAlign);
    
    virtual void UpdateLayout() {}

protected:
    std::vector<std::shared_ptr<KUIElement>> Children;
    float Spacing;
    EUIHorizontalAlignment ChildHAlignment;
    EUIVerticalAlignment ChildVAlignment;
};
