#pragma once

#include <cstdint>
#include <unordered_map>

namespace Kojeom
{
enum class KeyCode : uint32_t
{
    Unknown = 0,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space = 32,
    Enter = 257,
    Escape = 256,
    Tab = 258,
    LeftShift = 340,
    RightShift = 344,
    LeftCtrl = 341,
    RightCtrl = 345,
    LeftAlt = 342,
    Down = 264, Up = 265, Left = 263, Right = 262,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

enum class MouseButton : uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2
};

struct InputState
{
    bool keysDown[512] = {};
    bool keysPressed[512] = {};
    bool keysReleased[512] = {};
    bool mouseDown[3] = {};
    bool mousePressed[3] = {};
    bool mouseReleased[3] = {};
    double mouseX = 0.0;
    double mouseY = 0.0;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;

    void BeginFrame()
    {
        for (int i = 0; i < 512; ++i)
        {
            keysPressed[i] = false;
            keysReleased[i] = false;
        }
        for (int i = 0; i < 3; ++i)
        {
            mousePressed[i] = false;
            mouseReleased[i] = false;
        }
        mouseDeltaX = 0.0;
        mouseDeltaY = 0.0;
    }

    bool IsKeyDown(KeyCode key) const { return keysDown[static_cast<uint32_t>(key)]; }
    bool IsKeyPressed(KeyCode key) const { return keysPressed[static_cast<uint32_t>(key)]; }
    bool IsKeyReleased(KeyCode key) const { return keysReleased[static_cast<uint32_t>(key)]; }
    bool IsMouseButtonDown(MouseButton btn) const { return mouseDown[static_cast<int>(btn)]; }
    bool IsMouseButtonPressed(MouseButton btn) const { return mousePressed[static_cast<int>(btn)]; }
};

class IInput
{
public:
    virtual ~IInput() = default;
    virtual void BeginFrame() = 0;
    virtual const InputState& GetState() const = 0;
    virtual void OnKeyEvent(int key, int scancode, int action, int mods) = 0;
    virtual void OnMouseMoveEvent(double xpos, double ypos) = 0;
    virtual void OnMouseButtonEvent(int button, int action, int mods) = 0;
};
}
