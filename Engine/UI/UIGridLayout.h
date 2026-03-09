#pragma once

#include "UILayout.h"

class KUIGridLayout : public KUILayout
{
public:
    KUIGridLayout();
    virtual ~KUIGridLayout();

    virtual void UpdateLayout() override;
    
    void SetColumns(int InColumns);
    int GetColumns() const { return Columns; }
    
    void SetCellSize(float Width, float Height);
    void GetCellSize(float& OutWidth, float& OutHeight) const { OutWidth = CellWidth; OutHeight = CellHeight; }
    
    void SetFixedCellSize(bool bFixed) { bFixedCellSize = bFixed; UpdateLayout(); }
    bool IsFixedCellSize() const { return bFixedCellSize; }

private:
    int Columns;
    float CellWidth;
    float CellHeight;
    bool bFixedCellSize;
};
