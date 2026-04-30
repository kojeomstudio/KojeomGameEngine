# UI System

## Overview

KojeomGameEngine provides a canvas-based UI system built on DirectX 11. Elements are rendered as batched textured/quads for efficient GPU-side drawing. The root container is `KUICanvas`, which manages a list of `KUIElement` instances, handles input routing (hover, click, focus, keyboard), and performs batched rendering of all visible UI geometry each frame.

Source files are located under `Engine/UI/`.

---

## Architecture

```
KUICanvas (root, owns D3D11 resources, batches draw calls)
├── KUIElement (base class for all UI elements)
│   ├── KUIPanel
│   ├── KUIButton
│   ├── KUIText
│   ├── KUIImage
│   ├── KUISlider
│   ├── KUICheckbox
│   └── KUILayout (abstract container)
│       ├── KUIHorizontalLayout
│       ├── KUIVerticalLayout
│       └── KUIGridLayout
└── KUIFont (BMFont-style bitmap font renderer)
```

---

## UI Types (`UITypes.h`)

### EUIAnchor

Nine anchor positions plus stretch mode. Determines how an element is positioned relative to its parent or the canvas.

```cpp
enum class EUIAnchor
{
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Stretch
};
```

### EUIHorizontalAlignment / EUIVerticalAlignment

```cpp
enum class EUIHorizontalAlignment { Left, Center, Right, Stretch };
enum class EUIVerticalAlignment   { Top, Center, Bottom, Stretch };
```

### EUIEventType

Events fired by `KUICanvas` during input processing:

```cpp
enum class EUIEventType
{
    Click, HoverEnter, HoverLeave, Pressed, Released, Focus, FocusLost
};
```

### FColor

RGBA float color with factory methods:

```cpp
struct FColor
{
    float R, G, B, A;
    FColor(float r, float g, float b, float a = 1.0f);

    static FColor White();
    static FColor Black();
    static FColor Red();
    static FColor Green();
    static FColor Blue();
    static FColor Yellow();
    static FColor Cyan();
    static FColor Magenta();
    static FColor Transparent();
    static FColor Gray(float brightness = 0.5f);

    XMFLOAT4 ToFloat4() const;
};
```

### FRect

Axis-aligned rectangle with containment and intersection tests:

```cpp
struct FRect
{
    float X, Y, Width, Height;
    bool Contains(float px, float py) const;
    bool Intersects(const FRect& other) const;
};
```

### FPadding

Four-sided padding with convenience constructors:

```cpp
struct FPadding
{
    float Left, Top, Right, Bottom;

    FPadding();                                          // all zero
    FPadding(float uniform);                             // same value on all sides
    FPadding(float horizontal, float vertical);          // L/R and T/B
    FPadding(float left, float top, float right, float bottom);
};
```

### FUIVertex

Vertex structure used for batched UI quad rendering:

```cpp
struct FUIVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
    XMFLOAT4 Color;
};
```

### FFontChar / FFontPage

BMFont glyph data structures used by `KUIFont`:

```cpp
struct FFontChar
{
    wchar_t Character;
    float X, Y;              // position in texture atlas
    float Width, Height;
    float XOffset, YOffset;
    float XAdvance;
    float Page;
};

struct FFontPage
{
    std::wstring TexturePath;
    float Width, Height;
};
```

### UICallback

```cpp
using UICallback = std::function<void()>;
```

---

## KUIElement

**File:** `Engine/UI/UIElement.h`

Base class for all UI elements. Inherits from `std::enable_shared_from_this<KUIElement>` to support safe shared-pointer access.

### Lifecycle

```cpp
virtual void Update(float DeltaTime);
virtual void Render(ID3D11DeviceContext* Context, KUICanvas* Canvas);
```

- `Update` is called every frame by `KUICanvas::Update`.
- `Render` appends draw data (quads, text) to the canvas batch.

### Events

```cpp
virtual void OnHoverEnter();
virtual void OnHoverLeave();
virtual void OnMouseDown(float X, float Y, int Button);
virtual void OnMouseUp(float X, float Y, int Button);
virtual void OnClick();
virtual void OnFocus();
virtual void OnFocusLost();
virtual void OnKeyDown(int KeyCode);
virtual void OnKeyUp(int KeyCode);
```

