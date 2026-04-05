# Input System

The KojeomGameEngine input system provides keyboard, mouse, and raw input support through the `KInputManager` singleton. It includes key/mouse state polling, edge detection (just pressed / just released), action mapping with modifier keys, and an event callback system.

## Overview

| File | Description |
|------|-------------|
| `Engine/Input/InputTypes.h` | Enums (`EKeyCode`, `EMouseButton`, `EInputState`) and structs (`FMouseState`, `FKeyState`, `FInputAction`) |
| `Engine/Input/InputManager.h` | `KInputManager` class declaration |
| `Engine/Input/InputManager.cpp` | `KInputManager` implementation |

The input manager is a **singleton** accessed via `KInputManager::GetInstance()`. It must be initialized with a valid Win32 `HWND` before use:

```cpp
auto* inputMgr = KInputManager::GetInstance();
inputMgr->Initialize(hwnd);
```

`Initialize` registers a raw mouse input device and captures the initial cursor position.

Each frame, call `Update()` to advance the input state. This copies the previous frame's key states (enabling edge detection) and resets per-frame mouse deltas:

```cpp
inputMgr->Update();
```

## EKeyCode

Defined in `Engine/Input/InputTypes.h:12`. All values are `uint32_t`.

| Category | Keys |
|----------|------|
| **Function** | `F1` (`0x70`) through `F12` (`0x7B`) |
| **Number row** | `Num0` (`0x30`) through `Num9` (`0x39`) |
| **Letters** | `A` (`0x41`) through `Z` (`0x5A`) |
| **Arrows** | `Left`, `Right`, `Up`, `Down` (Win32 `VK_*` values) |
| **Navigation** | `Space`, `Enter`, `Tab`, `Escape`, `Backspace`, `Delete`, `Insert`, `Home`, `End`, `PageUp`, `PageDown` |
| **Modifiers** | `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl`, `LeftAlt`, `RightAlt` |
| **Numpad** | `Numpad0`-`Numpad9`, `NumpadAdd`, `NumpadSubtract`, `NumpadMultiply`, `NumpadDivide`, `NumpadDecimal` |
| **Symbols** | `Minus`, `Plus`, `Comma`, `Period`, `Slash`, `Semicolon`, `Quote`, `LeftBracket`, `RightBracket`, `Backslash`, `Grave` |
| **Mouse buttons** | `MouseLeft` (`0x100`), `MouseRight` (`0x101`), `MouseMiddle` (`0x102`), `MouseXButton1` (`0x103`), `MouseXButton2` (`0x104`) |

Mouse buttons have virtual key codes starting at `0x100`. These are used by the action mapping and callback system so mouse and keyboard inputs share a unified key code space.

## EMouseButton

Defined in `Engine/Input/InputTypes.h:137`. Bitfield values for testing button state with bitwise AND:

| Button | Value |
|--------|-------|
| `None` | `0` |
| `Left` | `1` |
| `Right` | `2` |
| `Middle` | `4` |
| `XButton1` | `8` |
| `XButton2` | `16` |

## FMouseState

Defined in `Engine/Input/InputTypes.h:161`:

```cpp
struct FMouseState
{
    int32_t X = 0;
    int32_t Y = 0;
    int32_t DeltaX = 0;
    int32_t DeltaY = 0;
    int32_t WheelDelta = 0;
    uint32_t Buttons = 0;
};
```

- `X`, `Y` — Cursor position in client coordinates.
- `DeltaX`, `DeltaY` — Frame-to-frame movement delta (accumulated from raw input when enabled, or computed from `WM_MOUSEMOVE`).
- `WheelDelta` — Scroll wheel delta for the current frame (reset each frame by `Update()`).
- `Buttons` — Bitfield of currently pressed mouse buttons (`EMouseButton::Button` values ORed together).

## EInputState

Defined in `Engine/Input/InputTypes.h:150`:

| State | Value | Description |
|-------|-------|-------------|
| `Released` | `0` | Key is not pressed |
| `Pressed` | `1` | Key is down this frame |
| `Held` | `2` | Key was down last frame and is still down |
| `JustReleased` | `3` | Key was down last frame and is up this frame |

