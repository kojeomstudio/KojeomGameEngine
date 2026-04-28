#pragma once

#include "../Core/Subsystem.h"
#include "InputManager.h"
#include <memory>

class KInputSubsystem : public ISubsystem
{
public:
    KInputSubsystem() = default;
    ~KInputSubsystem() override = default;

    KInputSubsystem(const KInputSubsystem&) = delete;
    KInputSubsystem& operator=(const KInputSubsystem&) = delete;

    HRESULT Initialize() override
    {
        State = ESubsystemState::Initialized;
        return S_OK;
    }

    void Tick(float DeltaTime) override
    {
        if (InputManager)
        {
            InputManager->Update();
        }
    }

    void Shutdown() override
    {
        if (InputManager)
        {
            InputManager->Shutdown();
            InputManager.reset();
        }
        State = ESubsystemState::Shutdown;
    }

    const std::string& GetName() const override
    {
        static const std::string Name = "InputSubsystem";
        return Name;
    }

    void SetInputManager(std::unique_ptr<KInputManager> InInputManager)
    {
        InputManager = std::move(InInputManager);
    }

    KInputManager* GetInputManager() { return InputManager.get(); }
    const KInputManager* GetInputManager() const { return InputManager.get(); }

private:
    std::unique_ptr<KInputManager> InputManager;
};