The canvas dispatches these based on hit-test results and input state. Override in derived classes to implement custom behavior.

### Hit Testing

```cpp
virtual bool HitTest(float X, float Y) const;
```

Default implementation checks whether the point lies within the element's `FRect`. Widgets like `KUISlider` and `KUICheckbox` override this for enlarged or custom hit regions.

### Transform

| Method | Description |
|--------|-------------|
| `SetPosition(float X, float Y)` | Sets top-left position |
| `SetSize(float Width, float Height)` | Sets dimensions |
| `SetRect(const FRect& InRect)` | Sets position and size in one call |
| `SetAnchor(EUIAnchor)` | Anchors to parent edge/center/stretch |
| `SetAlignment(EUIHorizontalAlignment, EUIVerticalAlignment)` | Sets content alignment |
| `SetPivot(float X, float Y)` | Sets pivot point (0.5, 0.5 = center) |

`UpdateTransform()` (protected, virtual) is called after anchor/pivot changes to recalculate the final position.

### State Properties

| Method | Description |
|--------|-------------|
| `SetVisible(bool)` / `IsVisible()` | Controls rendering; invisible elements are skipped |
| `SetEnabled(bool)` / `IsEnabled()` | Disables interaction but still renders |
| `SetInteractable(bool)` / `IsInteractable()` | Controls whether the element receives input |
| `SetName(const std::string&)` / `GetName()` | Logical name for lookup |
| `SetUserData(void*)` / `GetUserData()` | Opaque pointer for game-specific data |
| `SetOnClickCallback(UICallback)` | Assigns a click handler |

### Internal State Flags

```cpp
bool bIsVisible;
bool bIsEnabled;
bool bIsInteractable;
bool bIsHovered;
bool bIsPressed;
bool bIsFocused;
```

---

## UI Widgets

### KUIPanel

**File:** `Engine/UI/UIPanel.h`

A colored rectangle with optional border and rounded corners.

```cpp
class KUIPanel : public KUIElement
{
    void SetBackgroundColor(const FColor& Color);
    void SetBorderColor(const FColor& Color);
    void SetBorderWidth(float Width);
    void SetCornerRadius(float Radius);
};
```

**Example:**

```cpp
auto panel = std::make_shared<KUIPanel>();
panel->SetPosition(50.0f, 50.0f);
panel->SetSize(300.0f, 200.0f);
panel->SetBackgroundColor(FColor(0.1f, 0.1f, 0.1f, 0.8f));
panel->SetBorderColor(FColor::White());
panel->SetBorderWidth(2.0f);
panel->SetCornerRadius(8.0f);
canvas->AddElement(panel);
```

### KUIButton

**File:** `Engine/UI/UIButton.h`

Composite widget consisting of a `KUIPanel` background and a `KUIText` label. Supports four visual states.

```cpp
class KUIButton : public KUIElement
{
    void SetText(const std::wstring& InText);
    void SetNormalColor(const FColor& Color);
    void SetHoveredColor(const FColor& Color);
    void SetPressedColor(const FColor& Color);
    void SetDisabledColor(const FColor& Color);
    void SetTextColor(const FColor& Color);
    void SetFontSize(float Size);
};
```

**Example:**

```cpp
auto button = std::make_shared<KUIButton>();
button->SetPosition(100.0f, 100.0f);
button->SetSize(200.0f, 50.0f);
button->SetText(L"Start Game");
button->SetNormalColor(FColor(0.2f, 0.5f, 0.8f, 1.0f));
button->SetHoveredColor(FColor(0.3f, 0.6f, 0.9f, 1.0f));
button->SetPressedColor(FColor(0.1f, 0.3f, 0.6f, 1.0f));
button->SetTextColor(FColor::White());
button->SetOnClickCallback([]() {
    LOG_INFO("Button clicked!");
});
canvas->AddElement(button);
```

### KUIText

**File:** `Engine/UI/UIText.h`

Renders text using a `KUIFont` bitmap font. Supports drop shadow, alignment, and word wrap.

