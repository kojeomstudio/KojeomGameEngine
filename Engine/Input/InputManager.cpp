#include "InputManager.h"
#include "../Utils/Logger.h"

KInputManager* KInputManager::Instance = nullptr;

KInputManager::KInputManager()
    : WindowHandle(nullptr)
    , bIsInitialized(false)
    , bMouseVisible(true)
    , bMouseCaptured(false)
    , bRawInputEnabled(false)
    , bCursorClipped(false)
{
    Instance = this;
}

KInputManager::~KInputManager()
{
    Shutdown();
    if (Instance == this)
    {
        Instance = nullptr;
    }
}

HRESULT KInputManager::Initialize(HWND InWindowHandle)
{
    if (bIsInitialized)
    {
        return S_OK;
    }

    if (!InWindowHandle)
    {
        LOG_ERROR("Invalid window handle for input manager");
        return E_INVALIDARG;
    }

    WindowHandle = InWindowHandle;

    RAWINPUTDEVICE Rid;
    Rid.usUsagePage = 0x01;
    Rid.usUsage = 0x02;
    Rid.dwFlags = 0;
    Rid.hwndTarget = WindowHandle;

    if (RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        bRawInputEnabled = true;
        LOG_INFO("Raw input enabled for mouse");
    }
    else
    {
        LOG_WARNING("Failed to enable raw input, using standard mouse input");
    }

    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(WindowHandle, &cursorPos);
    MouseState.X = cursorPos.x;
    MouseState.Y = cursorPos.y;

    bIsInitialized = true;
    LOG_INFO("Input manager initialized");
    return S_OK;
}

void KInputManager::Shutdown()
{
    if (!bIsInitialized)
    {
        return;
    }

    if (bMouseCaptured)
    {
        ReleaseCapture();
    }

    if (bCursorClipped)
    {
        ClipCursor(nullptr);
    }

    KeyStates.clear();
    Actions.clear();
    InputCallbacks.clear();

    while (!EventQueue.empty())
    {
        EventQueue.pop();
    }

    bIsInitialized = false;
    LOG_INFO("Input manager shutdown");
}

void KInputManager::Update()
{
    UpdateKeyStates();

    PrevMouseState = MouseState;
    MouseState.DeltaX = 0;
    MouseState.DeltaY = 0;
    MouseState.WheelDelta = 0;
}

void KInputManager::UpdateKeyStates()
{
    for (auto& Pair : KeyStates)
    {
        FKeyState& State = Pair.second;
        State.bWasPressed = State.bIsPressed;
    }
}

void KInputManager::ProcessKeyDown(uint32_t KeyCode)
{
    if (KeyCode == 0) return;

    FKeyState& State = KeyStates[KeyCode];
    State.bWasPressed = State.bIsPressed;
    State.bIsPressed = true;

    FInputEvent Event;
    Event.KeyCode = KeyCode;
    Event.State = EInputState::Pressed;
    Event.bIsMouseEvent = false;
    DispatchCallbacks(Event);
}

void KInputManager::ProcessKeyUp(uint32_t KeyCode)
{
    if (KeyCode == 0) return;

    FKeyState& State = KeyStates[KeyCode];
    State.bWasPressed = State.bIsPressed;
    State.bIsPressed = false;

    FInputEvent Event;
    Event.KeyCode = KeyCode;
    Event.State = EInputState::Released;
    Event.bIsMouseEvent = false;
    DispatchCallbacks(Event);
}

void KInputManager::ProcessMouseMove(int32_t X, int32_t Y)
{
    MouseState.DeltaX = X - MouseState.X;
    MouseState.DeltaY = Y - MouseState.Y;
    MouseState.X = X;
    MouseState.Y = Y;
}

void KInputManager::ProcessMouseWheel(int32_t WheelDelta)
{
    MouseState.WheelDelta = WheelDelta;
}

void KInputManager::ProcessMouseDown(EMouseButton::Button Button)
{
    MouseState.Buttons |= static_cast<uint32_t>(Button);

    uint32_t KeyCode = EKeyCode::MouseLeft + (static_cast<uint32_t>(Button) - 1);

    FKeyState& State = KeyStates[KeyCode];
    State.bWasPressed = State.bIsPressed;
    State.bIsPressed = true;

    FInputEvent Event;
    Event.KeyCode = KeyCode;
    Event.State = EInputState::Pressed;
    Event.MouseX = MouseState.X;
    Event.MouseY = MouseState.Y;
    Event.bIsMouseEvent = true;
    DispatchCallbacks(Event);
}

