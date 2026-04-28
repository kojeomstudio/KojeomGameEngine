#pragma once

#include "../Core/Subsystem.h"
#include "Renderer.h"
#include <memory>

class KRenderSubsystem : public ISubsystem
{
public:
    KRenderSubsystem() = default;
    ~KRenderSubsystem() override = default;

    KRenderSubsystem(const KRenderSubsystem&) = delete;
    KRenderSubsystem& operator=(const KRenderSubsystem&) = delete;

    HRESULT Initialize() override
    {
        State = ESubsystemState::Initialized;
        return S_OK;
    }

    void Tick(float DeltaTime) override
    {
    }

    void Shutdown() override
    {
        if (Renderer)
        {
            Renderer->Cleanup();
        }
        State = ESubsystemState::Shutdown;
    }

    const std::string& GetName() const override
    {
        static const std::string Name = "RenderSubsystem";
        return Name;
    }

    void SetRenderer(std::unique_ptr<KRenderer> InRenderer)
    {
        Renderer = std::move(InRenderer);
    }

    KRenderer* GetRenderer() { return Renderer.get(); }
    const KRenderer* GetRenderer() const { return Renderer.get(); }

private:
    std::unique_ptr<KRenderer> Renderer;
};
