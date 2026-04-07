#include "UIPanel.h"
#include "UICanvas.h"

KUIPanel::KUIPanel()
    : BackgroundColor(FColor::Gray(0.2f))
    , BorderColor(FColor::White())
    , BorderWidth(0.0f)
    , CornerRadius(0.0f)
{
}

KUIPanel::~KUIPanel()
{
}

void KUIPanel::Render(ID3D11DeviceContext* Context, KUICanvas* Canvas)
{
    if (!IsVisible()) return;

    Canvas->AddQuad(PositionX, PositionY, Width, Height, BackgroundColor);
}