void KInputManager::ProcessMouseUp(EMouseButton::Button Button)
{
    MouseState.Buttons &= ~static_cast<uint32_t>(Button);

    uint32_t KeyCode = EKeyCode::MouseLeft + (static_cast<uint32_t>(Button) - 1);

    FKeyState& State = KeyStates[KeyCode];
    State.bWasPressed = State.bIsPressed;
    State.bIsPressed = false;

    FInputEvent Event;
    Event.KeyCode = KeyCode;
    Event.State = EInputState::Released;
    Event.MouseX = MouseState.X;
    Event.MouseY = MouseState.Y;
    Event.bIsMouseEvent = true;
    DispatchCallbacks(Event);
}

void KInputManager::ProcessRawMouseInput(int32_t DeltaX, int32_t DeltaY)
{
    MouseState.DeltaX += DeltaX;
    MouseState.DeltaY += DeltaY;
}

bool KInputManager::IsKeyDown(uint32_t KeyCode) const
{
    auto It = KeyStates.find(KeyCode);
    if (It != KeyStates.end())
    {
        return It->second.bIsPressed;
    }
    return false;
}

bool KInputManager::IsKeyUp(uint32_t KeyCode) const
{
    auto It = KeyStates.find(KeyCode);
    if (It != KeyStates.end())
    {
        return !It->second.bIsPressed;
    }
    return true;
}

bool KInputManager::IsKeyHeld(uint32_t KeyCode) const
{
    auto It = KeyStates.find(KeyCode);
    if (It != KeyStates.end())
    {
        return It->second.bIsPressed && It->second.bWasPressed;
    }
    return false;
}

bool KInputManager::IsKeyJustPressed(uint32_t KeyCode) const
{
    auto It = KeyStates.find(KeyCode);
    if (It != KeyStates.end())
    {
        return It->second.bIsPressed && !It->second.bWasPressed;
    }
    return false;
}

bool KInputManager::IsKeyJustReleased(uint32_t KeyCode) const
{
    auto It = KeyStates.find(KeyCode);
    if (It != KeyStates.end())
    {
        return !It->second.bIsPressed && It->second.bWasPressed;
    }
    return false;
}

bool KInputManager::IsMouseButtonDown(EMouseButton::Button Button) const
{
    return (MouseState.Buttons & static_cast<uint32_t>(Button)) != 0;
}

bool KInputManager::IsMouseButtonUp(EMouseButton::Button Button) const
{
    return (MouseState.Buttons & static_cast<uint32_t>(Button)) == 0;
}

bool KInputManager::IsMouseButtonHeld(EMouseButton::Button Button) const
{
    return (MouseState.Buttons & static_cast<uint32_t>(Button)) != 0 &&
           (PrevMouseState.Buttons & static_cast<uint32_t>(Button)) != 0;
}

void KInputManager::GetMousePosition(int32_t& OutX, int32_t& OutY) const
{
    OutX = MouseState.X;
    OutY = MouseState.Y;
}

void KInputManager::GetMouseDelta(int32_t& OutDeltaX, int32_t& OutDeltaY) const
{
    OutDeltaX = MouseState.DeltaX;
    OutDeltaY = MouseState.DeltaY;
}

int32_t KInputManager::GetMouseWheelDelta() const
{
    return MouseState.WheelDelta;
}

void KInputManager::SetMousePosition(int32_t X, int32_t Y)
{
    POINT Point = { X, Y };
    ClientToScreen(WindowHandle, &Point);
    SetCursorPos(Point.x, Point.y);
    MouseState.X = X;
    MouseState.Y = Y;
}

void KInputManager::SetMouseVisible(bool bVisible)
{
    bMouseVisible = bVisible;
    ShowCursor(bVisible ? TRUE : FALSE);
}

void KInputManager::SetMouseCapture(bool bCapture)
{
    bMouseCaptured = bCapture;
    if (bCapture)
    {
        SetCapture(WindowHandle);
    }
    else
    {
        ReleaseCapture();
    }
}

void KInputManager::RegisterInputCallback(uint32_t KeyCode, InputCallback Callback)
{
    InputCallbacks[KeyCode].push_back(Callback);
}

