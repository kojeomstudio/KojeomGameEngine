#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include <string>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <vector>

/**
 * @brief Subsystem lifecycle states
 */
enum class ESubsystemState : uint8
{
    Uninitialized,
    Initialized,
    Running,
    Shutdown
};

/**
 * @brief Base subsystem interface for modular engine architecture
 *
 * All engine modules (Renderer, Physics, Audio, etc.) implement this interface
 * to enable uniform lifecycle management and module discovery.
 */
class ISubsystem
{
public:
    virtual ~ISubsystem() = default;

    ISubsystem(const ISubsystem&) = delete;
    ISubsystem& operator=(const ISubsystem&) = delete;

    /** @brief Called once during engine startup */
    virtual HRESULT Initialize() = 0;

    /** @brief Called every frame */
    virtual void Tick(float DeltaTime) = 0;

    /** @brief Called during engine shutdown */
    virtual void Shutdown() = 0;

    /** @brief Human-readable subsystem name */
    virtual const std::string& GetName() const = 0;

    /** @brief Current lifecycle state */
    ESubsystemState GetState() const { return State; }

    /** @brief Whether the subsystem has been initialized */
    bool IsInitialized() const { return State == ESubsystemState::Initialized || State == ESubsystemState::Running; }

protected:
    ISubsystem() = default;
    ESubsystemState State = ESubsystemState::Uninitialized;

    friend class KSubsystemRegistry;
};

/**
 * @brief Registry that owns and manages all engine subsystems
 *
 * Subsystems are registered at startup, initialized in order,
 * ticked each frame, and shut down in reverse order.
 */
class KSubsystemRegistry
{
public:
    KSubsystemRegistry() = default;
    ~KSubsystemRegistry() { ShutdownAll(); }

    KSubsystemRegistry(const KSubsystemRegistry&) = delete;
    KSubsystemRegistry& operator=(const KSubsystemRegistry&) = delete;

    /**
     * @brief Register a subsystem instance
     * @tparam T Subsystem type (must derive from ISubsystem)
     * @param Subsystem Shared pointer to the subsystem
     */
    template<typename T>
    void Register(std::shared_ptr<T> Subsystem)
    {
        static_assert(std::is_base_of_v<ISubsystem, T>, "T must derive from ISubsystem");
        std::type_index key(typeid(T));
        SubsystemMap[key] = Subsystem;
        bool alreadyExists = false;
        for (const auto& existingKey : SubsystemOrder)
        {
            if (existingKey == key)
            {
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists)
        {
            SubsystemOrder.push_back(key);
        }
    }

    /**
     * @brief Get a subsystem by type
     * @tparam T Subsystem type
     * @return Pointer to subsystem or nullptr if not registered
     */
    template<typename T>
    T* Get()
    {
        static_assert(std::is_base_of_v<ISubsystem, T>, "T must derive from ISubsystem");
        auto it = SubsystemMap.find(std::type_index(typeid(T)));
        if (it != SubsystemMap.end())
        {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    /** @brief Initialize all registered subsystems in registration order */
    HRESULT InitializeAll()
    {
        HRESULT result = S_OK;
        for (const auto& key : SubsystemOrder)
        {
            auto it = SubsystemMap.find(key);
            if (it != SubsystemMap.end())
            {
                HRESULT hr = it->second->Initialize();
                if (FAILED(hr))
                {
                    LOG_ERROR("Failed to initialize subsystem: " + it->second->GetName());
                    result = hr;
                }
            }
        }
        return result;
    }

    /** @brief Tick all registered subsystems */
    void TickAll(float DeltaTime)
    {
        for (const auto& key : SubsystemOrder)
        {
            auto it = SubsystemMap.find(key);
            if (it != SubsystemMap.end() && it->second->IsInitialized())
            {
                it->second->State = ESubsystemState::Running;
                it->second->Tick(DeltaTime);
            }
        }
    }

    /** @brief Shut down all subsystems in reverse registration order */
    void ShutdownAll()
    {
        for (auto it = SubsystemOrder.rbegin(); it != SubsystemOrder.rend(); ++it)
        {
            auto mapIt = SubsystemMap.find(*it);
            if (mapIt != SubsystemMap.end() && mapIt->second->IsInitialized())
            {
                mapIt->second->Shutdown();
                mapIt->second->State = ESubsystemState::Shutdown;
            }
        }
        SubsystemMap.clear();
        SubsystemOrder.clear();
    }

    /** @brief Number of registered subsystems */
    size_t GetCount() const { return SubsystemMap.size(); }

private:
    std::unordered_map<std::type_index, std::shared_ptr<ISubsystem>> SubsystemMap;
    std::vector<std::type_index> SubsystemOrder;
};
