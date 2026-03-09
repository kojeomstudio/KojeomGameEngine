#include "UIVerticalLayout.h"

KUIVerticalLayout::KUIVerticalLayout()
    : bReverseOrder(false)
{
}

KUIVerticalLayout::~KUIVerticalLayout()
{
}

void KUIVerticalLayout::UpdateLayout()
{
    float currentY = PositionY + Padding.Top;
    
    float layoutWidth = Width - Padding.Left - Padding.Right;
    
    size_t startIndex = bReverseOrder ? Children.size() - 1 : 0;
    int step = bReverseOrder ? -1 : 1;
    
    for (size_t i = 0; i < Children.size(); ++i)
    {
        size_t childIndex = bReverseOrder ? (Children.size() - 1 - i) : i;
        auto& child = Children[childIndex];
        
        if (!child)
            continue;
            
        float childWidth, childHeight;
        child->GetSize(childWidth, childHeight);
        
        float childX = PositionX + Padding.Left;
        
        switch (ChildHAlignment)
        {
        case EUIHorizontalAlignment::Left:
            childX = PositionX + Padding.Left;
            break;
        case EUIHorizontalAlignment::Center:
            childX = PositionX + Padding.Left + (layoutWidth - childWidth) * 0.5f;
            break;
        case EUIHorizontalAlignment::Right:
            childX = PositionX + Width - Padding.Right - childWidth;
            break;
        }
        
        child->SetPosition(childX, currentY);
        currentY += childHeight + Spacing;
    }
}
