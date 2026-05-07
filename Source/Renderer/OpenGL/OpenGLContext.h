#pragma once

#include <glad/gl.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include "Core/Log.h"

namespace Kojeom
{
class OpenGLContext
{
public:
    OpenGLContext() = default;
    ~OpenGLContext() { Shutdown(); }

    bool Initialize(void* windowHandle)
    {
        glfwMakeContextCurrent(static_cast<GLFWwindow*>(windowHandle));
        int version = gladLoadGL(glfwGetProcAddress);
        if (!version)
        {
            KE_LOG_ERROR("Failed to initialize GLAD");
            return false;
        }
        KE_LOG_INFO("OpenGL Version: {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

        if (GLAD_VERSION_MAJOR(version) < 4 || (GLAD_VERSION_MAJOR(version) == 4 && GLAD_VERSION_MINOR(version) < 5))
        {
            KE_LOG_WARN("OpenGL 4.5 or higher recommended, got {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
        }

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const GLchar* message, const void* userParam)
        {
            if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
                return;
            if (severity == GL_DEBUG_SEVERITY_HIGH)
                KE_LOG_ERROR("GL: {}", message);
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
                KE_LOG_WARN("GL: {}", message);
        }, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        GLint maxTextureUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
        KE_LOG_INFO("Max texture units: {}", maxTextureUnits);

        m_initialized = true;
        return true;
    }

    void Shutdown()
    {
        m_initialized = false;
    }

    bool IsInitialized() const { return m_initialized; }

    void SetViewport(int x, int y, int width, int height)
    {
        glViewport(x, y, width, height);
    }

    void ClearColorDepth()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void ClearDepth()
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void ClearColor()
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void EnableDepthTest(bool enable)
    {
        if (enable) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }

    void SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void BindFramebuffer(GLuint fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void BindDefaultFramebuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void UseProgram(GLuint program)
    {
        glUseProgram(program);
    }

    void Flush()
    {
        glFlush();
    }

    static void GetFramebufferSize(void* windowHandle, int* width, int* height)
    {
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(windowHandle), width, height);
    }

private:
    bool m_initialized = false;
};
}
