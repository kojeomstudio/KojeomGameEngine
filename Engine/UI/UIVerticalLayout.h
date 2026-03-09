#pragma once

#include "UILayout.h"

class KUIVerticalLayout : public KUILayout
{
public:
    KUIVerticalLayout();
    virtual ~KUIVerticalLayout();

    virtual void UpdateLayout() override;
    
    void SetReverseOrder(bool bReverse) { bReverseOrder = bReverse; UpdateLayout(); }
    bool IsReverseOrder() const { return bReverseOrder; }

private:
    bool bReverseOrder;
};