```cpp
class KUIText : public KUIElement
{
    void SetText(const std::wstring& InText);
    void SetFont(std::shared_ptr<KUIFont> InFont);
    void SetColor(const FColor& InColor);
    void SetFontSize(float Size);
    void SetDropShadow(bool bEnabled);
    void SetDropShadowColor(const FColor& InColor);
    void SetDropShadowOffset(float X, float Y);
    void SetTextAlignment(EUIHorizontalAlignment InAlignment);
    void SetWordWrap(bool bEnabled);
    float GetTextWidth() const;
    float GetTextHeight() const;
};
```

**Example:**

```cpp
auto text = std::make_shared<KUIText>();
text->SetPosition(10.0f, 10.0f);
text->SetText(L"Hello, World!");
text->SetFont(canvas->GetDefaultFont());
text->SetColor(FColor::White());
text->SetFontSize(24.0f);
text->SetDropShadow(true);
text->SetDropShadowColor(FColor(0.0f, 0.0f, 0.0f, 0.5f));
text->SetDropShadowOffset(2.0f, 2.0f);
canvas->AddElement(text);
```

### KUIImage

**File:** `Engine/UI/UIImage.h`

Displays a texture with optional tint and UV sub-rect. Can preserve the original aspect ratio.

```cpp
class KUIImage : public KUIElement
{
    HRESULT Initialize(ID3D11Device* Device, const std::wstring& TexturePath);
    void SetTexture(ID3D11ShaderResourceView* InTextureSRV);
    void SetTint(const FColor& InTint);
    void SetUVRect(const FRect& InUVRect);
    void SetPreserveAspectRatio(bool bPreserve);
    ID3D11ShaderResourceView* GetTextureSRV() const;
};
```

**Example:**

```cpp
auto image = std::make_shared<KUIImage>();
image->Initialize(device, L"Assets/Textures/icon.png");
image->SetPosition(10.0f, 10.0f);
image->SetSize(64.0f, 64.0f);
image->SetTint(FColor(1.0f, 1.0f, 1.0f, 0.8f));
image->SetPreserveAspectRatio(true);
canvas->AddElement(image);

// Use a sub-region of a texture atlas
image->SetUVRect(FRect(0.0f, 0.0f, 0.25f, 0.25f));
```

### KUISlider

**File:** `Engine/UI/UISlider.h`

A horizontal slider with track, fill, and draggable handle. Fires a value-changed callback during interaction.

```cpp
class KUISlider : public KUIElement
{
    void SetValue(float InValue);
    float GetValue() const;
    void SetMinValue(float InMin);
    void SetMaxValue(float InMax);
    void SetHandleSize(float Width, float Height);
    void SetTrackColor(const FColor& Color);
    void SetHandleColor(const FColor& Color);
    void SetFillColor(const FColor& Color);

    using SliderCallback = std::function<void(float)>;
    void SetOnValueChangedCallback(SliderCallback Callback);
};
```

**Example:**

```cpp
auto slider = std::make_shared<KUISlider>();
slider->SetPosition(50.0f, 300.0f);
slider->SetSize(250.0f, 30.0f);
slider->SetMinValue(0.0f);
slider->SetMaxValue(100.0f);
slider->SetValue(50.0f);
slider->SetTrackColor(FColor::Gray(0.3f));
slider->SetHandleColor(FColor::White());
slider->SetFillColor(FColor(0.2f, 0.5f, 0.8f, 1.0f));
slider->SetOnValueChangedCallback([](float value) {
    LOG_INFO("Slider value: %.1f", value);
});
canvas->AddElement(slider);
```

### KUICheckbox

**File:** `Engine/UI/UICheckbox.h`

A toggleable checkbox with box, check mark, and hover highlight. Fires a value-changed callback on toggle.

```cpp
class KUICheckbox : public KUIElement
{
    void SetChecked(bool bChecked);
    bool IsChecked() const;
    void SetBoxSize(float Size);
    void SetBoxColor(const FColor& Color);
    void SetCheckColor(const FColor& Color);
    void SetHoverColor(const FColor& Color);

    using CheckboxCallback = std::function<void(bool)>;
    void SetOnValueChangedCallback(CheckboxCallback Callback);
};
```

**Example:**

