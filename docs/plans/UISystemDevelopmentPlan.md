# UI System Development Plan

## Document Information

- **Created**: 2026-03-08
- **Author**: AI Agent
- **Status**: Completed
- **Priority**: High
- **Base Commit**: Latest
- **Completion Date**: 2026-03-08

## Overview

This document outlines the development plan for the UI System in KojeomGameEngine. The UI System provides a complete 2D user interface framework for game HUDs, menus, and editor interfaces.

## Architecture

### Core Components

```
UI/
├── UITypes.h           - Type definitions (FColor, FRect, FPadding, etc.)
├── UIFont.h/cpp        - Font loading and text rendering
├── UICanvas.h/cpp      - UI container and rendering system
├── UIElement.h/cpp     - Base UI element class
├── UIPanel.h/cpp       - Background panel element
├── UIText.h/cpp        - Text display element
├── UIButton.h/cpp      - Interactive button element
└── UIImage.h/cpp       - Image/texture display element
```

### Class Hierarchy

```
KUIElement (base)
├── KUIPanel
├── KUIText
├── KUIButton
│   └── Contains KUIPanel + KUIText
└── KUIImage
```

## Implementation Details

### Phase 1: Core UI Types (Completed)

**File**: `Engine/UI/UITypes.h`

**Types**:
- `EUIAnchor` - Element anchoring (TopLeft, Center, Stretch, etc.)
- `EUIHorizontalAlignment` - Horizontal alignment options
- `EUIVerticalAlignment` - Vertical alignment options
- `EUIEventType` - UI event types (Click, Hover, Focus, etc.)
- `FColor` - RGBA color with predefined constants
- `FRect` - Rectangle with position and size
- `FPadding` - Padding for layouts
- `FUIVertex` - UI vertex structure for rendering
- `FFontChar` - Font character metrics
- `FFontPage` - Font texture page

### Phase 2: Font System (Completed)

**File**: `Engine/UI/UIFont.h/cpp`

**Features**:
- BMFont format loading
- Procedural fallback font generation
- Text rendering to vertex/index buffers
- Text width and line height calculation
- Unicode character support

**API**:
```cpp
class KUIFont {
    HRESULT Initialize(ID3D11Device* Device, const std::wstring& FontPath);
    void RenderText(ID3D11DeviceContext* Context,
                    const std::wstring& Text,
                    float X, float Y,
                    float Scale,
                    const FColor& Color,
                    std::vector<FUIVertex>& OutVertices,
                    std::vector<uint32>& OutIndices);
    float GetTextWidth(const std::wstring& Text, float Scale) const;
    float GetLineHeight(float Scale) const;
};
```

### Phase 3: UI Canvas (Completed)

**File**: `Engine/UI/UICanvas.h/cpp`

**Features**:
- Orthographic projection for 2D rendering
- Dynamic vertex/index buffer management
- Shader compilation and management
- Mouse event handling (move, down, up)
- Element hit testing
- Focus management

**API**:
```cpp
class KUICanvas {
    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Update(float DeltaTime);
    void Render(ID3D11DeviceContext* Context);
    
    void AddElement(std::shared_ptr<KUIElement> Element);
    void RemoveElement(std::shared_ptr<KUIElement> Element);
    
    void OnMouseMove(float X, float Y);
    void OnMouseDown(float X, float Y, int Button);
    void OnMouseUp(float X, float Y, int Button);
    
    KUIElement* GetElementAt(float X, float Y) const;
    void SetFocusedElement(KUIElement* Element);
};
```

### Phase 4: UI Elements (Completed)

**Base Class**: `Engine/UI/UIElement.h/cpp`

**Features**:
- Position and size management
- Anchoring system
- Alignment options
- Pivot point customization
- Visibility and enabled states
- Hit testing
- Event callbacks (OnClick)

**Panel**: `Engine/UI/UIPanel.h/cpp`

**Features**:
- Background color
- Border color and width
- Corner radius (planned)

**Text**: `Engine/UI/UIText.h/cpp`

**Features**:
- Text content
- Font reference
- Color and size
- Drop shadow
- Text alignment
- Word wrap

**Button**: `Engine/UI/UIButton.h/cpp`

**Features**:
- Normal/Hovered/Pressed/Disabled colors
- Click callback
- Contains panel and text elements
- Hover animations

**Image**: `Engine/UI/UIImage.h/cpp`

**Features**:
- Texture display
- Tint color
- UV rect for sprites
- Aspect ratio preservation

### Phase 5: UI Sample (Completed)

**File**: `samples/UI/UISample.cpp`

**Demonstrates**:
- Multiple panels with different colors
- Text labels with various fonts
- Interactive buttons with callbacks
- Click counter
- Status updates

## Technical Details

### Rendering Pipeline

1. **Begin Frame**
   - Set orthographic projection
   - Disable depth test
   - Enable alpha blending

2. **Render Elements**
   - Traverse elements in z-order
   - Generate vertices and indices
   - Update dynamic buffers
   - Draw indexed

3. **End Frame**
   - Restore render states

### Input Handling

1. **Mouse Move**
   - Get element at position
   - Handle hover enter/leave events
   - Track hovered element

2. **Mouse Down**
   - Check if element is interactable
   - Set pressed element
   - Handle pressed event

3. **Mouse Up**
   - Handle released event
   - If same element as pressed, trigger click
   - Update focus

### Event System

```cpp
// Register click callback
button->SetOnClickCallback([]() {
    // Handle button click
});

// Event flow
OnHoverEnter() -> bIsHovered = true
OnMouseDown()  -> bIsPressed = true
OnMouseUp()    -> bIsPressed = false
OnClick()      -> Execute callback
OnHoverLeave() -> bIsHovered = false
```

## File Structure

```
Engine/
└── UI/
    ├── UITypes.h
    ├── UIFont.h
    ├── UIFont.cpp
    ├── UICanvas.h
    ├── UICanvas.cpp
    ├── UIElement.h
    ├── UIElement.cpp
    ├── UIPanel.h
    ├── UIPanel.cpp
    ├── UIText.h
    ├── UIText.cpp
    ├── UIButton.h
    ├── UIButton.cpp
    ├── UIImage.h
    └── UIImage.cpp

samples/
└── UI/
    ├── UISample.cpp
    └── UISample.vcxproj
```

## Dependencies

- DirectX 11 (d3d11.lib, d3dcompiler.lib)
- Engine Core
- Engine Input (for mouse handling)
- Engine Graphics (for texture loading)

## Future Enhancements

1. **Layout System**
   - Vertical/Horizontal layouts
   - Grid layout
   - Flexbox-style layout

2. **More Elements**
   - Slider
   - Checkbox
   - Radio button
   - Dropdown
   - Scroll view

3. **Advanced Features**
   - 9-slice scaling for panels
   - Masking and clipping
   - Animations and tweens
   - Data binding
   - Styles and themes

4. **Editor Integration**
   - Visual UI editor
   - Drag-and-drop layout
   - Property inspector

## Notes

- All coordinates are in screen space (0,0 at top-left)
- Z-order is determined by element addition order
- Focus is managed by the canvas
- Alpha blending uses pre-multiplied alpha
