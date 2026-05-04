#pragma once

#include "Platform/IWindow.h"
#include "Core/Log.h"
#include <GLFW/glfw3.h>

namespace Kojeom
{
class GlfwWindow : public IWindow
{
public:
    GlfwWindow() = default;
    ~GlfwWindow() override { Destroy(); }

    bool Create(const WindowDesc& desc) override
    {
        if (!glfwInit())
        {
            KE_LOG_ERROR("Failed to initialize GLFW");
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, desc.hidden ? GLFW_FALSE : GLFW_TRUE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        m_window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
        if (!m_window)
        {
            KE_LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, this);

        m_hidden = desc.hidden;
        KE_LOG_INFO("GLFW Window created: {}x{}", desc.width, desc.height);
        return true;
    }

    void Destroy() override
    {
        if (m_window)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            KE_LOG_INFO("GLFW Window destroyed");
        }
    }

    bool ShouldClose() const override
    {
        return m_window ? glfwWindowShouldClose(m_window) : true;
    }

    void PollEvents() override
    {
        glfwPollEvents();
    }

    void SwapBuffers() override
    {
        if (m_window) glfwSwapBuffers(m_window);
    }

    void GetSize(int& width, int& height) const override
    {
        if (m_window) glfwGetFramebufferSize(m_window, &width, &height);
        else { width = 0; height = 0; }
    }

    void SetTitle(const std::string& title) override
    {
        if (m_window) glfwSetWindowTitle(m_window, title.c_str());
    }

    void* GetNativeHandle() const override
    {
        return static_cast<void*>(m_window);
    }

    bool IsHidden() const override { return m_hidden; }

    GLFWwindow* GetGLFWWindow() const { return m_window; }

    void SetCloseCallback(GLFWwindowclosefun callback)
    {
        if (m_window) glfwSetWindowCloseCallback(m_window, callback);
    }

    void SetKeyCallback(GLFWkeyfun callback)
    {
        if (m_window) glfwSetKeyCallback(m_window, callback);
    }

    void SetCursorPosCallback(GLFWcursorposfun callback)
    {
        if (m_window) glfwSetCursorPosCallback(m_window, callback);
    }

    void SetMouseButtonCallback(GLFWmousebuttonfun callback)
    {
        if (m_window) glfwSetMouseButtonCallback(m_window, callback);
    }

    void SetFramebufferSizeCallback(GLFWframebuffersizefun callback)
    {
        if (m_window) glfwSetFramebufferSizeCallback(m_window, callback);
    }

private:
    GLFWwindow* m_window = nullptr;
    bool m_hidden = false;
};
}
