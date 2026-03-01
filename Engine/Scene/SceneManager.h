#pragma once

#include "Actor.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <functional>

class KSceneManager
{
public:
    KSceneManager() = default;
    ~KSceneManager() = default;

    KSceneManager(const KSceneManager&) = delete;
    KSceneManager& operator=(const KSceneManager&) = delete;

    HRESULT Initialize();

    std::shared_ptr<KScene> CreateScene(const std::string& Name);
    std::shared_ptr<KScene> LoadScene(const std::wstring& Path);
    HRESULT SaveScene(const std::wstring& Path, std::shared_ptr<KScene> Scene);

    void SetActiveScene(const std::string& Name);
    void SetActiveScene(std::shared_ptr<KScene> Scene);
    std::shared_ptr<KScene> GetActiveScene() const { return ActiveScene; }

    std::shared_ptr<KScene> GetScene(const std::string& Name) const;
    bool HasScene(const std::string& Name) const;

    void UnloadScene(const std::string& Name);
    void UnloadAllScenes();

    void Tick(float DeltaTime);
    void Render(class KRenderer* Renderer);

    const std::unordered_map<std::string, std::shared_ptr<KScene>>& GetAllScenes() const { return Scenes; }
    size_t GetSceneCount() const { return Scenes.size(); }

    template<typename FuncType>
    void ForEachScene(FuncType&& InFunc)
    {
        for (auto& pair : Scenes)
        {
            InFunc(pair.second);
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<KScene>> Scenes;
    std::shared_ptr<KScene> ActiveScene;
    bool bInitialized = false;
};
