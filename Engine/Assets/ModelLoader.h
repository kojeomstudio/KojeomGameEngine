#pragma once

#include "../Utils/Common.h"
#include "../Assets/StaticMesh.h"
#include "../Assets/Skeleton.h"
#include "../Assets/Animation.h"
#include "../Assets/SkeletalMeshComponent.h"
#include "../Graphics/Material.h"
#include <future>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

struct FLoadedModel
{
    std::shared_ptr<KStaticMesh> StaticMesh;
    std::vector<std::shared_ptr<KStaticMesh>> SubMeshes;
    std::shared_ptr<KSkeletalMesh> SkeletalMesh;
    std::shared_ptr<KSkeleton> Skeleton;
    std::vector<std::shared_ptr<KAnimation>> Animations;
    std::vector<std::shared_ptr<KMaterial>> Materials;
    std::vector<int32> MeshMaterialIndices;
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

    void SetDevice(ID3D11Device* InDevice) { Device = InDevice; }

    std::shared_ptr<FLoadedModel> LoadModel(const std::wstring& Path, const FModelLoadOptions& Options = FModelLoadOptions());
    std::future<std::shared_ptr<FLoadedModel>> LoadModelAsync(const std::wstring& Path, const FModelLoadOptions& Options = FModelLoadOptions());

    bool IsModelLoaded(const std::wstring& Path) const;
    std::shared_ptr<FLoadedModel> GetLoadedModel(const std::wstring& Path);

    void UnloadModel(const std::wstring& Path);
    void UnloadAllModels();

    static bool IsSupportedFormat(const std::wstring& Path);
    static std::string GetFileExtension(const std::wstring& Path);

private:
    HRESULT LoadWithAssimp(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT LoadOBJFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT LoadGLTFFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT ParseGLTFJson(const std::string& JsonContent, const std::wstring& BasePath, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT LoadFBXFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    HRESULT ParseFBXAscii(const std::string& Content, FLoadedModel* OutModel, const FModelLoadOptions& Options);

    void ProcessNode(void* AssimpNode, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    std::shared_ptr<KStaticMesh> ProcessMesh(void* AssimpMesh, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options);
    void ProcessBones(void* AssimpMesh, FLoadedModel* OutModel, uint32 MeshIndex);
    void ProcessAnimations(void* AssimpScene, FLoadedModel* OutModel);
    void ProcessMaterials(void* AssimpScene, FLoadedModel* OutModel, const std::wstring& ModelPath);

    std::unordered_map<std::wstring, std::shared_ptr<FLoadedModel>> LoadedModels;
    mutable std::mutex LoadedModelsMutex;
    ID3D11Device* Device = nullptr;
    bool bInitialized = false;
    bool bAssimpAvailable = false;
};