```cpp
auto checkbox = std::make_shared<KUICheckbox>();
checkbox->SetPosition(50.0f, 350.0f);
checkbox->SetBoxSize(24.0f);
checkbox->SetBoxColor(FColor::Gray(0.2f));
checkbox->SetCheckColor(FColor(0.2f, 0.8f, 0.2f, 1.0f));
checkbox->SetHoverColor(FColor::Gray(0.4f));
checkbox->SetOnValueChangedCallback([](bool checked) {
    LOG_INFO("Checkbox: %s", checked ? "ON" : "OFF");
});
canvas->AddElement(checkbox);
```

---

## Layout System

### KUILayout

**File:** `Engine/UI/UILayout.h`

Abstract base class that extends `KUIElement` with child management and automatic layout computation.

```cpp
class KUILayout : public KUIElement
{
    void AddChild(std::shared_ptr<KUIElement> Child);
    void RemoveChild(std::shared_ptr<KUIElement> Child);
    void ClearChildren();
    size_t GetChildCount() const;
    std::shared_ptr<KUIElement> GetChild(size_t Index) const;

    void SetSpacing(float InSpacing);
    void SetPadding(const FPadding& InPadding);
    void SetChildAlignment(EUIHorizontalAlignment InHAlign, EUIVerticalAlignment InVAlign);

    virtual void UpdateLayout() {}
};
```

Child alignment is applied within each cell. Spacing is inserted between children. Padding insets the layout region. Call `UpdateLayout()` explicitly after structural changes, or it is called automatically when spacing/padding is modified.

### KUIHorizontalLayout

**File:** `Engine/UI/UIHorizontalLayout.h`

Arranges children in a horizontal row.

```cpp
class KUIHorizontalLayout : public KUILayout
{
    void SetReverseOrder(bool bReverse);
    bool IsReverseOrder() const;
};
```

**Example:**

```cpp
auto hLayout = std::make_shared<KUIHorizontalLayout>();
hLayout->SetPosition(20.0f, 20.0f);
hLayout->SetSize(600.0f, 40.0f);
hLayout->SetSpacing(10.0f);
hLayout->SetPadding(FPadding(10.0f));
hLayout->SetChildAlignment(EUIHorizontalAlignment::Center, EUIVerticalAlignment::Center);

hLayout->AddChild(std::make_shared<KUIButton>());  // Button 1
hLayout->AddChild(std::make_shared<KUIButton>());  // Button 2
hLayout->AddChild(std::make_shared<KUIButton>());  // Button 3

canvas->AddElement(hLayout);
```

### KUIVerticalLayout

**File:** `Engine/UI/UIVerticalLayout.h`

Arranges children in a vertical column.

```cpp
class KUIVerticalLayout : public KUILayout
{
    void SetReverseOrder(bool bReverse);
    bool IsReverseOrder() const;
};
```

**Example:**

```cpp
auto vLayout = std::make_shared<KUIVerticalLayout>();
vLayout->SetPosition(20.0f, 100.0f);
vLayout->SetSize(200.0f, 400.0f);
vLayout->SetSpacing(8.0f);

vLayout->AddChild(std::make_shared<KUIPanel>());   // Row 1
vLayout->AddChild(std::make_shared<KUIText>());     // Row 2
vLayout->AddChild(std::make_shared<KUISlider>());   // Row 3

canvas->AddElement(vLayout);
```

### KUIGridLayout

**File:** `Engine/UI/UIGridLayout.h`

Arranges children in a grid with configurable columns and cell size.

```cpp
class KUIGridLayout : public KUILayout
{
    void SetColumns(int InColumns);
    int GetColumns() const;

    void SetCellSize(float Width, float Height);
    void GetCellSize(float& OutWidth, float& OutHeight) const;

    void SetFixedCellSize(bool bFixed);
    bool IsFixedCellSize() const;
};
```

When `bFixedCellSize` is true, each cell has the exact dimensions specified by `SetCellSize` and the layout grows to accommodate all children. When false, the cells evenly divide the layout's own width/height.

**Example:**

```cpp
auto grid = std::make_shared<KUIGridLayout>();
grid->SetPosition(20.0f, 20.0f);
grid->SetSize(400.0f, 300.0f);
grid->SetColumns(4);
grid->SetSpacing(5.0f);
grid->SetFixedCellSize(false);

for (int i = 0; i < 12; ++i)
{
    auto panel = std::make_shared<KUIPanel>();
    panel->SetBackgroundColor(FColor::Gray(0.2f + i * 0.05f));
    grid->AddChild(panel);
}

canvas->AddElement(grid);
```

