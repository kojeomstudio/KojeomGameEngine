#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace Kojeom
{
struct WindowDesc
{
    int width = 1280;
    int height = 720;
    std::string title = "KojeomEngine";
    bool hidden = false;
    bool resizable = true;
};

class IWindow
{
public:
    virtual ~IWindow() = default;
    virtual bool Create(const WindowDesc& desc) = 0;
    virtual void Destroy() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual void GetSize(int& width, int& height) const = 0;
    virtual void SetTitle(const std::string& title) = 0;
    virtual void* GetNativeHandle() const = 0;
    virtual bool IsHidden() const = 0;
};
}
