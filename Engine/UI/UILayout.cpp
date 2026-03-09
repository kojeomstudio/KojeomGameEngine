#include "UILayout.h"
#include "UICanvas.h"

KUILayout::KUILayout()
    : Spacing(0.0f)
    , ChildHAlignment(EUIHorizontalAlignment::Left)
    , ChildVAlignment(EUIVerticalAlignment::Top)
{
}

KUILayout::~KUILayout()
{
    Children.clear();
}

void KUILayout::Update(float DeltaTime)
{
    KUIElement::Update(DeltaTime);

    for (auto& Child : Children)
    {
        if (Child && Child->IsVisible())
        {
            Child->Update(DeltaTime);
        }
    }
}

void KUILayout::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!bIsVisible)
        return;

    for (auto& Child : Children)
    {
        if (Child && Child->IsVisible())
        {
            Child->Render(Context, Canvas);
        }
    }
}

void KUILayout::AddChild(std::shared_ptr<KUIElement> Child)
{
    if (Child)
    {
        Children.push_back(Child);
        UpdateLayout();
    }
}

void KUILayout::RemoveChild(std::shared_ptr<KUIElement> Child)
{
    if (!Child)
        return;

    for (auto it = Children.begin(); it != Children.end(); ++it)
    {
        if (*it == Child)
        {
            Children.erase(it);
            UpdateLayout();
            return;
        }
    }
}

void KUILayout::ClearChildren()
{
    Children.clear();
    UpdateLayout();
}

std::shared_ptr<KUIElement> KUILayout::GetChild(size_t Index) const
{
    if (Index < Children.size())
    {
        return Children[Index];
    }
    return nullptr;
}

void KUILayout::SetChildAlignment(EUIHorizontalAlignment InHAlign, EUIVerticalAlignment InVAlign)
{
    ChildHAlignment = InHAlign;
    ChildVAlignment = InVAlign;
    UpdateLayout();
}