---

## KUIFont

**File:** `Engine/UI/UIFont.h`

BMFont-style bitmap font system. Loads a font description file and texture atlas, then renders text by generating quads for each glyph.

```cpp
class KUIFont
{
    HRESULT Initialize(ID3D11Device* Device, const std::wstring& FontPath);

    void RenderText(ID3D11DeviceContext* Context,
                    const std::wstring& Text,
                    float X, float Y,
                    float Scale,
                    const FColor& Color,
                    std::vector<FUIVertex>& OutVertices,
                    std::vector<uint32>& OutIndices);

    float GetTextWidth(const std::wstring& Text, float Scale = 1.0f) const;
    float GetLineHeight(float Scale = 1.0f) const;
};
```

- `Initialize` parses the BMFont file and creates the texture SRV.
- `RenderText` generates vertices and indices for the given string at the specified position, scale, and color. The output is appended to the canvas batch.
- `GetTextWidth` calculates the total advance width of a string (useful for sizing `KUIText` elements).
- `GetLineHeight` returns the scaled line height for vertical layout calculations.

---

## KUICanvas

**File:** `Engine/UI/UICanvas.h`

Root container and renderer for the entire UI. Manages the element list, processes input, maintains focus state, and batches all draw calls into a single GPU submission.

### Initialization

```cpp
auto canvas = std::make_shared<KUICanvas>();
canvas->Initialize(device, windowWidth, windowHeight);
```

Creates all required D3D11 resources: constant buffer, vertex/index buffers, input layout, shaders, blend state, rasterizer state, depth/stencil state, and sampler state.

### Element Management

```cpp
void AddElement(std::shared_ptr<KUIElement> Element);
void RemoveElement(std::shared_ptr<KUIElement> Element);
void ClearElements();
const std::list<std::shared_ptr<KUIElement>>& GetElements() const;
```

Elements are drawn in list order (first added = back, last added = front).

### Input Routing

The canvas receives raw input from the application and dispatches it to the correct element:

```cpp
void OnMouseMove(float X, float Y);
void OnMouseDown(float X, float Y, int Button);
void OnMouseUp(float X, float Y, int Button);
void OnKeyDown(int KeyCode);
void OnKeyUp(int KeyCode);
```

**Hit testing** walks the element list in reverse order (front-to-back). The first element whose `HitTest` returns true receives the event. Non-visible, non-enabled, or non-interactable elements are skipped.

### Focus Management

```cpp
KUIElement* GetFocusedElement() const;
void SetFocusedElement(KUIElement* Element);
```

Only one element can be focused at a time. Keyboard events (`OnKeyDown`, `OnKeyUp`) are routed to the focused element. Calling `SetFocusedElement(nullptr)` clears focus.

### Rendering

```cpp
void Update(float DeltaTime);
void Render(ID3D11DeviceContext* Context);
```

- `Update` iterates all elements and calls their `Update`.
- `Render` iterates all visible elements, calling their `Render` to populate batched vertex/index buffers, then flushes the batch with a single draw call.

### Batched Quad Submission

Elements use these methods to add geometry to the batch:

```cpp
void AddQuad(float X, float Y, float Width, float Height, const FColor& Color);
void AddQuad(float X, float Y, float Width, float Height, const FColor& Color, ID3D11ShaderResourceView* Texture);
void FlushDrawCommands(ID3D11DeviceContext* Context);
```

The canvas batches all quads, then submits them in as few draw calls as possible (batch breaks on texture changes).

### Default Font

```cpp
KUIFont* GetDefaultFont() const;
void SetDefaultFont(std::shared_ptr<KUIFont> Font);
```

The default font is used by `KUIText` elements that do not have a font explicitly set.

---

## Code Examples

### Creating a Basic HUD

