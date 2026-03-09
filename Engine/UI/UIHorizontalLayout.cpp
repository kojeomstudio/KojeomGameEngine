#include "UIHorizontalLayout.h"

KUIHorizontalLayout::KUIHorizontalLayout()
    : bReverseOrder(false)
{
}

KUIHorizontalLayout::~KUIHorizontalLayout()
{
}

void KUIHorizontalLayout::UpdateLayout()
{
    float currentX = PositionX + Padding.Left;
    
    float layoutHeight = Height - Padding.Top - Padding.Bottom;
    
    for (size_t i = 0; i < Children.size(); ++i)
    {
        size_t childIndex = bReverseOrder ? (Children.size() - 1 - i) : i;
        auto& child = Children[childIndex];
        
        if (!child)
            continue;
            
        float childWidth, childHeight;
        child->GetSize(childWidth, childHeight);
        
        float childY = PositionY + Padding.Top;
        
        switch (ChildVAlignment)
        {
        case EUIVerticalAlignment::Top:
            childY = PositionY + Padding.Top;
            break;
        case EUIVerticalAlignment::Center:
            childY = PositionY + Padding.Top + (layoutHeight - childHeight) * 0.5f;
            break;
        case EUIVerticalAlignment::Bottom:
            childY = PositionY + Height - Padding.Bottom - childHeight;
            break;
        }
        
        child->SetPosition(currentX, childY);
        currentX += childWidth + Spacing;
    }
}