## FKeyState

Internal per-key state tracked by the manager (`Engine/Input/InputTypes.h:171`):

```cpp
struct FKeyState
{
    bool bIsPressed = false;
    bool bWasPressed = false;
};
```

Edge detection is implemented by comparing `bIsPressed` (current frame) against `bWasPressed` (previous frame, saved at the start of each `Update()` call).

## FInputEvent

Passed to registered input callbacks (`Engine/Input/InputManager.h:12`):

```cpp
struct FInputEvent
{
    uint32_t KeyCode = 0;
    EInputState::State State = EInputState::Released;
    int32_t MouseX = 0;
    int32_t MouseY = 0;
    bool bIsMouseEvent = false;
};
```

## Key Query Methods

All key query methods accept a `uint32_t KeyCode` (any `EKeyCode::Key` value).

| Method | Returns `true` when |
|--------|---------------------|
| `IsKeyDown(KeyCode)` | Key is currently pressed |
| `IsKeyUp(KeyCode)` | Key is currently not pressed |
| `IsKeyHeld(KeyCode)` | Key was pressed last frame **and** is still pressed |
| `IsKeyJustPressed(KeyCode)` | Key is pressed this frame but was not pressed last frame |
| `IsKeyJustReleased(KeyCode)` | Key is not pressed this frame but was pressed last frame |

Mouse buttons can also be queried with these methods using their `EKeyCode::Mouse*` codes.

### Example

```cpp
auto* input = KInputManager::GetInstance();

if (input->IsKeyDown(EKeyCode::W))
{
    camera.MoveForward(speed * deltaTime);
}

if (input->IsKeyJustPressed(EKeyCode::Escape))
{
    PostQuitMessage(0);
}

if (input->IsKeyJustPressed(EKeyCode::MouseLeft))
{
    // First frame of left click
}
```

## Mouse Query Methods

### Button State

| Method | Returns `true` when |
|--------|---------------------|
| `IsMouseButtonDown(Button)` | The mouse button bit is set in current frame |
| `IsMouseButtonUp(Button)` | The mouse button bit is not set in current frame |
| `IsMouseButtonHeld(Button)` | Button was down last frame **and** is still down |

These use the `EMouseButton::Button` bitfield directly (unlike the key query methods which use `EKeyCode`).

### Position and Delta

```cpp
int32_t mouseX, mouseY;
input->GetMousePosition(mouseX, mouseY);

int32_t deltaX, deltaY;
input->GetMouseDelta(deltaX, deltaY);

int32_t wheelDelta = input->GetMouseWheelDelta();
```

- `GetMousePosition` — Current cursor position in window client coordinates.
- `GetMouseDelta` — Movement since last frame (reset by `Update()`).
- `GetMouseWheelDelta` — Scroll wheel delta this frame (reset by `Update()`). Positive = scroll up, negative = scroll down (units of `WHEEL_DELTA`, typically 120).

### Example: FPS Camera

```cpp
if (input->IsKeyJustPressed(EKeyCode::MouseRight))
{
    input->SetCursorClipToWindow(true);
    input->SetMouseVisible(false);
}

if (input->IsKeyJustReleased(EKeyCode::MouseRight))
{
    input->SetCursorClipToWindow(false);
    input->SetMouseVisible(true);
}

if (input->IsMouseButtonDown(EMouseButton::Right))
{
    int32_t deltaX, deltaY;
    input->GetMouseDelta(deltaX, deltaY);

    camera.Yaw += deltaX * sensitivity;
    camera.Pitch -= deltaY * sensitivity;
}
```

## Mouse Control Methods

