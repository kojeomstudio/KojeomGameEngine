#pragma once

#include "UILayout.h"

class KUIHorizontalLayout : public KUILayout
{
public:
    KUIHorizontalLayout();
    virtual ~KUIHorizontalLayout();

    virtual void UpdateLayout() override;
    
    void SetReverseOrder(bool bReverse) { bReverseOrder = bReverse; UpdateLayout(); }
    bool IsReverseOrder() const { return bReverseOrder; }

private:
    bool bReverseOrder;
};
