#include "ModelLoader.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

HRESULT KModelLoader::Initialize()
{
    if (bInitialized)
    {
        return S_OK;
    }

    LOG_INFO("Initializing Model Loader");

#ifdef USE_ASSIMP
    bAssimpAvailable = true;
    LOG_INFO("Assimp library available for model loading");
#else
    bAssimpAvailable = false;
    LOG_WARNING("Assimp not available - using fallback OBJ loader only");
#endif

    bInitialized = true;
    return S_OK;
}

void KModelLoader::Cleanup()
{
    UnloadAllModels();
    bInitialized = false;
}

std::shared_ptr<FLoadedModel> KModelLoader::LoadModel(const std::wstring& Path, const FModelLoadOptions& Options)
{
    if (!bInitialized)
    {
        LOG_ERROR("Model loader not initialized");
        return nullptr;
    }

    if (IsModelLoaded(Path))
    {
        return GetLoadedModel(Path);
    }

    auto model = std::make_shared<FLoadedModel>();
    model->SourcePath = Path;

    size_t lastSlash = Path.find_last_of(L"\\/");
    size_t lastDot = Path.find_last_of(L".");
    if (lastSlash != std::wstring::npos && lastDot != std::wstring::npos && lastDot > lastSlash)
    {
        model->Name = StringUtils::WideToMultiByte(Path.substr(lastSlash + 1, lastDot - lastSlash - 1));
    }
    else
    {
        model->Name = "UnnamedModel";
    }

    HRESULT hr = E_FAIL;

#ifdef USE_ASSIMP
    if (bAssimpAvailable)
    {
        hr = LoadWithAssimp(Path, model.get(), Options);
    }
    else
#endif
    {
        std::string ext = GetFileExtension(Path);
        if (ext == "obj" || ext == "OBJ")
        {
            hr = LoadOBJFallback(Path, model.get(), Options);
        }
        else
        {
            LOG_ERROR("Unsupported model format: " + ext + " (Assimp not available)");
            return nullptr;
        }
    }

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to load model: " + StringUtils::WideToMultiByte(Path));
        return nullptr;
    }

    LoadedModels[Path] = model;
    LOG_INFO("Loaded model: " + model->Name);
    return model;
}

std::shared_ptr<FLoadedModel> KModelLoader::LoadModelAsync(const std::wstring& Path, const FModelLoadOptions& Options)
{
    return LoadModel(Path, Options);
}

bool KModelLoader::IsModelLoaded(const std::wstring& Path) const
{
    return LoadedModels.find(Path) != LoadedModels.end();
}

std::shared_ptr<FLoadedModel> KModelLoader::GetLoadedModel(const std::wstring& Path)
{
    auto it = LoadedModels.find(Path);
    if (it != LoadedModels.end())
    {
        return it->second;
    }
    return nullptr;
}

void KModelLoader::UnloadModel(const std::wstring& Path)
{
    auto it = LoadedModels.find(Path);
    if (it != LoadedModels.end())
    {
        LoadedModels.erase(it);
        LOG_INFO("Unloaded model: " + StringUtils::WideToMultiByte(Path));
    }
}

void KModelLoader::UnloadAllModels()
{
    LoadedModels.clear();
    LOG_INFO("Unloaded all models");
}

bool KModelLoader::IsSupportedFormat(const std::wstring& Path)
{
    std::string ext = GetFileExtension(Path);
    static const std::vector<std::string> supportedFormats = {
        "fbx", "FBX",
        "obj", "OBJ",
        "gltf", "GLTF",
        "glb", "GLB",
        "dae", "DAE",
        "x", "X"
    };

    for (const auto& format : supportedFormats)
    {
        if (ext == format)
        {
            return true;
        }
    }
    return false;
}

std::string KModelLoader::GetFileExtension(const std::wstring& Path)
{
    size_t lastDot = Path.find_last_of(L".");
    if (lastDot != std::wstring::npos)
    {
        return StringUtils::WideToMultiByte(Path.substr(lastDot + 1));
    }
    return "";
}

