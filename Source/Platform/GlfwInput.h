#pragma once

#include "Platform/IInput.h"

namespace Kojeom
{
class GlfwInput : public IInput
{
public:
    GlfwInput() = default;

    void BeginFrame() override
    {
        m_state.BeginFrame();
    }

    const InputState& GetState() const override
    {
        return m_state;
    }

    void OnKeyEvent(int key, int scancode, int action, int mods) override
    {
        if (key < 0 || key >= 512) return;
        if (action == GLFW_PRESS)
        {
            m_state.keysDown[key] = true;
            m_state.keysPressed[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_state.keysDown[key] = false;
            m_state.keysReleased[key] = true;
        }
    }

    void OnMouseMoveEvent(double xpos, double ypos) override
    {
        m_state.mouseDeltaX = xpos - m_state.mouseX;
        m_state.mouseDeltaY = ypos - m_state.mouseY;
        m_state.mouseX = xpos;
        m_state.mouseY = ypos;
    }

    void OnMouseButtonEvent(int button, int action, int mods) override
    {
        if (button < 0 || button > 2) return;
        if (action == GLFW_PRESS)
        {
            m_state.mouseDown[button] = true;
            m_state.mousePressed[button] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_state.mouseDown[button] = false;
            m_state.mouseReleased[button] = true;
        }
    }

private:
    InputState m_state;
};
}