```cpp
// Initialize canvas
auto canvas = std::make_shared<KUICanvas>();
canvas->Initialize(device, 1280, 720);

// Load default font
auto font = std::make_shared<KUIFont>();
font->Initialize(device, L"Assets/Fonts/Roboto.fnt");
canvas->SetDefaultFont(font);

// Title bar
auto titleBar = std::make_shared<KUIPanel>();
titleBar->SetAnchor(EUIAnchor::Stretch);
titleBar->SetSize(1280.0f, 60.0f);
titleBar->SetBackgroundColor(FColor(0.0f, 0.0f, 0.0f, 0.7f));
canvas->AddElement(titleBar);

// Title text
auto titleText = std::make_shared<KUIText>();
titleText->SetPosition(20.0f, 15.0f);
titleText->SetText(L"KojeomGameEngine");
titleText->SetColor(FColor::White());
titleText->SetFontSize(28.0f);
canvas->AddElement(titleText);

// FPS counter (bottom-left)
auto fpsText = std::make_shared<KUIText>();
fpsText->SetAnchor(EUIAnchor::BottomLeft);
fpsText->SetPosition(10.0f, -40.0f);
fpsText->SetColor(FColor::Green());
fpsText->SetFontSize(18.0f);
canvas->AddElement(fpsText);

// Main loop
while (bIsRunning)
{
    canvas->Update(deltaTime);
    canvas->Render(context);

    // Update FPS text
    fpsText->SetText(std::to_wstring(currentFPS));
}
```

### Setting Up a Vertical Menu

```cpp
auto menu = std::make_shared<KUIVerticalLayout>();
menu->SetPosition(windowWidth / 2.0f - 100.0f, windowHeight / 2.0f - 120.0f);
menu->SetSize(200.0f, 240.0f);
menu->SetSpacing(10.0f);
menu->SetPadding(FPadding(5.0f));
menu->SetChildAlignment(EUIHorizontalAlignment::Stretch, EUIVerticalAlignment::Stretch);

auto addButton = [&](const std::wstring& label, UICallback callback) {
    auto btn = std::make_shared<KUIButton>();
    btn->SetSize(190.0f, 50.0f);
    btn->SetText(label);
    btn->SetOnClickCallback(callback);
    menu->AddChild(btn);
};

addButton(L"New Game", []() { /* start game */ });
addButton(L"Settings", []() { /* open settings */ });
addButton(L"Quit", []() { /* exit */ });

canvas->AddElement(menu);
```

### Handling Slider and Checkbox Input

```cpp
auto slider = std::make_shared<KUISlider>();
slider->SetPosition(20.0f, 20.0f);
slider->SetSize(300.0f, 24.0f);
slider->SetMinValue(0.0f);
slider->SetMaxValue(1.0f);
slider->SetValue(0.5f);
slider->SetOnValueChangedCallback([](float value) {
    LOG_INFO("Volume: %.0f%%", value * 100.0f);
});
canvas->AddElement(slider);

auto checkbox = std::make_shared<KUICheckbox>();
checkbox->SetPosition(20.0f, 60.0f);
checkbox->SetBoxSize(20.0f);
checkbox->SetOnValueChangedCallback([](bool checked) {
    LOG_INFO("Fullscreen: %s", checked ? "enabled" : "disabled");
});
canvas->AddElement(checkbox);

// Wire input from the game loop
void OnMouseMove(float x, float y) { canvas->OnMouseMove(x, y); }
void OnMouseDown(float x, float y, int button) { canvas->OnMouseDown(x, y, button); }
void OnMouseUp(float x, float y, int button) { canvas->OnMouseUp(x, y, button); }
```

### Using Anchors for Responsive Layout

```cpp
// Full-screen background panel
auto bg = std::make_shared<KUIPanel>();
bg->SetAnchor(EUIAnchor::Stretch);
bg->SetBackgroundColor(FColor(0.0f, 0.0f, 0.0f, 0.5f));
canvas->AddElement(bg);

// Centered dialog
auto dialog = std::make_shared<KUIPanel>();
dialog->SetAnchor(EUIAnchor::Center);
dialog->SetSize(400.0f, 300.0f);
dialog->SetBackgroundColor(FColor(0.15f, 0.15f, 0.15f, 0.95f));
dialog->SetCornerRadius(10.0f);
canvas->AddElement(dialog);

// Bottom-right stats
auto stats = std::make_shared<KUIText>();
stats->SetAnchor(EUIAnchor::BottomRight);
stats->SetPosition(-20.0f, -20.0f);
stats->SetText(L"Score: 0");
stats->SetTextAlignment(EUIHorizontalAlignment::Right);
canvas->AddElement(stats);
```
