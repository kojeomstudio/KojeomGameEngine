#include "UIGridLayout.h"

KUIGridLayout::KUIGridLayout()
    : Columns(3)
    , CellWidth(100.0f)
    , CellHeight(100.0f)
    , bFixedCellSize(true)
{
}

KUIGridLayout::~KUIGridLayout()
{
}

void KUIGridLayout::SetColumns(int InColumns)
{
    Columns = InColumns > 0 ? InColumns : 1;
    UpdateLayout();
}

void KUIGridLayout::SetCellSize(float Width, float Height)
{
    CellWidth = Width;
    CellHeight = Height;
    UpdateLayout();
}

void KUIGridLayout::UpdateLayout()
{
    if (Columns <= 0)
        return;
    
    float startX = PositionX + Padding.Left;
    float startY = PositionY + Padding.Top;
    
    float currentX = startX;
    float currentY = startY;
    int currentColumn = 0;
    
    for (auto& child : Children)
    {
        if (!child)
            continue;
        
        float childWidth = bFixedCellSize ? CellWidth : CellWidth;
        float childHeight = bFixedCellSize ? CellHeight : CellHeight;
        
        if (!bFixedCellSize)
        {
            child->GetSize(childWidth, childHeight);
        }
        
        float posX = currentX;
        float posY = currentY;
        
        switch (ChildHAlignment)
        {
        case EUIHorizontalAlignment::Center:
            posX = currentX + (CellWidth - childWidth) * 0.5f;
            break;
        case EUIHorizontalAlignment::Right:
            posX = currentX + CellWidth - childWidth;
            break;
        default:
            break;
        }
        
        switch (ChildVAlignment)
        {
        case EUIVerticalAlignment::Center:
            posY = currentY + (CellHeight - childHeight) * 0.5f;
            break;
        case EUIVerticalAlignment::Bottom:
            posY = currentY + CellHeight - childHeight;
            break;
        default:
            break;
        }
        
        child->SetPosition(posX, posY);
        
        if (bFixedCellSize)
        {
            child->SetSize(CellWidth, CellHeight);
        }
        
        currentColumn++;
        currentX += CellWidth + Spacing;
        
        if (currentColumn >= Columns)
        {
            currentColumn = 0;
            currentX = startX;
            currentY += CellHeight + Spacing;
        }
    }
}