void KInputManager::UnregisterInputCallback(uint32_t KeyCode)
{
    InputCallbacks.erase(KeyCode);
}

void KInputManager::RegisterAction(const std::string& ActionName, uint32_t PrimaryKey, uint32_t ModifierKey)
{
    FInputAction& Action = Actions[ActionName];
    Action.Name = ActionName;
    Action.KeyCodes.clear();
    Action.KeyCodes.push_back(PrimaryKey);
    if (ModifierKey != 0)
    {
        Action.ModifierKeys.push_back(ModifierKey);
    }
}

bool KInputManager::IsActionDown(const std::string& ActionName) const
{
    auto It = Actions.find(ActionName);
    if (It == Actions.end()) return false;

    const FInputAction& Action = It->second;

    for (uint32_t KeyCode : Action.ModifierKeys)
    {
        if (!IsKeyDown(KeyCode)) return false;
    }

    for (uint32_t KeyCode : Action.KeyCodes)
    {
        if (IsKeyDown(KeyCode)) return true;
    }

    return false;
}

bool KInputManager::IsActionPressed(const std::string& ActionName) const
{
    auto It = Actions.find(ActionName);
    if (It == Actions.end()) return false;

    const FInputAction& Action = It->second;

    for (uint32_t KeyCode : Action.ModifierKeys)
    {
        if (!IsKeyDown(KeyCode)) return false;
    }

    for (uint32_t KeyCode : Action.KeyCodes)
    {
        if (IsKeyJustPressed(KeyCode)) return true;
    }

    return false;
}

bool KInputManager::IsActionReleased(const std::string& ActionName) const
{
    auto It = Actions.find(ActionName);
    if (It == Actions.end()) return false;

    const FInputAction& Action = It->second;

    for (uint32_t KeyCode : Action.ModifierKeys)
    {
        if (!IsKeyDown(KeyCode)) return false;
    }

    for (uint32_t KeyCode : Action.KeyCodes)
    {
        if (IsKeyJustReleased(KeyCode)) return true;
    }

    return false;
}

void KInputManager::EnableRawInput(bool bEnable)
{
    if (bEnable == bRawInputEnabled) return;

    if (bEnable)
    {
        RAWINPUTDEVICE Rid;
        Rid.usUsagePage = 0x01;
        Rid.usUsage = 0x02;
        Rid.dwFlags = 0;
        Rid.hwndTarget = WindowHandle;

        if (RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE)))
        {
            bRawInputEnabled = true;
            LOG_INFO("Raw input enabled");
        }
    }
    else
    {
        RAWINPUTDEVICE Rid;
        Rid.usUsagePage = 0x01;
        Rid.usUsage = 0x02;
        Rid.dwFlags = RIDEV_REMOVE;
        Rid.hwndTarget = nullptr;

        if (RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE)))
        {
            bRawInputEnabled = false;
            LOG_INFO("Raw input disabled");
        }
    }
}

void KInputManager::SetCursorClipToWindow(bool bClip)
{
    bCursorClipped = bClip;
    if (bClip)
    {
        RECT Rect;
        GetClientRect(WindowHandle, &Rect);
        POINT UL = { Rect.left, Rect.top };
        POINT LR = { Rect.right, Rect.bottom };
        ClientToScreen(WindowHandle, &UL);
        ClientToScreen(WindowHandle, &LR);
        Rect.left = UL.x;
        Rect.top = UL.y;
        Rect.right = LR.x;
        Rect.bottom = LR.y;
        ClipCursor(&Rect);
    }
    else
    {
        ClipCursor(nullptr);
    }
}

void KInputManager::FlushInput()
{
    KeyStates.clear();
    MouseState = FMouseState();
    PrevMouseState = FMouseState();

    while (!EventQueue.empty())
    {
        EventQueue.pop();
    }
}

void KInputManager::ClearState()
{
    FlushInput();
}

void KInputManager::DispatchCallbacks(const FInputEvent& Event)
{
    auto It = InputCallbacks.find(Event.KeyCode);
    if (It != InputCallbacks.end())
    {
        for (const auto& Callback : It->second)
        {
            Callback(Event);
        }
    }
}

bool KInputManager::CheckActionModifiers(uint32_t ModifierKey) const
{
    if (ModifierKey == 0) return true;
    return IsKeyDown(ModifierKey);
}