| Method | Description |
|--------|-------------|
| `SetMousePosition(X, Y)` | Moves the cursor to the given client coordinates. Converts to screen space internally. |
| `SetMouseVisible(bVisible)` | Shows or hides the system cursor via `ShowCursor`. |
| `IsMouseVisible()` | Returns whether the cursor is currently visible. |
| `SetMouseCapture(bCapture)` | Calls `SetCapture` / `ReleaseCapture` to route all mouse input to the engine window. |
| `IsMouseCaptured()` | Returns whether capture is active. |
| `EnableRawInput(bEnable)` | Registers or unregisters a raw mouse input device. Raw input provides accumulator-based deltas (additive) via `ProcessRawMouseInput`. |
| `IsRawInputEnabled()` | Returns whether raw input is currently active. |
| `SetCursorClipToWindow(bClip)` | Restricts the cursor to the window's client rect via `ClipCursor`. Use `false` to release. |
| `IsCursorClipped()` | Returns whether the cursor is currently clipped. |

## Action Mapping

Action mapping abstracts physical keys into named actions, supporting optional modifier keys (e.g., Ctrl+S for "Save").

### RegisterAction

```cpp
void RegisterAction(const std::string& ActionName, uint32_t PrimaryKey, uint32_t ModifierKey = 0);
```

Registers a named action with a primary key and optional modifier. Calling with the same `ActionName` replaces the previous binding.

```cpp
input->RegisterAction("MoveForward", EKeyCode::W);
input->RegisterAction("Sprint", EKeyCode::LeftShift);
input->RegisterAction("Save", EKeyCode::S, EKeyCode::LeftCtrl);
input->RegisterAction("Fire", EKeyCode::MouseLeft);
```

### Query Actions

| Method | Returns `true` when |
|--------|---------------------|
| `IsActionDown(Name)` | All modifier keys are down **and** at least one primary key is currently pressed |
| `IsActionPressed(Name)` | All modifier keys are down **and** at least one primary key was just pressed |
| `IsActionReleased(Name)` | All modifier keys are down **and** at least one primary key was just released |

### FInputAction

```cpp
struct FInputAction
{
    std::string Name;
    std::vector<uint32_t> KeyCodes;
    std::vector<uint32_t> ModifierKeys;
};
```

While `RegisterAction` assigns a single primary key, the underlying `FInputAction` struct supports multiple key bindings via vectors. Action queries iterate `KeyCodes` (OR logic) and `ModifierKeys` (AND logic).

### Example

```cpp
auto* input = KInputManager::GetInstance();

input->RegisterAction("Jump", EKeyCode::Space);
input->RegisterAction("Crouch", EKeyCode::LeftCtrl);
input->RegisterAction("Save", EKeyCode::S, EKeyCode::LeftCtrl);
input->RegisterAction("Shoot", EKeyCode::MouseLeft);
input->RegisterAction("Aim", EKeyCode::MouseRight);

// In game loop:
if (input->IsActionPressed("Jump"))
{
    player.Jump();
}

if (input->IsActionDown("Shoot"))
{
    player.Fire();
}

if (input->IsActionReleased("Aim"))
{
    player.LowerWeapon();
}
```

## Input Callbacks

Register callback functions that fire immediately when a key or mouse button event is processed (not deferred to `Update()`):

```cpp
using InputCallback = std::function<void(const FInputEvent&)>;

void RegisterInputCallback(uint32_t KeyCode, InputCallback Callback);
void UnregisterInputCallback(uint32_t KeyCode);
```

Multiple callbacks can be registered for the same key code. `UnregisterInputCallback` removes **all** callbacks for that key.

```cpp
input->RegisterInputCallback(EKeyCode::F1, [](const FInputEvent& event) {
    LOG_INFO("F1 pressed - toggling debug UI");
});

input->RegisterInputCallback(EKeyCode::MouseLeft, [](const FInputEvent& event) {
    if (event.bIsMouseEvent)
    {
        LOG_INFO("Left click at (%d, %d)", event.MouseX, event.MouseY);
    }
});
```

Callbacks are dispatched inside `ProcessKeyDown`, `ProcessKeyUp`, `ProcessMouseDown`, and `ProcessMouseUp` — i.e., at the time the Win32 message is processed, before `Update()` is called.

## Raw Input

