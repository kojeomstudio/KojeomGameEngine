#include "SceneManager.h"
#include "../Utils/Logger.h"
#include "../Utils/Common.h"

HRESULT KSceneManager::Initialize()
{
    if (bInitialized)
    {
        return S_OK;
    }

    LOG_INFO("Initializing Scene Manager");

    bInitialized = true;
    return S_OK;
}

std::shared_ptr<KScene> KSceneManager::CreateScene(const std::string& Name)
{
    if (HasScene(Name))
    {
        LOG_WARNING("Scene already exists: " + Name);
        return GetScene(Name);
    }

    auto scene = std::make_shared<KScene>();
    scene->SetName(Name);

    Scenes[Name] = scene;

    LOG_INFO("Created scene: " + Name);
    return scene;
}

std::shared_ptr<KScene> KSceneManager::LoadScene(const std::wstring& Path)
{
    if (PathUtils::ContainsTraversal(Path) || !PathUtils::IsPathSafe(Path, L"."))
    {
        LOG_ERROR("Scene path rejected (unsafe path): " + StringUtils::WideToMultiByte(Path));
        return nullptr;
    }

    auto scene = std::make_shared<KScene>();

    HRESULT hr = scene->Load(Path);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to load scene from: " + StringUtils::WideToMultiByte(Path));
        return nullptr;
    }

    std::string sceneName = scene->GetName();
    if (sceneName.empty())
    {
        sceneName = StringUtils::WideToMultiByte(Path);
        size_t lastSlash = sceneName.find_last_of("\\/");
        if (lastSlash != std::string::npos)
        {
            sceneName = sceneName.substr(lastSlash + 1);
        }
        size_t lastDot = sceneName.find_last_of(".");
        if (lastDot != std::string::npos)
        {
            sceneName = sceneName.substr(0, lastDot);
        }
        scene->SetName(sceneName);
    }

    Scenes[sceneName] = scene;

    LOG_INFO("Loaded scene: " + sceneName);
    return scene;
}

HRESULT KSceneManager::SaveScene(const std::wstring& Path, std::shared_ptr<KScene> Scene)
{
    if (!Scene)
    {
        LOG_ERROR("Cannot save null scene");
        return E_INVALIDARG;
    }

    if (PathUtils::ContainsTraversal(Path) || !PathUtils::IsPathSafe(Path, L"."))
    {
        LOG_ERROR("Scene save path rejected (unsafe path): " + StringUtils::WideToMultiByte(Path));
        return E_INVALIDARG;
    }

    HRESULT hr = Scene->Save(Path);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to save scene to: " + StringUtils::WideToMultiByte(Path));
        return hr;
    }

    LOG_INFO("Saved scene: " + Scene->GetName());
    return S_OK;
}

void KSceneManager::SetActiveScene(const std::string& Name)
{
    auto it = Scenes.find(Name);
    if (it != Scenes.end())
    {
        ActiveScene = it->second;
        LOG_INFO("Set active scene: " + Name);
    }
    else
    {
        LOG_WARNING("Scene not found: " + Name);
    }
}

void KSceneManager::SetActiveScene(std::shared_ptr<KScene> Scene)
{
    if (Scene)
    {
        ActiveScene = Scene;
        if (!HasScene(Scene->GetName()))
        {
            Scenes[Scene->GetName()] = Scene;
        }
        LOG_INFO("Set active scene: " + Scene->GetName());
    }
}

std::shared_ptr<KScene> KSceneManager::GetScene(const std::string& Name) const
{
    auto it = Scenes.find(Name);
    if (it != Scenes.end())
    {
        return it->second;
    }
    return nullptr;
}

bool KSceneManager::HasScene(const std::string& Name) const
{
    return Scenes.find(Name) != Scenes.end();
}

void KSceneManager::UnloadScene(const std::string& Name)
{
    auto it = Scenes.find(Name);
    if (it != Scenes.end())
    {
        if (ActiveScene == it->second)
        {
            ActiveScene = nullptr;
        }

        it->second->Clear();
        Scenes.erase(it);

        LOG_INFO("Unloaded scene: " + Name);
    }
}

void KSceneManager::UnloadAllScenes()
{
    ActiveScene = nullptr;

    for (auto& pair : Scenes)
    {
        pair.second->Clear();
    }
    Scenes.clear();

    LOG_INFO("Unloaded all scenes");
}

void KSceneManager::Tick(float DeltaTime)
{
    if (ActiveScene)
    {
        ActiveScene->Tick(DeltaTime);
    }
}

void KSceneManager::Render(KRenderer* Renderer)
{
    if (ActiveScene)
    {
        ActiveScene->Render(Renderer);
    }
}
