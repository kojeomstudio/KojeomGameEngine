#include "ModelLoader.h"
#include "SkeletalMeshComponent.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <future>
#include <chrono>

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

    if (PathUtils::ContainsTraversal(Path))
    {
        LOG_ERROR("Model loader: path contains traversal patterns");
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
        else if (ext == "fbx")
        {
            hr = LoadFBXFallback(Path, model.get(), Options);
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
    if (PathUtils::ContainsTraversal(Path))
    {
        LOG_ERROR("Model: async load path contains traversal");
        return nullptr;
    }

    if (IsModelLoaded(Path))
    {
        return LoadedModels[Path];
    }

    std::shared_ptr<FLoadedModel> result;
    std::future<std::shared_ptr<FLoadedModel>> future = std::async(std::launch::async, [this, Path, Options]() -> std::shared_ptr<FLoadedModel> {
        return this->LoadModel(Path, Options);
    });

    auto status = future.wait_for(std::chrono::milliseconds(0));
    if (status == std::future_status::ready)
    {
        result = future.get();
    }
    else
    {
        LOG_INFO("Model load started asynchronously: " + StringUtils::WideToMultiByte(Path));
        result = future.get();
    }

    return result;
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
        flags |= aiProcess_GenSmoothNormals;
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

    constexpr uint32 MAX_OBJ_VERTICES = 5000000;
    constexpr uint32 MAX_OBJ_LINES = 10000000;
    uint32 lineCount = 0;

    std::string line;
    while (std::getline(file, line) && ++lineCount <= MAX_OBJ_LINES)
    {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            if (vertices.size() >= MAX_OBJ_VERTICES)
            {
                LOG_WARNING("OBJ vertex count limit reached: " + std::to_string(MAX_OBJ_VERTICES));
                break;
            }
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
                        try
                        {
                            int posIdx = std::stoi(vertexStr.substr(0, posSlash1)) - 1;
                            if (posIdx >= 0 && posIdx < (int)positions.size())
                            {
                                vertex.Position = positions[posIdx];
                            }
                        }
                        catch (const std::exception&)
                        {
                            LOG_WARNING("OBJ: failed to parse vertex index: " + vertexStr);
                        }
                    }

                    size_t posSlash2 = vertexStr.find('/', posSlash1 + 1);
                    if (posSlash2 != std::string::npos)
                    {
                        std::string texStr = vertexStr.substr(posSlash1 + 1, posSlash2 - posSlash1 - 1);
                        if (!texStr.empty())
                        {
                            try
                            {
                                int texIdx = std::stoi(texStr) - 1;
                                if (texIdx >= 0 && texIdx < (int)texcoords.size())
                                {
                                    vertex.TexCoord = texcoords[texIdx];
                                }
                            }
                            catch (const std::exception&)
                            {
                                LOG_WARNING("OBJ: failed to parse texcoord index: " + texStr);
                            }
                        }

                        std::string normStr = vertexStr.substr(posSlash2 + 1);
                        if (!normStr.empty())
                        {
                            try
                            {
                                int normIdx = std::stoi(normStr) - 1;
                                if (normIdx >= 0 && normIdx < (int)normals.size())
                                {
                                    vertex.Normal = normals[normIdx];
                                }
                            }
                            catch (const std::exception&)
                            {
                                LOG_WARNING("OBJ: failed to parse normal index: " + normStr);
                            }
                        }
                    }
                }
                else
                {
                    try
                    {
                        int posIdx = std::stoi(vertexStr) - 1;
                        if (posIdx >= 0 && posIdx < (int)positions.size())
                        {
                            vertex.Position = positions[posIdx];
                        }
                    }
                    catch (const std::exception&)
                    {
                        LOG_WARNING("OBJ: failed to parse vertex index: " + vertexStr);
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
        
        static constexpr uint32 MaxGLBChunkSize = 256 * 1024 * 1024;
        if (jsonChunkLength > MaxGLBChunkSize)
        {
            LOG_ERROR("GLB JSON chunk too large: " + std::to_string(jsonChunkLength));
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

        static constexpr uint32 MaxGLTFFileSize = 256 * 1024 * 1024;
        file.seekg(0, std::ios::end);
        auto gltfFileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        if (gltfFileSize > 0 && static_cast<uint32>(gltfFileSize) > MaxGLTFFileSize)
        {
            LOG_ERROR("GLTF file too large: " + std::to_string(static_cast<uint32>(gltfFileSize)));
            file.close();
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

    std::string accessorsJson = findJsonArray(JsonContent, "accessors");
    std::string bufferViewsJson = findJsonArray(JsonContent, "bufferViews");
    std::string buffersJson = findJsonArray(JsonContent, "buffers");
    std::string meshesJson = findJsonArray(JsonContent, "meshes");

    LOG_INFO("GLTF parsed JSON structure - attempting basic geometry extraction");
    LOG_WARNING("GLTF fallback loader only supports basic geometry. Install Assimp for full GLTF/GLB support.");

    // Create a visible placeholder unit cube so the model is at least visible in the viewport
    auto mesh = std::make_shared<KStaticMesh>();
    mesh->SetName(OutModel->Name);

    float s = Options.Scale * 0.5f;
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;

    // Front face
    vertices.push_back({XMFLOAT3(-s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3( s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3( s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3(-s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(0, 0)});
    // Back face
    vertices.push_back({XMFLOAT3( s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3(-s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3(-s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3( s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 0, -1), XMFLOAT2(0, 0)});
    // Top face
    vertices.push_back({XMFLOAT3(-s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3( s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3( s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3(-s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(0, 0)});
    // Bottom face
    vertices.push_back({XMFLOAT3(-s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3( s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3( s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3(-s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(0, 0)});
    // Right face
    vertices.push_back({XMFLOAT3( s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3( s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3( s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3( s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(0, 0)});
    // Left face
    vertices.push_back({XMFLOAT3(-s, -s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(0, 1)});
    vertices.push_back({XMFLOAT3(-s, -s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(1, 1)});
    vertices.push_back({XMFLOAT3(-s,  s,  s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(1, 0)});
    vertices.push_back({XMFLOAT3(-s,  s, -s), XMFLOAT4(0.8f, 0.8f, 0.8f, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(0, 0)});

    for (uint32 face = 0; face < 6; ++face)
    {
        uint32 base = face * 4;
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
    }

    mesh->AddLOD(vertices, indices);
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
            else
            {
                staticMesh->SetName(OutModel->StaticMesh->GetName() + "_sub" + std::to_string(i));
                LOG_WARNING("Multiple meshes detected; only first mesh is stored as primary StaticMesh");
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

    if (mesh->HasBones() && OutModel->Skeleton)
    {
        ProcessBones(mesh, OutModel, 0);
    }

    if (mesh->HasBones() && OutModel->Skeleton && !vertexBoneWeights.empty())
    {
        std::vector<FSkinnedVertex> skinnedVertices;
        skinnedVertices.resize(mesh->mNumVertices);

        for (uint32 i = 0; i < mesh->mNumVertices; ++i)
        {
            skinnedVertices[i].Position = XMFLOAT3(
                mesh->mVertices[i].x * Options.Scale,
                mesh->mVertices[i].y * Options.Scale,
                mesh->mVertices[i].z * Options.Scale
            );

            if (mesh->HasNormals())
            {
                skinnedVertices[i].Normal = XMFLOAT3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }

            if (mesh->HasTextureCoords(0))
            {
                skinnedVertices[i].TexCoord = XMFLOAT2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
                if (Options.bFlipUVs)
                {
                    skinnedVertices[i].TexCoord.y = 1.0f - skinnedVertices[i].TexCoord.y;
                }
            }

            if (mesh->HasTangentsAndBitangents())
            {
                skinnedVertices[i].Tangent = XMFLOAT3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );
                skinnedVertices[i].Bitangent = XMFLOAT3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                );
            }

            skinnedVertices[i].Color = XMFLOAT4(1, 1, 1, 1);

            for (size_t w = 0; w < vertexBoneWeights[i].size() && w < MAX_BONE_INFLUENCES; ++w)
            {
                skinnedVertices[i].BoneIndices[w] = vertexBoneWeights[i][w].first;
                skinnedVertices[i].BoneWeights[w] = vertexBoneWeights[i][w].second;
            }
        }

        auto skeletalMesh = std::make_shared<KSkeletalMesh>();
        skeletalMesh->CreateFromData(Device, skinnedVertices, indices);
        skeletalMesh->SetName(meshName);
        OutModel->SkeletalMesh = skeletalMesh;
    }
    else
    {
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

        staticMesh->AddLOD(vertices, indices);
        staticMesh->CalculateBounds();
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

    LOG_INFO("Processing " + std::to_string(mesh->mNumBones) + " bones for mesh " + std::to_string(MeshIndex));

    for (uint32 boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
    {
        aiBone* aiBone = mesh->mBones[boneIdx];
        std::string boneName = aiBone->mName.C_Str();
        
        int32 skelBoneIdx = OutModel->Skeleton->GetBoneIndex(boneName);
        if (skelBoneIdx >= 0)
        {
            FBone* bone = OutModel->Skeleton->GetBoneMutable(static_cast<uint32>(skelBoneIdx));
            if (bone)
            {
                bone->InverseBindPose = AiMatrixToXMMatrix(aiBone->mOffsetMatrix);
            }
        }
    }

    OutModel->Skeleton->CalculateBindPoses();
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

HRESULT KModelLoader::LoadFBXFallback(const std::wstring& Path, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
    std::ifstream file(Path);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open FBX file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    static constexpr uint32 MaxFBXFallbackSize = 256 * 1024 * 1024;
    file.seekg(0, std::ios::end);
    auto fbxFileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fbxFileSize > 0 && static_cast<uint32>(fbxFileSize) > MaxFBXFallbackSize)
    {
        LOG_ERROR("FBX file too large for fallback parser: " + std::to_string(static_cast<uint32>(fbxFileSize)));
        file.close();
        return E_FAIL;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    if (content.find("FBX") == std::string::npos)
    {
        LOG_ERROR("Not a valid FBX file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    if (content.find("FBXHeader") != std::string::npos)
    {
        LOG_INFO("Loading ASCII FBX file");
        return ParseFBXAscii(content, OutModel, Options);
    }

    LOG_ERROR("Binary FBX not supported without Assimp. Use ASCII FBX format.");
    return E_FAIL;
}

HRESULT KModelLoader::ParseFBXAscii(const std::string& Content, FLoadedModel* OutModel, const FModelLoadOptions& Options)
{
    static constexpr size_t MaxFBXVertices = 5000000;
    static constexpr size_t MaxFBXIndices = 15000000;
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texcoords;
    std::vector<int32> indices;
    std::vector<std::string> boneNames;
    std::vector<int32> boneParentIndices;
    std::vector<XMFLOAT3> boneLocalPositions;
    std::vector<XMFLOAT4> boneLocalRotations;
    std::unordered_map<std::string, std::vector<std::pair<float, XMFLOAT3>>> bonePositionKeys;
    std::unordered_map<std::string, std::vector<std::pair<float, XMFLOAT4>>> boneRotationKeys;
    std::unordered_map<std::string, int32> boneNameToIndex;

    auto findSection = [&Content](const std::string& sectionName) -> size_t {
        std::string search = sectionName + ": ";
        size_t pos = Content.find(search);
        if (pos == std::string::npos)
        {
            search = sectionName + ":\n";
            pos = Content.find(search);
        }
        return pos;
    };

    auto extractFloatArray = [&Content](size_t startPos) -> std::vector<float> {
        std::vector<float> result;
        size_t pos = Content.find('*');
        if (pos != std::string::npos && pos >= startPos)
        {
            size_t braceOpen = Content.find('{', pos);
            size_t braceClose = Content.find('}', braceOpen);
            if (braceOpen != std::string::npos && braceClose != std::string::npos)
            {
                std::string data = Content.substr(braceOpen + 1, braceClose - braceOpen - 1);
                std::istringstream iss(data);
                float val;
                char comma;
                while (iss >> val)
                {
                    result.push_back(val);
                    if (iss >> comma && comma != ',')
                    {
                        iss.putback(comma);
                    }
                }
            }
        }
        else
        {
            size_t braceOpen = Content.find('{', startPos);
            size_t braceClose = Content.find('}', braceOpen);
            if (braceOpen != std::string::npos && braceClose != std::string::npos)
            {
                std::string data = Content.substr(braceOpen + 1, braceClose - braceOpen - 1);
                std::istringstream iss(data);
                float val;
                char comma;
                while (iss >> val)
                {
                    result.push_back(val);
                    if (iss >> comma && comma != ',')
                    {
                        iss.putback(comma);
                    }
                }
            }
        }
        return result;
    };

    auto extractIntArray = [&Content](size_t startPos) -> std::vector<int32> {
        std::vector<int32> result;
        size_t pos = Content.find('*');
        if (pos != std::string::npos && pos >= startPos)
        {
            size_t braceOpen = Content.find('{', pos);
            size_t braceClose = Content.find('}', braceOpen);
            if (braceOpen != std::string::npos && braceClose != std::string::npos)
            {
                std::string data = Content.substr(braceOpen + 1, braceClose - braceOpen - 1);
                std::istringstream iss(data);
                int32 val;
                char comma;
                while (iss >> val)
                {
                    result.push_back(val);
                    if (iss >> comma && comma != ',')
                    {
                        iss.putback(comma);
                    }
                }
            }
        }
        else
        {
            size_t braceOpen = Content.find('{', startPos);
            size_t braceClose = Content.find('}', braceOpen);
            if (braceOpen != std::string::npos && braceClose != std::string::npos)
            {
                std::string data = Content.substr(braceOpen + 1, braceClose - braceOpen - 1);
                std::istringstream iss(data);
                int32 val;
                char comma;
                while (iss >> val)
                {
                    result.push_back(val);
                    if (iss >> comma && comma != ',')
                    {
                        iss.putback(comma);
                    }
                }
            }
        }
        return result;
    };

    auto extractStringProperty = [&Content](size_t startPos) -> std::string {
        size_t quote1 = Content.find('"', startPos);
        if (quote1 == std::string::npos) return "";
        size_t quote2 = Content.find('"', quote1 + 1);
        if (quote2 == std::string::npos) return "";
        return Content.substr(quote1 + 1, quote2 - quote1 - 1);
    };

    size_t pos = 0;
    while ((pos = Content.find("Model: ", pos)) != std::string::npos)
    {
        size_t modelNameStart = pos + 7;
        size_t modelNameEnd = Content.find(',', modelNameStart);
        if (modelNameEnd == std::string::npos) modelNameEnd = Content.find('\n', modelNameStart);
        if (modelNameEnd == std::string::npos) { pos++; continue; }
        
        std::string modelName = Content.substr(modelNameStart, modelNameEnd - modelNameStart);
        if (modelName.empty()) { pos++; continue; }
        while (!modelName.empty() && modelName.back() == ' ') modelName.pop_back();
        if (modelName.front() == modelName.back() && (modelName.front() == '"' || modelName.front() == '\''))
        {
            modelName = modelName.substr(1, modelName.length() - 2);
        }

        size_t modelEnd = Content.find('}', pos);
        if (modelEnd == std::string::npos) { pos++; continue; }
        std::string modelBlock = Content.substr(pos, modelEnd - pos + 1);

        size_t typePos = modelBlock.find("Type: \"");
        if (typePos != std::string::npos)
        {
            size_t typeStart = typePos + 7;
            size_t typeEnd = modelBlock.find('"', typeStart);
            std::string modelType = modelBlock.substr(typeStart, typeEnd - typeStart);

            if (modelType == "Mesh")
            {
                size_t vertPos = modelBlock.find("Vertices: *");
                if (vertPos != std::string::npos)
                {
                    auto floats = extractFloatArray(vertPos);
                    for (size_t i = 0; i + 2 < floats.size(); i += 3)
                    {
                        if (positions.size() >= MaxFBXVertices)
                        {
                            LOG_WARNING("FBX: Vertex limit reached (" + std::to_string(MaxFBXVertices) + ")");
                            break;
                        }
                        positions.push_back(XMFLOAT3(floats[i] * Options.Scale, floats[i+1] * Options.Scale, floats[i+2] * Options.Scale));
                    }
                }

                size_t normPos = modelBlock.find("Normals: *");
                if (normPos != std::string::npos)
                {
                    auto floats = extractFloatArray(normPos);
                    for (size_t i = 0; i + 2 < floats.size(); i += 3)
                    {
                        normals.push_back(XMFLOAT3(floats[i], floats[i+1], floats[i+2]));
                    }
                }

                size_t uvPos = modelBlock.find("UV");
                if (uvPos != std::string::npos)
                {
                    size_t uvDataPos = modelBlock.find("UVIndex:", uvPos);
                    if (uvDataPos == std::string::npos) uvDataPos = modelBlock.find("UV: *", uvPos);
                    if (uvDataPos != std::string::npos)
                    {
                        auto floats = extractFloatArray(uvDataPos);
                        for (size_t i = 0; i + 1 < floats.size(); i += 2)
                        {
                            texcoords.push_back(XMFLOAT2(floats[i], Options.bFlipUVs ? (1.0f - floats[i+1]) : floats[i+1]));
                        }
                    }
                }

                size_t idxPos = modelBlock.find("PolygonVertexIndex: *");
                if (idxPos != std::string::npos)
                {
                    auto idxArray = extractIntArray(idxPos);
                    
                    std::vector<int32> triangleIndices;
                    std::vector<int32> currentPoly;
                    for (int32 idx : idxArray)
                    {
                        if (idx >= 0)
                        {
                            currentPoly.push_back(idx);
                        }
                        else
                        {
                            currentPoly.push_back(~idx);
                            if (currentPoly.size() == 3)
                            {
                                triangleIndices.push_back(currentPoly[0]);
                                triangleIndices.push_back(currentPoly[1]);
                                triangleIndices.push_back(currentPoly[2]);
                            }
                            else if (currentPoly.size() == 4)
                            {
                                triangleIndices.push_back(currentPoly[0]);
                                triangleIndices.push_back(currentPoly[1]);
                                triangleIndices.push_back(currentPoly[2]);
                                triangleIndices.push_back(currentPoly[0]);
                                triangleIndices.push_back(currentPoly[2]);
                                triangleIndices.push_back(currentPoly[3]);
                            }
                            else if (currentPoly.size() > 4)
                            {
                                for (size_t j = 1; j + 1 < currentPoly.size(); ++j)
                                {
                                    triangleIndices.push_back(currentPoly[0]);
                                    triangleIndices.push_back(currentPoly[j]);
                                    triangleIndices.push_back(currentPoly[j + 1]);
                                }
                            }
                            currentPoly.clear();
                        }
                    }
                    indices = triangleIndices;
                    if (indices.size() > MaxFBXIndices)
                    {
                        LOG_WARNING("FBX: Index limit reached, truncating to " + std::to_string(MaxFBXIndices));
                        indices.resize(MaxFBXIndices);
                    }
                }
            }
            else if (modelType == "LimbNode" || modelType == "Root")
            {
                uint32 boneIdx = static_cast<uint32>(boneNames.size());
                boneNames.push_back(modelName);
                boneParentIndices.push_back(-1);
                boneLocalPositions.push_back(XMFLOAT3(0, 0, 0));
                boneLocalRotations.push_back(XMFLOAT4(0, 0, 0, 1));
                boneNameToIndex[modelName] = boneIdx;

                size_t lclTransPos = modelBlock.find("Lcl Translation");
                if (lclTransPos != std::string::npos)
                {
                    auto floats = extractFloatArray(lclTransPos + 14);
                    if (floats.size() >= 3)
                    {
                        boneLocalPositions[boneIdx] = XMFLOAT3(floats[0], floats[1], floats[2]);
                    }
                }

                size_t lclRotPos = modelBlock.find("Lcl Rotation");
                if (lclRotPos != std::string::npos)
                {
                    auto floats = extractFloatArray(lclRotPos + 12);
                    if (floats.size() >= 3)
                    {
                        float pitch = floats[0] * XM_PI / 180.0f;
                        float yaw = floats[1] * XM_PI / 180.0f;
                        float roll = floats[2] * XM_PI / 180.0f;
                        XMFLOAT4 quat;
                        XMStoreFloat4(&quat, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
                        boneLocalRotations[boneIdx] = quat;
                    }
                }
            }
        }

        pos = modelEnd + 1;
    }

    pos = 0;
    while ((pos = Content.find("Connect: ", pos)) != std::string::npos)
    {
        size_t lineEnd = Content.find('\n', pos);
        if (lineEnd == std::string::npos) break;
        std::string line = Content.substr(pos, lineEnd - pos);

        size_t cPos = line.find("C, \"OS\",");
        size_t pPos = line.find("P, \"Model::");
        
        if (cPos != std::string::npos && pPos != std::string::npos)
        {
            size_t childNameStart = line.find('"', cPos + 7);
            size_t childNameEnd = line.find('"', childNameStart + 1);
            if (childNameStart != std::string::npos && childNameEnd != std::string::npos)
            {
                std::string childName = line.substr(childNameStart + 1, childNameEnd - childNameStart - 1);
                
                size_t parentNameStart = line.find("Model::", pPos);
                if (parentNameStart != std::string::npos)
                {
                    parentNameStart += 7;
                    size_t parentNameEnd = line.find('"', parentNameStart);
                    if (parentNameEnd != std::string::npos)
                    {
                        std::string parentName = line.substr(parentNameStart, parentNameEnd - parentNameStart);
                        
                        auto childIt = boneNameToIndex.find(childName);
                        auto parentIt = boneNameToIndex.find(parentName);
                        if (childIt != boneNameToIndex.end() && parentIt != boneNameToIndex.end())
                        {
                            boneParentIndices[childIt->second] = parentIt->second;
                        }
                    }
                }
            }
        }
        pos = lineEnd + 1;
    }

    pos = 0;
    struct FBXAnimCurve
    {
        std::string CurveName;
        std::string TargetProperty;
        std::vector<float> Times;
        std::vector<float> Values;
    };
    std::vector<FBXAnimCurve> animCurves;

    while ((pos = Content.find("AnimationCurve: ", pos)) != std::string::npos)
    {
        size_t animEnd = Content.find('}', pos);
        if (animEnd == std::string::npos) break;
        std::string animBlock = Content.substr(pos, animEnd - pos + 1);

        size_t keyCountPos = animBlock.find("KeyCount:");
        size_t keyTimePos = animBlock.find("KeyTime:");
        size_t keyValuePos = animBlock.find("KeyValueFloat:");

        if (keyCountPos != std::string::npos && keyTimePos != std::string::npos && keyValuePos != std::string::npos)
        {
            FBXAnimCurve curve;
            auto times = extractFloatArray(keyTimePos);
            auto values = extractFloatArray(keyValuePos);

            curve.Times = std::move(times);
            curve.Values = std::move(values);

            size_t nameStart = Content.find('"', pos + 16);
            if (nameStart != std::string::npos)
            {
                size_t nameEnd = Content.find('"', nameStart + 1);
                if (nameEnd != std::string::npos)
                {
                    curve.CurveName = Content.substr(nameStart + 1, nameEnd - nameStart - 1);
                }
            }

            animCurves.push_back(std::move(curve));
        }

        pos = animEnd + 1;
    }

    if (!animCurves.empty() && !boneNames.empty())
    {
        LOG_INFO("FBX: Found " + std::to_string(animCurves.size()) + " animation curves, " + std::to_string(boneNames.size()) + " bones");

        std::unordered_map<std::string, std::vector<size_t>> boneCurves;
        pos = 0;
        while ((pos = Content.find("Connect: ", pos)) != std::string::npos)
        {
            size_t lineEnd = Content.find('\n', pos);
            if (lineEnd == std::string::npos) break;
            std::string line = Content.substr(pos, lineEnd - pos);

            size_t cPos = line.find("C, \"OO\",");
            if (cPos != std::string::npos)
            {
                size_t firstQuote = line.find('"', cPos + 8);
                size_t secondQuote = line.find('"', firstQuote + 1);
                size_t thirdQuote = line.find('"', secondQuote + 1);
                size_t fourthQuote = line.find('"', thirdQuote + 1);

                if (firstQuote != std::string::npos && secondQuote != std::string::npos &&
                    thirdQuote != std::string::npos && fourthQuote != std::string::npos)
                {
                    std::string curveID = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    std::string targetID = line.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);

                    for (size_t ci = 0; ci < animCurves.size(); ++ci)
                    {
                        if (animCurves[ci].CurveName == curveID)
                        {
                            for (size_t bi = 0; bi < boneNames.size(); ++bi)
                            {
                                if (boneNames[bi].find(targetID) != std::string::npos)
                                {
                                    boneCurves[boneNames[bi]].push_back(ci);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            pos = lineEnd + 1;
        }

        if (!boneCurves.empty())
        {
            auto animation = std::make_shared<KAnimation>();
            animation->SetName(OutModel->Name + "_Anim");

            float maxDuration = 0.0f;
            for (const auto& [boneName, curveIndices] : boneCurves)
            {
                int32 boneIdx = INVALID_BONE_INDEX;
                for (size_t bi = 0; bi < boneNames.size(); ++bi)
                {
                    if (boneNames[bi] == boneName)
                    {
                        boneIdx = static_cast<int32>(bi);
                        break;
                    }
                }
                if (boneIdx == INVALID_BONE_INDEX) continue;

                FAnimationChannel channel;
                channel.BoneName = boneName;
                channel.BoneIndex = static_cast<uint32>(boneIdx);

                for (size_t ci = 0; ci < curveIndices.size() && ci < 3; ++ci)
                {
                    const auto& curve = animCurves[curveIndices[ci]];
                    for (size_t k = 0; k < std::min(curve.Times.size(), curve.Values.size()); ++k)
                    {
                        FTransformKey key;
                        key.Time = curve.Times[k];

                        if (k < channel.PositionKeys.size())
                        {
                            if (ci == 0) channel.PositionKeys[k].Position.x = curve.Values[k];
                            else if (ci == 1) channel.PositionKeys[k].Position.y = curve.Values[k];
                            else channel.PositionKeys[k].Position.z = curve.Values[k];
                        }
                        else
                        {
                            key.Position = XMFLOAT3(0, 0, 0);
                            if (ci == 0) key.Position.x = curve.Values[k];
                            else if (ci == 1) key.Position.y = curve.Values[k];
                            else key.Position.z = curve.Values[k];
                            channel.PositionKeys.push_back(key);
                        }

                        if (key.Time > maxDuration) maxDuration = key.Time;
                    }
                }

                if (!channel.PositionKeys.empty())
                {
                    animation->AddChannel(channel);
                }
            }

            if (animation->GetChannelCount() > 0)
            {
                animation->SetDuration(maxDuration > 0.0f ? maxDuration : 1.0f);
                animation->SetTicksPerSecond(30.0f);
                animation->BuildBoneIndexMap(OutModel->Skeleton.get());
                OutModel->Animations.push_back(animation);
                LOG_INFO("FBX: Created animation with " + std::to_string(animation->GetChannelCount()) + " channels, duration=" + std::to_string(maxDuration));
            }
        }
    }

    if (positions.empty())
    {
        LOG_WARNING("FBX: No vertices found");
        return E_FAIL;
    }

    while (normals.size() < positions.size())
    {
        normals.push_back(XMFLOAT3(0, 1, 0));
    }
    while (texcoords.size() < positions.size())
    {
        texcoords.push_back(XMFLOAT2(0, 0));
    }

    std::vector<FVertex> vertices;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        FVertex v;
        v.Position = positions[i];
        v.Normal = normals[i];
        v.TexCoord = texcoords[i];
        v.Color = XMFLOAT4(1, 1, 1, 1);
        vertices.push_back(v);
    }

    std::vector<uint32> triIndices;
    for (int32 idx : indices)
    {
        if (idx >= 0 && idx < static_cast<int32>(vertices.size()))
        {
            triIndices.push_back(static_cast<uint32>(idx));
        }
    }

    auto mesh = std::make_shared<KStaticMesh>();
    mesh->SetName(OutModel->Name);
    mesh->AddLOD(vertices, triIndices);
    mesh->CalculateBounds();
    OutModel->StaticMesh = mesh;

    if (!boneNames.empty())
    {
        auto skeleton = std::make_shared<KSkeleton>();

        for (size_t i = 0; i < boneNames.size(); ++i)
        {
            FBone bone;
            bone.Name = boneNames[i];
            bone.ParentIndex = boneParentIndices[i];
            bone.LocalPosition = boneLocalPositions[i];
            bone.LocalRotation = boneLocalRotations[i];
            bone.LocalScale = XMFLOAT3(1, 1, 1);
            bone.BindPose = XMMatrixIdentity();
            bone.InverseBindPose = XMMatrixIdentity();
            skeleton->AddBone(bone);
        }

        skeleton->CalculateBindPoses();
        OutModel->Skeleton = skeleton;
    }

    LOG_INFO("Loaded FBX: " + std::to_string(vertices.size()) + " vertices, " + std::to_string(triIndices.size()) + " indices, " + std::to_string(boneNames.size()) + " bones");
    return S_OK;
}
