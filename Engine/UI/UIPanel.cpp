#include "UIPanel.h"
#include "UICanvas.h"
#include <vector>

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

    std::vector<FUIVertex> vertices;
    std::vector<uint32> indices;

    XMFLOAT4 color = BackgroundColor.ToFloat4();

    float x = PositionX;
    float y = PositionY;
    float w = Width;
    float h = Height;

    uint32 baseIndex = 0;

    vertices.push_back(FUIVertex(XMFLOAT3(x, y, 0), XMFLOAT2(0, 0), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x + w, y, 0), XMFLOAT2(1, 0), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x + w, y + h, 0), XMFLOAT2(1, 1), color));
    vertices.push_back(FUIVertex(XMFLOAT3(x, y + h, 0), XMFLOAT2(0, 1), color));

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    UINT stride = sizeof(FUIVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
}
