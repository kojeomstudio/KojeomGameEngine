#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <algorithm>

enum class ERenderModulePhase : uint8
{
    PrePass,
    Shadow,
    Geometry,
    Lighting,
    PostProcess,
    Overlay,
    Count
};

class IRenderModule
{
public:
    virtual ~IRenderModule() = default;

    IRenderModule(const IRenderModule&) = delete;
    IRenderModule& operator=(const IRenderModule&) = delete;

    virtual HRESULT Initialize(class KGraphicsDevice* InDevice) = 0;
    virtual void Shutdown() = 0;
    virtual void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) = 0;

    virtual const std::string& GetName() const = 0;
    virtual ERenderModulePhase GetPhase() const = 0;

    virtual bool IsInitialized() const { return bInitialized; }
    void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }
    bool IsEnabled() const { return bEnabled; }

protected:
    IRenderModule() = default;
    bool bInitialized = false;
    bool bEnabled = false;
};

class KRenderModuleRegistry
{
public:
    KRenderModuleRegistry() = default;
    ~KRenderModuleRegistry() { ShutdownAll(); }

    KRenderModuleRegistry(const KRenderModuleRegistry&) = delete;
    KRenderModuleRegistry& operator=(const KRenderModuleRegistry&) = delete;

    template<typename T, typename... Args>
    std::shared_ptr<T> RegisterModule(Args&&... args)
    {
        static_assert(std::is_base_of_v<IRenderModule, T>, "T must derive from IRenderModule");
        auto Module = std::make_shared<T>(std::forward<Args>(args)...);
        std::type_index key(typeid(T));
        ModuleMap[key] = Module;
        ModuleOrder.push_back(key);
        PhaseModules[static_cast<size_t>(Module->GetPhase())].push_back(key);
        return Module;
    }

    template<typename T>
    T* GetModule()
    {
        static_assert(std::is_base_of_v<IRenderModule, T>, "T must derive from IRenderModule");
        auto it = ModuleMap.find(std::type_index(typeid(T)));
        if (it != ModuleMap.end())
        {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    template<typename T>
    bool HasModule() const
    {
        return ModuleMap.find(std::type_index(typeid(T))) != ModuleMap.end();
    }

    HRESULT InitializeAll(class KGraphicsDevice* InDevice)
    {
        HRESULT result = S_OK;
        for (const auto& key : ModuleOrder)
        {
            auto it = ModuleMap.find(key);
            if (it != ModuleMap.end())
            {
                HRESULT hr = it->second->Initialize(InDevice);
                if (FAILED(hr))
                {
                    LOG_ERROR("Failed to initialize render module: " + it->second->GetName());
                    result = hr;
                }
            }
        }
        return result;
    }

    void ResizeAll(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight)
    {
        for (const auto& key : ModuleOrder)
        {
            auto it = ModuleMap.find(key);
            if (it != ModuleMap.end() && it->second->IsInitialized())
            {
                it->second->OnResize(Device, NewWidth, NewHeight);
            }
        }
    }

    void ShutdownAll()
    {
        for (auto it = ModuleOrder.rbegin(); it != ModuleOrder.rend(); ++it)
        {
            auto mapIt = ModuleMap.find(*it);
            if (mapIt != ModuleMap.end() && mapIt->second->IsInitialized())
            {
                mapIt->second->Shutdown();
            }
        }
        ModuleMap.clear();
        ModuleOrder.clear();
        for (auto& phase : PhaseModules)
        {
            phase.clear();
        }
    }

    std::vector<IRenderModule*> GetModulesByPhase(ERenderModulePhase Phase) const
    {
        std::vector<IRenderModule*> Result;
        size_t idx = static_cast<size_t>(Phase);
        if (idx < static_cast<size_t>(ERenderModulePhase::Count))
        {
            for (const auto& key : PhaseModules[idx])
            {
                auto it = ModuleMap.find(key);
                if (it != ModuleMap.end())
                {
                    Result.push_back(it->second.get());
                }
            }
        }
        return Result;
    }

    size_t GetModuleCount() const { return ModuleMap.size(); }

    void ForEachModule(std::function<void(IRenderModule*)> Callback)
    {
        for (const auto& key : ModuleOrder)
        {
            auto it = ModuleMap.find(key);
            if (it != ModuleMap.end())
            {
                Callback(it->second.get());
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<IRenderModule>> ModuleMap;
    std::vector<std::type_index> ModuleOrder;
    std::vector<std::type_index> PhaseModules[static_cast<size_t>(ERenderModulePhase::Count)];
};
