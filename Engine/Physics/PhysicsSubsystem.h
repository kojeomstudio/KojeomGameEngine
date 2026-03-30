#pragma once

#include "../Core/Subsystem.h"
#include "PhysicsWorld.h"
#include <memory>

/**
 * @brief Physics subsystem adapter for the modular engine architecture
 *
 * Wraps KPhysicsWorld as an ISubsystem, enabling it to be registered
 * with KSubsystemRegistry for uniform lifecycle management.
 */
class KPhysicsSubsystem : public ISubsystem
{
public:
    KPhysicsSubsystem() = default;
    ~KPhysicsSubsystem() override = default;

    KPhysicsSubsystem(const KPhysicsSubsystem&) = delete;
    KPhysicsSubsystem& operator=(const KPhysicsSubsystem&) = delete;

    HRESULT Initialize() override
    {
        HRESULT hr = PhysicsWorld.Initialize(PhysicsSettings);
        if (SUCCEEDED(hr))
        {
            State = ESubsystemState::Initialized;
        }
        return hr;
    }

    void Tick(float DeltaTime) override
    {
        PhysicsWorld.Update(DeltaTime);
    }

    void Shutdown() override
    {
        PhysicsWorld.Shutdown();
        State = ESubsystemState::Shutdown;
    }

    const std::string& GetName() const override
    {
        static const std::string Name = "PhysicsSubsystem";
        return Name;
    }

    /** @brief Access the underlying physics world */
    KPhysicsWorld* GetPhysicsWorld() { return &PhysicsWorld; }
    const KPhysicsWorld* GetPhysicsWorld() const { return &PhysicsWorld; }

    /** @brief Set physics settings (must be called before Initialize) */
    void SetSettings(const FPhysicsSettings& InSettings) { PhysicsSettings = InSettings; }
    const FPhysicsSettings& GetSettings() const { return PhysicsSettings; }

private:
    KPhysicsWorld PhysicsWorld;
    FPhysicsSettings PhysicsSettings;
};
