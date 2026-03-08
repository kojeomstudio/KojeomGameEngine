#pragma once

#define NOMINMAX
#include <windows.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <queue>
#include "InputTypes.h"

struct FInputEvent
{
    uint32_t KeyCode = 0;
    EInputState::State State = EInputState::Released;
    int32_t MouseX = 0;
    int32_t MouseY = 0;
    bool bIsMouseEvent = false;
};

class KInputManager
{
public:
    using InputCallback = std::function<void(const FInputEvent&)>;

    KInputManager();
    ~KInputManager();

    KInputManager(const KInputManager&) = delete;
    KInputManager& operator=(const KInputManager&) = delete;

    HRESULT Initialize(HWND WindowHandle);
    void Shutdown();

    void Update();

    void ProcessKeyDown(uint32_t KeyCode);
    void ProcessKeyUp(uint32_t KeyCode);
    void ProcessMouseMove(int32_t X, int32_t Y);
    void ProcessMouseWheel(int32_t WheelDelta);
    void ProcessMouseDown(EMouseButton::Button Button);
    void ProcessMouseUp(EMouseButton::Button Button);
    void ProcessRawMouseInput(int32_t DeltaX, int32_t DeltaY);

    bool IsKeyDown(uint32_t KeyCode) const;
    bool IsKeyUp(uint32_t KeyCode) const;
    bool IsKeyHeld(uint32_t KeyCode) const;
    bool IsKeyJustPressed(uint32_t KeyCode) const;
    bool IsKeyJustReleased(uint32_t KeyCode) const;

    bool IsMouseButtonDown(EMouseButton::Button Button) const;
    bool IsMouseButtonUp(EMouseButton::Button Button) const;
    bool IsMouseButtonHeld(EMouseButton::Button Button) const;

    void GetMousePosition(int32_t& OutX, int32_t& OutY) const;
    void GetMouseDelta(int32_t& OutDeltaX, int32_t& OutDeltaY) const;
    int32_t GetMouseWheelDelta() const;

    void SetMousePosition(int32_t X, int32_t Y);
    void SetMouseVisible(bool bVisible);
    bool IsMouseVisible() const { return bMouseVisible; }

    void SetMouseCapture(bool bCapture);
    bool IsMouseCaptured() const { return bMouseCaptured; }

    void RegisterInputCallback(uint32_t KeyCode, InputCallback Callback);
    void UnregisterInputCallback(uint32_t KeyCode);

    void RegisterAction(const std::string& ActionName, uint32_t PrimaryKey, uint32_t ModifierKey = 0);
    bool IsActionDown(const std::string& ActionName) const;
    bool IsActionPressed(const std::string& ActionName) const;
    bool IsActionReleased(const std::string& ActionName) const;

    void EnableRawInput(bool bEnable);
    bool IsRawInputEnabled() const { return bRawInputEnabled; }

    void SetCursorClipToWindow(bool bClip);
    bool IsCursorClipped() const { return bCursorClipped; }

    void FlushInput();
    void ClearState();

    static KInputManager* GetInstance() { return Instance; }

private:
    void UpdateKeyStates();
    void DispatchCallbacks(const FInputEvent& Event);
    bool CheckActionModifiers(uint32_t ModifierKey) const;

private:
    HWND WindowHandle;
    bool bIsInitialized;
    bool bMouseVisible;
    bool bMouseCaptured;
    bool bRawInputEnabled;
    bool bCursorClipped;

    FMouseState MouseState;
    FMouseState PrevMouseState;

    std::unordered_map<uint32_t, FKeyState> KeyStates;
    std::unordered_map<std::string, FInputAction> Actions;
    std::unordered_map<uint32_t, std::vector<InputCallback>> InputCallbacks;

    std::queue<FInputEvent> EventQueue;

    static KInputManager* Instance;
};