HRESULT KModelLoader::LoadWithAssimp(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
#ifdef USE_ASSIMP
    #include <assimp/Importer.hpp>
    #include <assimp/scene.h>
    #include <assimp/postprocess.h>

    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices;
    if (Options.bGenerateNormals)
    {
        flags |= aiProcess_GenNormals;
    }
    if (Options.bGenerateTangents)
    {
        flags |= aiProcess_CalcTangentSpace;
    }
    if (Options.bFlipUVs)
    {
        flags |= aiProcess_FlipUVs;
    }

    const aiScene* scene = importer.ReadFile(StringUtils::WideToMultiByte(Path), flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("Assimp error: " + std::string(importer.GetErrorString()));
        return E_FAIL;
    }

    ProcessNode(scene->mRootNode, scene, OutModel, Options);

    if (Options.bLoadAnimations && scene->HasAnimations())
    {
        ProcessAnimations(scene, OutModel);
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT KModelLoader::LoadOBJFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
    std::ifstream file(Path);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open OBJ file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texcoords;
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            pos.x *= Options.Scale;
            pos.y *= Options.Scale;
            pos.z *= Options.Scale;
            positions.push_back(pos);
        }
        else if (prefix == "vt")
        {
            XMFLOAT2 tex;
            iss >> tex.x >> tex.y;
            if (Options.bFlipUVs)
            {
                tex.y = 1.0f - tex.y;
            }
            texcoords.push_back(tex);
        }
        else if (prefix == "vn")
        {
            XMFLOAT3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (prefix == "f")
        {
            for (int i = 0; i < 3; ++i)
            {
                std::string vertexStr;
                iss >> vertexStr;

                FVertex vertex;
                vertex.Position = XMFLOAT3(0, 0, 0);
                vertex.Normal = XMFLOAT3(0, 1, 0);
                vertex.TexCoord = XMFLOAT2(0, 0);
                vertex.Color = XMFLOAT4(1, 1, 1, 1);

                size_t posSlash1 = vertexStr.find('/');
                if (posSlash1 != std::string::npos)
                {
                    int posIdx = std::stoi(vertexStr.substr(0, posSlash1)) - 1;
                    if (posIdx >= 0 && posIdx < (int)positions.size())
                    {
                        vertex.Position = positions[posIdx];
                    }

                    size_t posSlash2 = vertexStr.find('/', posSlash1 + 1);
                    if (posSlash2 != std::string::npos)
                    {
                        std::string texStr = vertexStr.substr(posSlash1 + 1, posSlash2 - posSlash1 - 1);
                        if (!texStr.empty())
                        {
                            int texIdx = std::stoi(texStr) - 1;
                            if (texIdx >= 0 && texIdx < (int)texcoords.size())
                            {
                                vertex.TexCoord = texcoords[texIdx];
                            }
                        }

                        std::string normStr = vertexStr.substr(posSlash2 + 1);
                        if (!normStr.empty())
                        {
                            int normIdx = std::stoi(normStr) - 1;
                            if (normIdx >= 0 && normIdx < (int)normals.size())
                            {
                                vertex.Normal = normals[normIdx];
                            }
                        }
                    }
                }
                else
                {
                    int posIdx = std::stoi(vertexStr) - 1;
                    if (posIdx >= 0 && posIdx < (int)positions.size())
                    {
                        vertex.Position = positions[posIdx];
                    }
                }

                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32>(vertices.size() - 1));
            }
        }
    }

    file.close();

    if (vertices.empty())
    {
        LOG_ERROR("No vertices found in OBJ file");
        return E_FAIL;
    }

    auto mesh = std::make_shared<KStaticMesh>();
    mesh->SetName(OutModel->Name);

    OutModel->StaticMesh = mesh;

    LOG_INFO("Loaded OBJ: " + std::to_string(vertices.size()) + " vertices, " + std::to_string(indices.size()) + " indices");
    return S_OK;
}

void KModelLoader::ProcessNode(void* AssimpNode, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
#ifdef USE_ASSIMP
    aiNode* node = static_cast<aiNode*>(AssimpNode);
    const aiScene* scene = static_cast<const aiScene*>(AssimpScene);

    for (uint32 i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto staticMesh = ProcessMesh(mesh, scene, OutModel, Options);
        if (staticMesh && !OutModel->StaticMesh)
        {
            OutModel->StaticMesh = staticMesh;
        }
    }

    for (uint32 i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene, OutModel, Options);
    }
#endif
}

std::shared_ptr<KStaticMesh> KModelLoader::ProcessMesh(void* AssimpMesh, void* AssimpScene, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
#ifdef USE_ASSIMP
    aiMesh* mesh = static_cast<aiMesh*>(AssimpMesh);
    const aiScene* scene = static_cast<const aiScene*>(AssimpScene);

    auto staticMesh = std::make_shared<KStaticMesh>();
    
    std::string meshName = mesh->mName.C_Str();
    if (meshName.empty())
    {
        meshName = "Mesh_" + std::to_string(OutModel->StaticMesh ? 1 : 0);
    }
    staticMesh->SetName(meshName);

    std::vector<FVertex> vertices;
    std::vector<uint32> indices;

    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        FVertex vertex;
        
        vertex.Position = XMFLOAT3(
            mesh->mVertices[i].x * Options.Scale,
            mesh->mVertices[i].y * Options.Scale,
            mesh->mVertices[i].z * Options.Scale
        );

        if (mesh->HasNormals())
        {
            vertex.Normal = XMFLOAT3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }
        else
        {
            vertex.Normal = XMFLOAT3(0, 1, 0);
        }

        if (mesh->HasTextureCoords(0))
        {
            vertex.TexCoord = XMFLOAT2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoord = XMFLOAT2(0, 0);
        }

        vertex.Color = XMFLOAT4(1, 1, 1, 1);
        vertices.push_back(vertex);
    }

    for (uint32 i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    return staticMesh;
#else
    return nullptr;
#endif
}

void KModelLoader::ProcessBones(void* AssimpMesh, FLoadedModel* OutModel, uint32 MeshIndex)
{
#ifdef USE_ASSIMP
    aiMesh* mesh = static_cast<aiMesh*>(AssimpMesh);

    if (!mesh->HasBones())
    {
        return;
    }

    if (!OutModel->Skeleton)
    {
        OutModel->Skeleton = std::make_shared<KSkeleton>();
        OutModel->Skeleton->SetName(OutModel->Name + "_Skeleton");
    }
#endif
}

void KModelLoader::ProcessAnimations(void* AssimpScene, FLoadedModel* OutModel)
{
#ifdef USE_ASSIMP
    const aiScene* scene = static_cast<const aiScene*>(AssimpScene);

    for (uint32 i = 0; i < scene->mNumAnimations; ++i)
    {
        aiAnimation* anim = scene->mAnimations[i];
        
        auto animation = std::make_shared<KAnimation>();
        animation->SetName(anim->mName.C_Str());
        
        float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);
        if (ticksPerSecond == 0.0f)
        {
            ticksPerSecond = 25.0f;
        }
        float duration = static_cast<float>(anim->mDuration) / ticksPerSecond;
        
        animation->SetDuration(duration);
        animation->SetTicksPerSecond(ticksPerSecond);

        OutModel->Animations.push_back(animation);
    }

    LOG_INFO("Processed " + std::to_string(OutModel->Animations.size()) + " animations");
#endif
}
