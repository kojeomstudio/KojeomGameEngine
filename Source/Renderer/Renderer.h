#pragma once

#include "Renderer/RenderScene.h"
#include <string>
#include <memory>

namespace Kojeom
{
class IRendererBackend
{
public:
    virtual ~IRendererBackend() = default;
    virtual bool Initialize(void* windowHandle) = 0;
    virtual void Shutdown() = 0;
    virtual void OnResize(int width, int height) = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Render(const RenderScene& scene) = 0;
    virtual std::string GetName() const = 0;
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() { Shutdown(); }

    bool Initialize(std::unique_ptr<IRendererBackend> backend, void* windowHandle)
    {
        m_backend = std::move(backend);
        if (!m_backend->Initialize(windowHandle))
        {
            m_backend.reset();
            return false;
        }
        return true;
    }

    void Shutdown()
    {
        if (m_backend)
        {
            m_backend->Shutdown();
            m_backend.reset();
        }
    }

    void OnResize(int width, int height)
    {
        if (m_backend) m_backend->OnResize(width, height);
    }

    void Render(const RenderScene& scene)
    {
        if (!m_backend) return;
        m_backend->BeginFrame();
        m_backend->Render(scene);
        m_backend->EndFrame();
    }

    IRendererBackend* GetBackend() { return m_backend.get(); }

private:
    std::unique_ptr<IRendererBackend> m_backend;
};
}
