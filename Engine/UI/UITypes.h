#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

using namespace DirectX;

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

enum class EUIHorizontalAlignment
{
    Left,
    Center,
    Right,
    Stretch
};

enum class EUIVerticalAlignment
{
    Top,
    Center,
    Bottom,
    Stretch
};

enum class EUIEventType
{
    Click,
    HoverEnter,
    HoverLeave,
    Pressed,
    Released,
    Focus,
    FocusLost
};

struct FColor
{
    float R, G, B, A;

    FColor() : R(1.0f), G(1.0f), B(1.0f), A(1.0f) {}
    FColor(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}

    static FColor White() { return FColor(1, 1, 1, 1); }
    static FColor Black() { return FColor(0, 0, 0, 1); }
    static FColor Red() { return FColor(1, 0, 0, 1); }
    static FColor Green() { return FColor(0, 1, 0, 1); }
    static FColor Blue() { return FColor(0, 0, 1, 1); }
    static FColor Yellow() { return FColor(1, 1, 0, 1); }
    static FColor Cyan() { return FColor(0, 1, 1, 1); }
    static FColor Magenta() { return FColor(1, 0, 1, 1); }
    static FColor Transparent() { return FColor(0, 0, 0, 0); }
    static FColor Gray(float brightness = 0.5f) { return FColor(brightness, brightness, brightness, 1); }

    XMFLOAT4 ToFloat4() const { return XMFLOAT4(R, G, B, A); }
};

struct FRect
{
    float X, Y, Width, Height;

    FRect() : X(0), Y(0), Width(0), Height(0) {}
    FRect(float x, float y, float w, float h) : X(x), Y(y), Width(w), Height(h) {}

    bool Contains(float px, float py) const
    {
        return px >= X && px <= X + Width && py >= Y && py <= Y + Height;
    }

    bool Intersects(const FRect& other) const
    {
        return X < other.X + other.Width && X + Width > other.X &&
               Y < other.Y + other.Height && Y + Height > other.Y;
    }

    FRect operator+(const FRect& other) const
    {
        return FRect(X + other.X, Y + other.Y, Width + other.Width, Height + other.Height);
    }
};

struct FPadding
{
    float Left, Top, Right, Bottom;

    FPadding() : Left(0), Top(0), Right(0), Bottom(0) {}
    FPadding(float uniform) : Left(uniform), Top(uniform), Right(uniform), Bottom(uniform) {}
    FPadding(float horizontal, float vertical) : Left(horizontal), Top(vertical), Right(horizontal), Bottom(vertical) {}
    FPadding(float left, float top, float right, float bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
};

struct FUIVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
    XMFLOAT4 Color;

    FUIVertex() : Position(0, 0, 0), TexCoord(0, 0), Color(1, 1, 1, 1) {}
    FUIVertex(const XMFLOAT3& pos, const XMFLOAT2& uv, const XMFLOAT4& col)
        : Position(pos), TexCoord(uv), Color(col) {}
};

struct FFontChar
{
    wchar_t Character;
    float X, Y;
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

using UICallback = std::function<void()>;