Raw input provides unfiltered, high-frequency mouse delta data. When enabled, `ProcessRawMouseInput(DeltaX, DeltaY)` accumulates deltas into `FMouseState::DeltaX` and `DeltaY` (additive).

The engine window procedure should handle `WM_INPUT` messages and forward them:

```cpp
case WM_INPUT:
{
    UINT dataSize = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
    if (dataSize > 0)
    {
        std::vector<uint8_t> buffer(dataSize);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != (UINT)-1)
        {
            RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
            if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                KInputManager::GetInstance()->ProcessRawMouseInput(
                    raw->data.mouse.lLastX,
                    raw->data.mouse.lLastY
                );
            }
        }
    }
    break;
}
```

Enable raw input at initialization or toggle at runtime:

```cpp
input->EnableRawInput(true);
```

Note: Raw input is enabled by default during `Initialize()`. When enabled, `DeltaX`/`DeltaY` are accumulated from raw input; when disabled, they are computed from `WM_MOUSEMOVE` position differences.

## Flush and Clear

| Method | Description |
|--------|-------------|
| `FlushInput()` | Clears all key states, mouse state, and the event queue. Resets both current and previous mouse state. |
| `ClearState()` | Calls `FlushInput()`. Identical behavior. |

Use these when regaining window focus or resetting input after a scene transition.

## FInputAction Struct

```cpp
struct FInputAction
{
    std::string Name;
    std::vector<uint32_t> KeyCodes;
    std::vector<uint32_t> ModifierKeys;
};
```

- `Name` — Action identifier string used for lookup.
- `KeyCodes` — Primary keys that trigger the action (OR logic — any one matching is sufficient).
- `ModifierKeys` — Keys that must all be held for the action to activate (AND logic).

## Complete Example: Game Loop Input

```cpp
void SetupInput()
{
    auto* input = KInputManager::GetInstance();

    input->RegisterAction("MoveForward",  EKeyCode::W);
    input->RegisterAction("MoveBackward", EKeyCode::S);
    input->RegisterAction("MoveLeft",     EKeyCode::A);
    input->RegisterAction("MoveRight",    EKeyCode::D);
    input->RegisterAction("Jump",         EKeyCode::Space);
    input->RegisterAction("Sprint",       EKeyCode::LeftShift);
    input->RegisterAction("Save",         EKeyCode::S, EKeyCode::LeftCtrl);
    input->RegisterAction("Fire",         EKeyCode::MouseLeft);
    input->RegisterAction("Aim",          EKeyCode::MouseRight);

    input->RegisterInputCallback(EKeyCode::Escape, [](const FInputEvent&) {
        PostQuitMessage(0);
    });
}

void GameLoop(float deltaTime)
{
    auto* input = KInputManager::GetInstance();
    input->Update();

    float moveSpeed = input->IsActionDown("Sprint") ? 600.0f : 300.0f;

    if (input->IsActionDown("MoveForward"))
        player.MoveForward(moveSpeed * deltaTime);
    if (input->IsActionDown("MoveBackward"))
        player.MoveForward(-moveSpeed * deltaTime);
    if (input->IsActionDown("MoveLeft"))
        player.MoveRight(-moveSpeed * deltaTime);
    if (input->IsActionDown("MoveRight"))
        player.MoveRight(moveSpeed * deltaTime);

    if (input->IsActionPressed("Jump") && player.IsGrounded())
        player.Jump();

    if (input->IsActionDown("Aim"))
    {
        input->SetCursorClipToWindow(true);
        input->SetMouseVisible(false);

        int32_t deltaX, deltaY;
        input->GetMouseDelta(deltaX, deltaY);
        camera.Yaw   += deltaX * 0.1f;
        camera.Pitch -= deltaY * 0.1f;
    }
    else
    {
        input->SetCursorClipToWindow(false);
        input->SetMouseVisible(true);
    }

    if (input->IsActionDown("Fire"))
        weapon.Fire();

    int32_t wheelDelta = input->GetMouseWheelDelta();
    if (wheelDelta != 0)
        camera.Zoom -= wheelDelta * 0.01f;
}
```
