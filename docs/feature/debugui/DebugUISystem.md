# Debug UI System

## Overview

KojeomGameEngine provides an ImGui-based debug overlay system for runtime diagnostics and profiling. The system is located in the `KojeomEngine::KDebugUI` namespace and is controlled by the `USE_IMGUI` preprocessor define.

Source files: `Engine/DebugUI/DebugUI.h`, `Engine/DebugUI/DebugUI.cpp`

---

## Architecture

`KDebugUI` is a singleton class that wraps ImGui for in-application debug visualization. It provides:

- FPS and frame statistics overlay
- Main menu bar with view toggles
- Registerable custom debug panels
- Windows message integration for ImGui input

---

## KDebugUI Class

**File:** `Engine/DebugUI/DebugUI.h`
**Namespace:** `KojeomEngine`

### Singleton Access

```cpp
static KDebugUI* GetInstance();
```

### Lifecycle

```cpp
HRESULT Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
void Cleanup();
```

Initialization creates the ImGui context, configures styling (dark theme with rounded corners), and initializes the Win32 and DX11 ImGui backends. The `USE_IMGUI` preprocessor guard must be defined for ImGui functionality to be active.

### Frame Rendering

```cpp
void BeginFrame();
void EndFrame();
void Render(ID3D11DeviceContext* context);
```

- `BeginFrame()` calls `NewFrame()` internally to prepare ImGui for input and layout
- `Render()` renders the main menu bar, stats overlay, and all registered debug panels
- `EndFrame()` is a placeholder for post-render cleanup

### Visibility Control

```cpp
bool IsVisible() const;
void SetVisible(bool visible);
void ToggleVisible();
```

### Windows Message Handling

```cpp
bool HandleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
```

Routes Windows messages to ImGui for input processing. Returns `true` if the message was consumed. Must be called from the application's WndProc.

### Debug Panels

```cpp
void RegisterDebugPanel(const std::string& name, std::function<void()> renderCallback);
void UnregisterDebugPanel(const std::string& name);
```

Register custom debug panels with a name and render callback. Panels are rendered as ImGui windows each frame. Example:

```cpp
KDebugUI::GetInstance()->RegisterDebugPanel("Lighting", []() {
    ImGui::Text("Directional Light Intensity: %.2f", intensity);
    ImGui::SliderFloat("Ambient", &ambientStrength, 0.0f, 1.0f);
});
```

### Frame Statistics

```cpp
struct FFrameStats {
    float FrameTime;
    float FPS;
    UINT DrawCalls;
    UINT TriangleCount;
    UINT VertexCount;
};

void SetFrameStats(const FFrameStats& stats);
void SetRenderStats(UINT drawCalls, UINT triangles, UINT vertices);
const FFrameStats& GetFrameStats() const;
```

Stats overlay displays FPS, frame time (ms), draw calls, triangle count, and vertex count.

---

## Preprocessor Configuration

All ImGui-dependent code is guarded by `USE_IMGUI`:

```cpp
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#endif
```

When `USE_IMGUI` is not defined, `KDebugUI` initializes successfully but all rendering and input handling are no-ops. This allows the engine to build without ImGui as a dependency.

---

## Integration with KEngine

`KDebugUI::GetInstance()` is called by the engine during initialization and rendering. The engine:

1. Calls `Initialize()` with the D3D11 device and context
2. Calls `BeginFrame()` at the start of each frame
3. Updates frame statistics via `SetFrameStats()` or `SetRenderStats()`
4. Calls `Render()` after the main scene render
5. Calls `HandleWndProc()` from the WndProc message handler

---

## ImGui Configuration

The debug UI uses the following ImGui settings:

- **Style:** Dark theme (`ImGui::StyleColorsDark()`)
- **Window rounding:** 4.0px
- **Frame rounding:** 2.0px
- **Grab rounding:** 2.0px
- **Config flags:** Keyboard navigation enabled, Docking enabled
