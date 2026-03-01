#pragma once

#include "../Utils/Common.h"
#include "../Assets/StaticMesh.h"
#include "../Assets/Skeleton.h"
#include "../Assets/Animation.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

struct FLoadedModel
{
    std::shared_ptr<KStaticMesh> StaticMesh;
    std::shared_ptr<KSkeleton> Skeleton;
    std::vector<std::shared_ptr<KAnimation>> Animations;
    std::string Name;
    std::wstring SourcePath;
};

struct FModelLoadOptions
{
    bool bLoadAnimations = true;
    bool bGenerateNormals = false;
    bool bGenerateTangents = true;
    bool bFlipUVs = true;
    float Scale = 1.0f;
};

class KModelLoader
{
public:
    KModelLoader() = default;
    ~KModelLoader() = default;

    KModelLoader(const KModelLoader&) = delete;
    KModelLoader& operator=(const KModelLoader&) = delete;

    HRESULT Initialize();
    void Cleanup();

    std::shared_ptr<FLoadedModel> LoadModel(const std::wstring& Path, const FModelLoadOptions& Options = FModelLoadOptions());
    std::shared_ptr<FLoadedModel> LoadModelAsync(const std::wstring& Path, const FModelLoadOptions& Options = FModelLoadOptions());

    bool IsModelLoaded(const std::wstring& Path) const;
    std::shared_ptr<FLoadedModel> GetLoadedModel(const std::wstring& Path);

    void UnloadModel(const std::wstring& Path);
    void UnloadAllModels();

    static bool IsSupportedFormat(const std::wstring& Path);
    static std::string GetFileExtension(const std::wstring& Path);

private:
    HRESULT LoadWithAssimp(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT LoadOBJFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);

    void ProcessNode(void* AssimpNode, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    std::shared_ptr<KStaticMesh> ProcessMesh(void* AssimpMesh, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    void ProcessBones(void* AssimpMesh, FLoadedModel* OutModel, uint32 MeshIndex);
    void ProcessAnimations(void* AssimpScene, FLoadedModel* OutModel);

    std::unordered_map<std::wstring, std::shared_ptr<FLoadedModel>> LoadedModels;
    bool bInitialized = false;
    bool bAssimpAvailable = false;
};
