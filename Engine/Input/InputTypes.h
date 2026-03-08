#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace EKeyCode
{
    enum Key : uint32_t
    {
        None = 0,

        // Function keys
        F1 = 0x70,
        F2 = 0x71,
        F3 = 0x72,
        F4 = 0x73,
        F5 = 0x74,
        F6 = 0x75,
        F7 = 0x76,
        F8 = 0x77,
        F9 = 0x78,
        F10 = 0x79,
        F11 = 0x7A,
        F12 = 0x7B,

        // Number keys (top row)
        Num0 = 0x30,
        Num1 = 0x31,
        Num2 = 0x32,
        Num3 = 0x33,
        Num4 = 0x34,
        Num5 = 0x35,
        Num6 = 0x36,
        Num7 = 0x37,
        Num8 = 0x38,
        Num9 = 0x39,

        // Letter keys
        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4A,
        K = 0x4B,
        L = 0x4C,
        M = 0x4D,
        N = 0x4E,
        O = 0x4F,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5A,

        // Arrow keys
        Left = VK_LEFT,
        Right = VK_RIGHT,
        Up = VK_UP,
        Down = VK_DOWN,

        // Navigation keys
        Space = VK_SPACE,
        Enter = VK_RETURN,
        Tab = VK_TAB,
        Escape = VK_ESCAPE,
        Backspace = VK_BACK,
        Delete = VK_DELETE,
        Insert = VK_INSERT,
        Home = VK_HOME,
        End = VK_END,
        PageUp = VK_PRIOR,
        PageDown = VK_NEXT,

        // Modifiers
        LeftShift = VK_LSHIFT,
        RightShift = VK_RSHIFT,
        LeftCtrl = VK_LCONTROL,
        RightCtrl = VK_RCONTROL,
        LeftAlt = VK_LMENU,
        RightAlt = VK_RMENU,

        // Numpad
        Numpad0 = VK_NUMPAD0,
        Numpad1 = VK_NUMPAD1,
        Numpad2 = VK_NUMPAD2,
        Numpad3 = VK_NUMPAD3,
        Numpad4 = VK_NUMPAD4,
        Numpad5 = VK_NUMPAD5,
        Numpad6 = VK_NUMPAD6,
        Numpad7 = VK_NUMPAD7,
        Numpad8 = VK_NUMPAD8,
        Numpad9 = VK_NUMPAD9,
        NumpadAdd = VK_ADD,
        NumpadSubtract = VK_SUBTRACT,
        NumpadMultiply = VK_MULTIPLY,
        NumpadDivide = VK_DIVIDE,
        NumpadDecimal = VK_DECIMAL,

        // Symbols
        Minus = 0xBD,
        Plus = 0xBB,
        Comma = 0xBC,
        Period = 0xBE,
        Slash = 0xBF,
        Semicolon = 0xBA,
        Quote = 0xDE,
        LeftBracket = 0xDB,
        RightBracket = 0xDD,
        Backslash = 0xDC,
        Grave = 0xC0,

        // Mouse buttons (virtual codes)
        MouseLeft = 0x100,
        MouseRight = 0x101,
        MouseMiddle = 0x102,
        MouseXButton1 = 0x103,
        MouseXButton2 = 0x104,
    };
}

namespace EMouseButton
{
    enum Button : uint32_t
    {
        None = 0,
        Left = 1,
        Right = 2,
        Middle = 4,
        XButton1 = 8,
        XButton2 = 16,
    };
}

namespace EInputState
{
    enum State : uint8_t
    {
        Released = 0,
        Pressed = 1,
        Held = 2,
        JustReleased = 3,
    };
}

struct FMouseState
{
    int32_t X = 0;
    int32_t Y = 0;
    int32_t DeltaX = 0;
    int32_t DeltaY = 0;
    int32_t WheelDelta = 0;
    uint32_t Buttons = 0;
};

struct FKeyState
{
    bool bIsPressed = false;
    bool bWasPressed = false;
};

struct FInputAction
{
    std::string Name;
    std::vector<uint32_t> KeyCodes;
    std::vector<uint32_t> ModifierKeys;
};
