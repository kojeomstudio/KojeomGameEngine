#include "ModelLoader.h"
#include "SkeletalMeshComponent.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

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
    LOG_WARNING("Assimp not available - using fallback OBJ/GLTF loader only");
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
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == "obj")
        {
            hr = LoadOBJFallback(Path, model.get(), Options);
        }
        else if (ext == "gltf" || ext == "glb")
        {
            hr = LoadGLTFFallback(Path, model.get(), Options);
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
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    static const std::vector<std::string> supportedFormats = {
        "fbx", "obj", "gltf", "glb", "dae", "x", "3ds", "blend", "ase"
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

#ifdef USE_ASSIMP
static XMMATRIX AiMatrixToXMMatrix(const aiMatrix4x4& aiMat)
{
    return XMMatrixSet(
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    );
}

static void BuildSkeletonRecursive(aiNode* Node, KSkeleton* Skeleton, int32 ParentIndex, std::unordered_map<std::string, uint32>& BoneMapping)
{
    std::string boneName = Node->mName.C_Str();
    
    auto it = BoneMapping.find(boneName);
    uint32 boneIndex;
    
    if (it == BoneMapping.end())
    {
        FBone bone;
        bone.Name = boneName;
        bone.ParentIndex = ParentIndex;
        bone.BindPose = AiMatrixToXMMatrix(Node->mTransformation);
        
        XMVECTOR scale, rotQuat, trans;
        XMMatrixDecompose(&scale, &rotQuat, &trans, bone.BindPose);
        
        XMStoreFloat3(&bone.LocalPosition, trans);
        XMStoreFloat4(&bone.LocalRotation, rotQuat);
        XMStoreFloat3(&bone.LocalScale, scale);
        
        boneIndex = Skeleton->AddBone(bone);
        BoneMapping[boneName] = boneIndex;
    }
    else
    {
        boneIndex = it->second;
    }

    for (uint32 i = 0; i < Node->mNumChildren; ++i)
    {
        BuildSkeletonRecursive(Node->mChildren[i], Skeleton, static_cast<int32>(boneIndex), BoneMapping);
    }
}

static void CollectBoneNames(const aiScene* Scene, std::unordered_set<std::string>& BoneNames)
{
    for (uint32 meshIdx = 0; meshIdx < Scene->mNumMeshes; ++meshIdx)
    {
        aiMesh* mesh = Scene->mMeshes[meshIdx];
        if (!mesh->HasBones()) continue;
        
        for (uint32 boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
        {
            BoneNames.insert(mesh->mBones[boneIdx]->mName.C_Str());
        }
    }
}

static aiNode* FindNodeByName(aiNode* Node, const std::string& Name)
{
    if (std::string(Node->mName.C_Str()) == Name)
    {
        return Node;
    }
    
    for (uint32 i = 0; i < Node->mNumChildren; ++i)
    {
        aiNode* found = FindNodeByName(Node->mChildren[i], Name);
        if (found) return found;
    }
    
    return nullptr;
}
#endif

HRESULT KModelLoader::LoadWithAssimp(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
#ifdef USE_ASSIMP
    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | 
                         aiProcess_LimitBoneWeights | aiProcess_ValidateDataStructure;
    
    if (Options.bGenerateNormals)
    {
        flags |= aiProcess_GenNormals | aiProcess_GenSmoothNormals;
    }
    if (Options.bGenerateTangents)
    {
        flags |= aiProcess_CalcTangentSpace;
    }
    if (Options.bFlipUVs)
    {
        flags |= aiProcess_FlipUVs;
    }

    std::string pathStr = StringUtils::WideToMultiByte(Path);
    const aiScene* scene = importer.ReadFile(pathStr, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("Assimp error: " + std::string(importer.GetErrorString()));
        return E_FAIL;
    }

    std::unordered_set<std::string> boneNames;
    bool bHasSkeleton = false;
    
    for (uint32 meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        if (scene->mMeshes[meshIdx]->HasBones())
        {
            bHasSkeleton = true;
            break;
        }
    }
    
    if (bHasSkeleton)
    {
        CollectBoneNames(scene, boneNames);
        
        OutModel->Skeleton = std::make_shared<KSkeleton>();
        OutModel->Skeleton->SetName(OutModel->Name + "_Skeleton");
        
        std::unordered_map<std::string, uint32> boneMapping;
        BuildSkeletonRecursive(scene->mRootNode, OutModel->Skeleton.get(), INVALID_BONE_INDEX, boneMapping);
        
        for (uint32 meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
        {
            aiMesh* mesh = scene->mMeshes[meshIdx];
            if (!mesh->HasBones()) continue;
            
            for (uint32 boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
            {
                aiBone* aiBone = mesh->mBones[boneIdx];
                std::string boneName = aiBone->mName.C_Str();
                
                int32 skelBoneIdx = OutModel->Skeleton->GetBoneIndex(boneName);
                if (skelBoneIdx != INVALID_BONE_INDEX)
                {
                    FBone* skelBone = OutModel->Skeleton->GetBoneMutable(skelBoneIdx);
                    skelBone->InverseBindPose = AiMatrixToXMMatrix(aiBone->mOffsetMatrix);
                }
            }
        }
        
        OutModel->Skeleton->CalculateBindPoses();
    }

    ProcessNode(scene->mRootNode, scene, OutModel, Options);

    if (Options.bLoadAnimations && scene->HasAnimations())
    {
        ProcessAnimations(scene, OutModel);
    }

    return S_OK;
#else
    LOG_ERROR("Assimp not available");
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
        if (line.empty() || line[0] == '#') continue;
        
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
            std::vector<uint32> faceIndices;
            std::string vertexStr;
            
            while (iss >> vertexStr)
            {
                FVertex vertex;
                vertex.Position = XMFLOAT3(0, 0, 0);
                vertex.Normal = XMFLOAT3(0, 1, 0);
                vertex.TexCoord = XMFLOAT2(0, 0);
                vertex.Color = XMFLOAT4(1, 1, 1, 1);

                size_t posSlash1 = vertexStr.find('/');
                if (posSlash1 != std::string::npos)
                {
                    if (posSlash1 > 0)
                    {
                        int posIdx = std::stoi(vertexStr.substr(0, posSlash1)) - 1;
                        if (posIdx >= 0 && posIdx < (int)positions.size())
                        {
                            vertex.Position = positions[posIdx];
                        }
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
                faceIndices.push_back(static_cast<uint32>(vertices.size() - 1));
            }
            
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[i]);
                indices.push_back(faceIndices[i + 1]);
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
    mesh->AddLOD(vertices, indices);
    mesh->CalculateBounds();

    OutModel->StaticMesh = mesh;

    LOG_INFO("Loaded OBJ: " + std::to_string(vertices.size()) + " vertices, " + std::to_string(indices.size()) + " indices");
    return S_OK;
}

HRESULT KModelLoader::LoadGLTFFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
    std::string ext = GetFileExtension(Path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "glb")
    {
        std::ifstream file(Path, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open GLB file: " + StringUtils::WideToMultiByte(Path));
            return E_FAIL;
        }
        
        uint32 magic;
        file.read(reinterpret_cast<char*>(&magic), 4);
        if (magic != 0x46546C67)
        {
            LOG_ERROR("Invalid GLB file (bad magic)");
            return E_FAIL;
        }
        
        uint32 version, totalLength;
        file.read(reinterpret_cast<char*>(&version), 4);
        file.read(reinterpret_cast<char*>(&totalLength), 4);
        
        uint32 jsonChunkLength, jsonChunkType;
        file.read(reinterpret_cast<char*>(&jsonChunkLength), 4);
        file.read(reinterpret_cast<char*>(&jsonChunkType), 4);
        
        if (jsonChunkType != 0x4E4F534A)
        {
            LOG_ERROR("Invalid GLB file (expected JSON chunk)");
            return E_FAIL;
        }
        
        std::string jsonContent(jsonChunkLength, '\0');
        file.read(&jsonContent[0], jsonChunkLength);
        file.close();
        
        LOG_WARNING("GLB binary loading requires full GLTF parser - using basic import");
        return ParseGLTFJson(jsonContent, L"", OutModel, Options);
    }
    else
    {
        std::ifstream file(Path);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open GLTF file: " + StringUtils::WideToMultiByte(Path));
            return E_FAIL;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        file.close();
        
        size_t lastSlash = Path.find_last_of(L"\\/");
        std::wstring basePath = (lastSlash != std::wstring::npos) ? Path.substr(0, lastSlash + 1) : L"";
        
        return ParseGLTFJson(jsonContent, basePath, OutModel, Options);
    }
}

HRESULT KModelLoader::ParseGLTFJson(const std::string& JsonContent, const std::wstring& BasePath, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
    auto findJsonArray = [](const std::string& json, const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "[]";
        
        pos += searchKey.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) pos++;
        
        if (pos >= json.length() || json[pos] != '[') return "[]";
        
        int depth = 0;
        size_t start = pos;
        while (pos < json.length())
        {
            if (json[pos] == '[') depth++;
            else if (json[pos] == ']')
            {
                depth--;
                if (depth == 0) return json.substr(start, pos - start + 1);
            }
            else if (json[pos] == '"')
            {
                pos++;
                while (pos < json.length() && json[pos] != '"')
                {
                    if (json[pos] == '\\') pos++;
                    pos++;
                }
            }
            pos++;
        }
        return "[]";
    };
    
    auto parseVector3 = [](const std::string& str) -> XMFLOAT3 {
        XMFLOAT3 result(0, 0, 0);
        size_t start = str.find('[');
        size_t end = str.find(']');
        if (start == std::string::npos || end == std::string::npos) return result;
        
        std::string content = str.substr(start + 1, end - start - 1);
        std::istringstream iss(content);
        char comma;
        iss >> result.x >> comma >> result.y >> comma >> result.z;
        return result;
    };
    
    auto parseVector4 = [](const std::string& str) -> XMFLOAT4 {
        XMFLOAT4 result(0, 0, 0, 1);
        size_t start = str.find('[');
        size_t end = str.find(']');
        if (start == std::string::npos || end == std::string::npos) return result;
        
        std::string content = str.substr(start + 1, end - start - 1);
        std::istringstream iss(content);
        char comma;
        iss >> result.x >> comma >> result.y >> comma >> result.z >> comma >> result.w;
        return result;
    };
    
    auto getNumber = [](const std::string& json, const std::string& key, float defaultValue = 0.0f) -> float {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos += searchKey.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) pos++;
        
        if (pos >= json.length()) return defaultValue;
        
        try {
            return std::stof(json.substr(pos));
        } catch (...) {
            return defaultValue;
        }
    };
    
    auto getInteger = [](const std::string& json, const std::string& key, int defaultValue = 0) -> int {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos += searchKey.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) pos++;
        
        if (pos >= json.length()) return defaultValue;
        
        try {
            return std::stoi(json.substr(pos));
        } catch (...) {
            return defaultValue;
        }
    };

    LOG_INFO("Parsing GLTF JSON for model: " + OutModel->Name);
    
    std::vector<FVertex> allVertices;
    std::vector<uint32> allIndices;
    
    std::string accessorsJson = findJsonArray(JsonContent, "accessors");
    std::string bufferViewsJson = findJsonArray(JsonContent, "bufferViews");
    std::string buffersJson = findJsonArray(JsonContent, "buffers");
    std::string meshesJson = findJsonArray(JsonContent, "meshes");
    
    LOG_INFO("GLTF parsed (basic) - full implementation requires Assimp for complete support");
    
    LOG_WARNING("GLTF loading requires Assimp for full support - returning empty mesh");
    
    auto mesh = std::make_shared<KStaticMesh>();
    mesh->SetName(OutModel->Name);
    
    FVertex defaultVertex;
    defaultVertex.Position = XMFLOAT3(0, 0, 0);
    defaultVertex.Normal = XMFLOAT3(0, 1, 0);
    defaultVertex.TexCoord = XMFLOAT2(0, 0);
    defaultVertex.Color = XMFLOAT4(1, 1, 1, 1);
    
    std::vector<FVertex> placeholderVertices = { defaultVertex };
    std::vector<uint32> placeholderIndices = { 0 };
    
    mesh->AddLOD(placeholderVertices, placeholderIndices);
    mesh->CalculateBounds();
    
    OutModel->StaticMesh = mesh;
    
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
        if (staticMesh)
        {
            if (!OutModel->StaticMesh)
            {
                OutModel->StaticMesh = staticMesh;
            }
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
        meshName = OutModel->Name + "_Mesh";
    }
    staticMesh->SetName(meshName);

    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    std::vector<std::vector<std::pair<uint32, float>>> vertexBoneWeights;
    vertexBoneWeights.resize(mesh->mNumVertices);

    if (mesh->HasBones() && OutModel->Skeleton)
    {
        for (uint32 boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
        {
            aiBone* aiBone = mesh->mBones[boneIdx];
            std::string boneName = aiBone->mName.C_Str();
            
            int32 skelBoneIdx = OutModel->Skeleton->GetBoneIndex(boneName);
            if (skelBoneIdx == INVALID_BONE_INDEX) continue;
            
            for (uint32 weightIdx = 0; weightIdx < aiBone->mNumWeights; ++weightIdx)
            {
                uint32 vertexIdx = aiBone->mWeights[weightIdx].mVertexId;
                float weight = aiBone->mWeights[weightIdx].mWeight;
                
                if (vertexIdx < vertexBoneWeights.size())
                {
                    vertexBoneWeights[vertexIdx].push_back({static_cast<uint32>(skelBoneIdx), weight});
                }
            }
        }
        
        for (auto& weights : vertexBoneWeights)
        {
            std::sort(weights.begin(), weights.end(), 
                [](const std::pair<uint32, float>& a, const std::pair<uint32, float>& b) {
                    return a.second > b.second;
                });
            
            if (weights.size() > MAX_BONE_INFLUENCES)
            {
                float totalWeight = 0.0f;
                for (size_t i = 0; i < MAX_BONE_INFLUENCES; ++i)
                {
                    totalWeight += weights[i].second;
                }
                for (size_t i = 0; i < MAX_BONE_INFLUENCES; ++i)
                {
                    weights[i].second /= totalWeight;
                }
                weights.resize(MAX_BONE_INFLUENCES);
            }
        }
    }

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
            if (Options.bFlipUVs)
            {
                vertex.TexCoord.y = 1.0f - vertex.TexCoord.y;
            }
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

    staticMesh->AddLOD(vertices, indices);
    staticMesh->CalculateBounds();

    if (mesh->HasBones() && OutModel->Skeleton)
    {
        ProcessBones(mesh, OutModel, 0);
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

    if (!mesh->HasBones() || !OutModel->Skeleton)
    {
        return;
    }

    LOG_INFO("Processing " + std::to_string(mesh->mNumBones) + " bones for mesh");
#endif
}

void KModelLoader::ProcessAnimations(void* AssimpScene, FLoadedModel* OutModel)
{
#ifdef USE_ASSIMP
    const aiScene* scene = static_cast<const aiScene*>(AssimpScene);

    if (!OutModel->Skeleton)
    {
        LOG_WARNING("Cannot process animations without skeleton");
        return;
    }

    for (uint32 i = 0; i < scene->mNumAnimations; ++i)
    {
        aiAnimation* aiAnim = scene->mAnimations[i];
        
        auto animation = std::make_shared<KAnimation>();
        
        std::string animName = aiAnim->mName.C_Str();
        if (animName.empty())
        {
            animName = "Animation_" + std::to_string(i);
        }
        animation->SetName(animName);
        
        double ticksPerSecond = aiAnim->mTicksPerSecond;
        if (ticksPerSecond == 0.0)
        {
            ticksPerSecond = 25.0;
        }
        
        double duration = aiAnim->mDuration;
        
        animation->SetDuration(static_cast<float>(duration));
        animation->SetTicksPerSecond(static_cast<float>(ticksPerSecond));

        for (uint32 channelIdx = 0; channelIdx < aiAnim->mNumChannels; ++channelIdx)
        {
            aiNodeAnim* nodeAnim = aiAnim->mChannels[channelIdx];
            
            FAnimationChannel channel;
            channel.BoneName = nodeAnim->mNodeName.C_Str();
            
            int32 boneIdx = OutModel->Skeleton->GetBoneIndex(channel.BoneName);
            if (boneIdx == INVALID_BONE_INDEX)
            {
                continue;
            }
            channel.BoneIndex = static_cast<uint32>(boneIdx);
            
            for (uint32 keyIdx = 0; keyIdx < nodeAnim->mNumPositionKeys; ++keyIdx)
            {
                aiVectorKey& key = nodeAnim->mPositionKeys[keyIdx];
                FTransformKey transformKey;
                transformKey.Time = static_cast<float>(key.mTime);
                transformKey.Position = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
                channel.PositionKeys.push_back(transformKey);
            }
            
            for (uint32 keyIdx = 0; keyIdx < nodeAnim->mNumRotationKeys; ++keyIdx)
            {
                aiQuatKey& key = nodeAnim->mRotationKeys[keyIdx];
                FTransformKey transformKey;
                transformKey.Time = static_cast<float>(key.mTime);
                transformKey.Rotation = XMFLOAT4(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
                channel.RotationKeys.push_back(transformKey);
            }
            
            for (uint32 keyIdx = 0; keyIdx < nodeAnim->mNumScalingKeys; ++keyIdx)
            {
                aiVectorKey& key = nodeAnim->mScalingKeys[keyIdx];
                FTransformKey transformKey;
                transformKey.Time = static_cast<float>(key.mTime);
                transformKey.Scale = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
                channel.ScaleKeys.push_back(transformKey);
            }
            
            animation->AddChannel(channel);
        }
        
        animation->BuildBoneIndexMap(OutModel->Skeleton.get());
        OutModel->Animations.push_back(animation);
    }

    LOG_INFO("Processed " + std::to_string(OutModel->Animations.size()) + " animations");
#endif
}
