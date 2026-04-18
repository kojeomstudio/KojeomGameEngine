#define NOMINMAX
#include "EngineAPI.h"
#include <new>
#include <unordered_map>
#include <Engine/Core/Engine.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Light.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Assets/ModelLoader.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/Actor.h>
#include <Engine/Assets/StaticMeshComponent.h>
#include <Engine/Assets/SkeletalMeshComponent.h>
#include <Engine/Assets/LightComponent.h>
#include <Engine/Graphics/Debug/DebugRenderer.h>

class FEngineWrapper
{
public:
    std::unique_ptr<KEngine> Engine;
    std::unique_ptr<KModelLoader> ModelLoader;
    std::unique_ptr<KDebugRenderer> DebugRenderer;

    FEngineWrapper()
    {
        Engine = std::make_unique<KEngine>();
        ModelLoader = std::make_unique<KModelLoader>();
        DebugRenderer = std::make_unique<KDebugRenderer>();
    }

    ~FEngineWrapper()
    {
        DebugRenderer.reset();
        ModelLoader.reset();
        Engine.reset();
    }

    HRESULT InitializeDebugRenderer()
    {
        if (!DebugRenderer || !Engine) return E_FAIL;
        if (DebugRenderer->IsInitialized()) return S_OK;
        KGraphicsDevice* device = Engine->GetGraphicsDevice();
        if (!device) return E_FAIL;
        return DebugRenderer->Initialize(device);
    }
};

extern "C"
{
    static FEngineWrapper* g_EngineWrapper = nullptr;

    ENGINEAPI void* Engine_Create()
    {
        try
        {
            g_EngineWrapper = new (std::nothrow) FEngineWrapper();
            return g_EngineWrapper;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    ENGINEAPI void Engine_Destroy(void* engine)
    {
        if (engine)
        {
            delete static_cast<FEngineWrapper*>(engine);
            if (g_EngineWrapper == static_cast<FEngineWrapper*>(engine))
            {
                g_EngineWrapper = nullptr;
            }
        }
    }

    ENGINEAPI HRESULT Engine_Initialize(void* engine, HWND hwnd, int width, int height)
    {
        if (!engine) return E_INVALIDARG;
        if (!hwnd) return E_INVALIDARG;
        if (width <= 0 || height <= 0) return E_INVALIDARG;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        
        HINSTANCE hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
        HRESULT hr = wrapper->Engine->Initialize(hInst, L"KojeomEditor", width, height);
        if (SUCCEEDED(hr))
        {
            wrapper->ModelLoader->Initialize();
            wrapper->InitializeDebugRenderer();
        }
        return hr;
    }

    ENGINEAPI HRESULT Engine_InitializeEmbedded(void* engine, HWND hwnd, int width, int height)
    {
        if (!engine) return E_INVALIDARG;
        if (!hwnd) return E_INVALIDARG;
        if (width <= 0 || height <= 0) return E_INVALIDARG;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        
        HRESULT hr = wrapper->Engine->InitializeWithExternalHwnd(hwnd, width, height);
        if (SUCCEEDED(hr))
        {
            wrapper->ModelLoader->Initialize();
            wrapper->InitializeDebugRenderer();
        }
        return hr;
    }

    ENGINEAPI void Engine_Tick(void* engine, float deltaTime)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        wrapper->Engine->Update(deltaTime);
    }

    ENGINEAPI void Engine_Render(void* engine)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        wrapper->Engine->Render();
    }

    ENGINEAPI void Engine_Resize(void* engine, int width, int height)
    {
        if (!engine) return;
        if (width <= 0 || height <= 0) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        wrapper->Engine->OnResize(width, height);
    }

    ENGINEAPI void* Engine_GetSceneManager(void* engine)
    {
        if (!engine) return nullptr;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        return &wrapper->Engine->GetSceneManager();
    }

    ENGINEAPI void* Engine_GetRenderer(void* engine)
    {
        if (!engine) return nullptr;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        return wrapper->Engine->GetRenderer();
    }

    ENGINEAPI void* Scene_Create(void* sceneMgr, const char* name)
    {
        if (!sceneMgr || !name) return nullptr;
        if (strlen(name) > 256) return nullptr;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        auto scene = mgr->CreateScene(std::string(name));
        if (!scene) return nullptr;
        return scene.get();
    }

    ENGINEAPI void* Scene_Load(void* sceneMgr, const wchar_t* path)
    {
        if (!sceneMgr || !path) return nullptr;
        if (PathUtils::ContainsTraversal(std::wstring(path)))
        {
            LOG_ERROR("Scene path rejected (unsafe path)");
            return nullptr;
        }
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        return mgr->LoadScene(std::wstring(path)).get();
    }

    ENGINEAPI HRESULT Scene_Save(void* sceneMgr, const wchar_t* path, void* scene)
    {
        if (!sceneMgr || !path || !scene) return E_INVALIDARG;
        if (PathUtils::ContainsTraversal(std::wstring(path)))
        {
            LOG_ERROR("Scene path rejected (unsafe path)");
            return E_INVALIDARG;
        }
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        KScene* rawScene = static_cast<KScene*>(scene);

        const auto& allScenes = mgr->GetAllScenes();
        for (const auto& pair : allScenes)
        {
            if (pair.second.get() == rawScene)
            {
                return mgr->SaveScene(std::wstring(path), pair.second);
            }
        }

        LOG_WARNING("Scene_Save: Scene not found in manager, attempting save with non-owning reference");
        auto scenePtr = std::shared_ptr<KScene>(rawScene, [](KScene*) {});
        return mgr->SaveScene(std::wstring(path), scenePtr);
    }

    ENGINEAPI void Scene_SetActive(void* sceneMgr, void* scene)
    {
        if (!sceneMgr || !scene) return;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        KScene* rawScene = static_cast<KScene*>(scene);

        const auto& allScenes = mgr->GetAllScenes();
        for (const auto& pair : allScenes)
        {
            if (pair.second.get() == rawScene)
            {
                mgr->SetActiveScene(pair.second);
                return;
            }
        }

        LOG_WARNING("Scene_SetActive: Scene not found in manager");
    }

    ENGINEAPI void* Scene_GetActive(void* sceneMgr)
    {
        if (!sceneMgr) return nullptr;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        return mgr->GetActiveScene().get();
    }

    ENGINEAPI void* Actor_Create(void* scene, const char* name)
    {
        if (!scene || !name) return nullptr;
        if (strlen(name) > 256) return nullptr;
        KScene* kscene = static_cast<KScene*>(scene);
        return kscene->CreateActor(std::string(name)).get();
    }

    ENGINEAPI void Actor_Destroy(void* scene, void* actor)
    {
        if (!scene || !actor) return;
        KScene* kscene = static_cast<KScene*>(scene);
        KActor* kactor = static_cast<KActor*>(actor);
        kscene->RemoveActor(kactor);
    }

    ENGINEAPI void Actor_SetPosition(void* actor, float x, float y, float z)
    {
        if (!actor) return;
        KActor* kactor = static_cast<KActor*>(actor);
        kactor->SetWorldPosition(XMFLOAT3(x, y, z));
    }

    ENGINEAPI void Actor_SetRotation(void* actor, float x, float y, float z, float w)
    {
        if (!actor) return;
        KActor* kactor = static_cast<KActor*>(actor);
        kactor->SetWorldRotation(XMFLOAT4(x, y, z, w));
    }

    ENGINEAPI void Actor_SetScale(void* actor, float x, float y, float z)
    {
        if (!actor) return;
        KActor* kactor = static_cast<KActor*>(actor);
        kactor->SetWorldScale(XMFLOAT3(x, y, z));
    }

    ENGINEAPI void Actor_GetPosition(void* actor, float* x, float* y, float* z)
    {
        if (!actor || !x || !y || !z) return;
        KActor* kactor = static_cast<KActor*>(actor);
        const FTransform& transform = kactor->GetWorldTransform();
        *x = transform.Position.x;
        *y = transform.Position.y;
        *z = transform.Position.z;
    }

    ENGINEAPI void Actor_GetRotation(void* actor, float* x, float* y, float* z, float* w)
    {
        if (!actor || !x || !y || !z || !w) return;
        KActor* kactor = static_cast<KActor*>(actor);
        const FTransform& transform = kactor->GetWorldTransform();
        *x = transform.Rotation.x;
        *y = transform.Rotation.y;
        *z = transform.Rotation.z;
        *w = transform.Rotation.w;
    }

    ENGINEAPI void Actor_GetScale(void* actor, float* x, float* y, float* z)
    {
        if (!actor || !x || !y || !z) return;
        KActor* kactor = static_cast<KActor*>(actor);
        const FTransform& transform = kactor->GetWorldTransform();
        *x = transform.Scale.x;
        *y = transform.Scale.y;
        *z = transform.Scale.z;
    }

    ENGINEAPI const char* Actor_GetName(void* actor)
    {
        if (!actor) return "";
        KActor* kactor = static_cast<KActor*>(actor);
        static thread_local std::string nameBuffer;
        nameBuffer = kactor->GetName();
        return nameBuffer.c_str();
    }

    ENGINEAPI void Actor_SetName(void* actor, const char* name)
    {
        if (!actor || !name) return;
        if (strlen(name) > 256) return;
        KActor* kactor = static_cast<KActor*>(actor);
        kactor->SetName(std::string(name));
    }

    ENGINEAPI void* Camera_GetMain(void* engine)
    {
        if (!engine) return nullptr;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        return wrapper->Engine->GetCamera();
    }

    ENGINEAPI void Camera_SetPosition(void* camera, float x, float y, float z)
    {
        if (!camera) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        kcam->SetPosition(x, y, z);
    }

    ENGINEAPI void Camera_SetRotation(void* camera, float pitch, float yaw, float roll)
    {
        if (!camera) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        kcam->SetRotation(pitch, yaw, roll);
    }

    ENGINEAPI void Camera_GetPosition(void* camera, float* x, float* y, float* z)
    {
        if (!camera || !x || !y || !z) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        XMFLOAT3 pos = kcam->GetPosition();
        *x = pos.x;
        *y = pos.y;
        *z = pos.z;
    }

    ENGINEAPI void Camera_GetViewMatrix(void* camera, float* matrix)
    {
        if (!camera || !matrix) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        XMMATRIX view = kcam->GetViewMatrix();
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(matrix), view);
    }

    ENGINEAPI void Camera_GetProjectionMatrix(void* camera, float* matrix)
    {
        if (!camera || !matrix) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        XMMATRIX proj = kcam->GetProjectionMatrix();
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(matrix), proj);
    }

ENGINEAPI void* Model_Load(void* engine, const wchar_t* path)
{
    if (!engine || !path) return nullptr;
    if (PathUtils::ContainsTraversal(std::wstring(path))) return nullptr;
    FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
    auto model = wrapper->ModelLoader->LoadModel(std::wstring(path));
    if (!model) return nullptr;
    return model.get();
}

    ENGINEAPI void Model_Unload(void* engine, const wchar_t* path)
    {
        if (!engine || !path) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        wrapper->ModelLoader->UnloadModel(std::wstring(path));
    }

    ENGINEAPI void Renderer_SetRenderPath(void* renderer, int path)
    {
        if (!renderer) return;
        if (path < 0 || path > 1) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetRenderPath(static_cast<ERenderPath>(path));
    }

    ENGINEAPI void Renderer_SetDebugMode(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetDebugMode(enabled);
    }

    ENGINEAPI void Renderer_GetStats(void* renderer, int* drawCalls, int* vertexCount, float* frameTime)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        if (drawCalls) *drawCalls = krenderer->GetDrawCallCount();
        if (vertexCount) *vertexCount = krenderer->GetVertexCount();
        if (frameTime) *frameTime = krenderer->GetFrameTime();
    }

    ENGINEAPI void* Scene_Raycast(void* scene, float originX, float originY, float originZ,
                                  float dirX, float dirY, float dirZ,
                                  float* outHitX, float* outHitY, float* outHitZ)
    {
        if (!scene) return nullptr;
        KScene* kscene = static_cast<KScene*>(scene);
        
        XMVECTOR origin = XMVectorSet(originX, originY, originZ, 1.0f);
        XMVECTOR direction = XMVectorSet(dirX, dirY, dirZ, 0.0f);
        direction = XMVector3Normalize(direction);
        
        float closestDistance = FLT_MAX;
        KActor* closestActor = nullptr;
        XMVECTOR closestHitPoint = XMVectorZero();
        
        const auto& actors = kscene->GetActors();
        for (const auto& actorPtr : actors)
        {
            KActor* actor = actorPtr.get();
            if (!actor) continue;
            
            const FTransform& transform = actor->GetWorldTransform();
            XMVECTOR actorPos = XMLoadFloat3(&transform.Position);
            float boundsRadius = std::max({ transform.Scale.x, transform.Scale.y, transform.Scale.z }) * 0.5f;
            
            XMVECTOR toActor = actorPos - origin;
            float t = XMVectorGetX(XMVector3Dot(toActor, direction));
            
            if (t < 0) continue;
            
            XMVECTOR closestPoint = origin + direction * t;
            float distanceSq = XMVectorGetX(XMVector3LengthSq(closestPoint - actorPos));
            float radiusSq = boundsRadius * boundsRadius;
            
            if (distanceSq <= radiusSq && t < closestDistance)
            {
                closestDistance = t;
                closestActor = actor;
                closestHitPoint = closestPoint;
            }
        }
        
        if (closestActor && outHitX && outHitY && outHitZ)
        {
            XMFLOAT3 hitPoint;
            XMStoreFloat3(&hitPoint, closestHitPoint);
            *outHitX = hitPoint.x;
            *outHitY = hitPoint.y;
            *outHitZ = hitPoint.z;
        }
        
        return closestActor;
    }

    ENGINEAPI int Scene_GetActorCount(void* scene)
    {
        if (!scene) return 0;
        KScene* kscene = static_cast<KScene*>(scene);
        return static_cast<int>(kscene->GetActors().size());
    }

    ENGINEAPI void* Scene_GetActorAt(void* scene, int index)
    {
        if (!scene) return nullptr;
        KScene* kscene = static_cast<KScene*>(scene);
        const auto& actors = kscene->GetActors();
        if (index >= 0 && index < static_cast<int>(actors.size()))
        {
            return actors[index].get();
        }
        return nullptr;
    }

    ENGINEAPI void Actor_GetComponentCount(void* actor, int* outCount)
    {
        if (!actor || !outCount) return;
        KActor* kactor = static_cast<KActor*>(actor);
        *outCount = static_cast<int>(kactor->GetComponents().size());
    }

    ENGINEAPI const char* Actor_GetComponentName(void* actor, int index)
    {
        if (!actor) return "";
        KActor* kactor = static_cast<KActor*>(actor);
        const auto& components = kactor->GetComponents();
        if (index >= 0 && index < static_cast<int>(components.size()))
        {
            KActorComponent* comp = components[index].get();
            if (auto* smc = dynamic_cast<KStaticMeshComponent*>(comp)) return "StaticMesh";
            if (auto* skmc = dynamic_cast<KSkeletalMeshComponent*>(comp)) return "SkeletalMesh";
            return "Component";
        }
        return "";
    }

    ENGINEAPI int Actor_GetComponentType(void* actor, int index)
    {
        if (!actor) return 0;
        KActor* kactor = static_cast<KActor*>(actor);
        const auto& components = kactor->GetComponents();
        if (index >= 0 && index < static_cast<int>(components.size()))
        {
            KActorComponent* comp = components[index].get();
            return static_cast<int>(comp->GetComponentTypeID());
        }
        return 0;
    }

    ENGINEAPI void Material_SetAlbedo(void* material, float r, float g, float b, float a)
    {
        if (!material) return;
        KMaterial* mat = static_cast<KMaterial*>(material);
        mat->SetAlbedoColor(XMFLOAT4(r, g, b, a));
    }

    ENGINEAPI void Material_SetMetallic(void* material, float value)
    {
        if (!material) return;
        KMaterial* mat = static_cast<KMaterial*>(material);
        mat->SetMetallic(value);
    }

    ENGINEAPI void Material_SetRoughness(void* material, float value)
    {
        if (!material) return;
        KMaterial* mat = static_cast<KMaterial*>(material);
        mat->SetRoughness(value);
    }

    ENGINEAPI void Material_SetAO(void* material, float value)
    {
        if (!material) return;
        KMaterial* mat = static_cast<KMaterial*>(material);
        mat->SetAO(value);
    }

    ENGINEAPI void Material_GetAlbedo(void* material, float* r, float* g, float* b, float* a)
    {
        if (!material || !r || !g || !b || !a) return;
        KMaterial* mat = static_cast<KMaterial*>(material);
        const FPBRMaterialParams& params = mat->GetParams();
        *r = params.AlbedoColor.x;
        *g = params.AlbedoColor.y;
        *b = params.AlbedoColor.z;
        *a = params.AlbedoColor.w;
    }

    ENGINEAPI float Material_GetMetallic(void* material)
    {
        if (!material) return 0.0f;
        KMaterial* mat = static_cast<KMaterial*>(material);
        return mat->GetParams().Metallic;
    }

    ENGINEAPI float Material_GetRoughness(void* material)
    {
        if (!material) return 0.0f;
        KMaterial* mat = static_cast<KMaterial*>(material);
        return mat->GetParams().Roughness;
    }

    ENGINEAPI float Material_GetAO(void* material)
    {
        if (!material) return 0.0f;
        KMaterial* mat = static_cast<KMaterial*>(material);
        return mat->GetParams().AO;
    }

    ENGINEAPI void* Actor_AddComponent(void* actor, int componentType)
    {
        if (!actor) return nullptr;
        if (componentType < 0 || componentType > 5) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        ComponentPtr newComp = CreateComponentByType(static_cast<EComponentType>(componentType));
        if (!newComp)
        {
            newComp = std::make_shared<KActorComponent>();
        }
        kactor->AddComponent(newComp);
        return newComp.get();
    }

    ENGINEAPI void* Actor_GetStaticMeshComponent(void* actor)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        return kactor->GetComponent<KStaticMeshComponent>();
    }

    ENGINEAPI void* Actor_GetSkeletalMeshComponent(void* actor)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        return kactor->GetComponent<KSkeletalMeshComponent>();
    }

    ENGINEAPI void* Actor_GetLightComponent(void* actor)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        return kactor->GetComponent<KLightComponent>();
    }

    ENGINEAPI void* StaticMeshComponent_SetMesh(void* component, const wchar_t* meshPath)
    {
        if (!component) return nullptr;
        KStaticMeshComponent* smc = static_cast<KStaticMeshComponent*>(component);
        
        if (g_EngineWrapper && g_EngineWrapper->ModelLoader && meshPath && wcslen(meshPath) > 0)
        {
            if (PathUtils::ContainsTraversal(std::wstring(meshPath))) return nullptr;
            auto model = g_EngineWrapper->ModelLoader->LoadModel(std::wstring(meshPath));
            if (model && model->StaticMesh)
            {
                smc->SetStaticMesh(model->StaticMesh);
                return model->StaticMesh.get();
            }
        }
        
        auto staticMesh = std::make_shared<KStaticMesh>();
        std::vector<FVertex> vertices;
        std::vector<uint32> indices;
        for (uint32 i = 0; i < 24; ++i)
        {
            vertices.push_back(FVertex());
        }
        staticMesh->AddLOD(vertices, indices);
        smc->SetStaticMesh(staticMesh);
        return staticMesh.get();
    }

    ENGINEAPI void* StaticMeshComponent_GetMaterial(void* component)
    {
        if (!component) return nullptr;
        KStaticMeshComponent* smc = static_cast<KStaticMeshComponent*>(component);
        return smc->GetMaterial();
    }

    ENGINEAPI void* StaticMeshComponent_CreateDefaultMesh(void* component)
    {
        if (!component) return nullptr;
        KStaticMeshComponent* smc = static_cast<KStaticMeshComponent*>(component);
        auto staticMesh = std::make_shared<KStaticMesh>();

        const XMFLOAT4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
        const XMFLOAT3 NUp   = {  0, 1, 0 };
        const XMFLOAT3 NDown = {  0,-1, 0 };
        const XMFLOAT3 NFwd  = {  0, 0,-1 };
        const XMFLOAT3 NBack = {  0, 0, 1 };
        const XMFLOAT3 NLeft = { -1, 0, 0 };
        const XMFLOAT3 NRgt  = {  1, 0, 0 };

        std::vector<FVertex> vertices = {
            {{-1, 1,-1}, White, NUp,   {0,0}}, {{ 1, 1,-1}, White, NUp,   {1,0}},
            {{ 1, 1, 1}, White, NUp,   {1,1}}, {{-1, 1, 1}, White, NUp,   {0,1}},
            {{-1,-1, 1}, White, NDown, {0,0}}, {{ 1,-1, 1}, White, NDown, {1,0}},
            {{ 1,-1,-1}, White, NDown, {1,1}}, {{-1,-1,-1}, White, NDown, {0,1}},
            {{-1, 1,-1}, White, NFwd,  {0,0}}, {{ 1, 1,-1}, White, NFwd,  {1,0}},
            {{ 1,-1,-1}, White, NFwd,  {1,1}}, {{-1,-1,-1}, White, NFwd,  {0,1}},
            {{ 1, 1, 1}, White, NBack, {0,0}}, {{-1, 1, 1}, White, NBack, {1,0}},
            {{-1,-1, 1}, White, NBack, {1,1}}, {{ 1,-1, 1}, White, NBack, {0,1}},
            {{-1, 1, 1}, White, NLeft, {0,0}}, {{-1, 1,-1}, White, NLeft, {1,0}},
            {{-1,-1,-1}, White, NLeft, {1,1}}, {{-1,-1, 1}, White, NLeft, {0,1}},
            {{ 1, 1,-1}, White, NRgt,  {0,0}}, {{ 1, 1, 1}, White, NRgt,  {1,0}},
            {{ 1,-1, 1}, White, NRgt,  {1,1}}, {{ 1,-1,-1}, White, NRgt,  {0,1}},
        };
        std::vector<uint32> indices = {
            0, 2, 1,   3, 2, 0,
            4, 6, 5,   4, 7, 6,
            8, 9,10,   8,10,11,
           12,13,14,  12,14,15,
           16,17,18,  16,18,19,
           20,21,22,  20,22,23,
        };
        staticMesh->AddLOD(vertices, indices);
        staticMesh->CalculateBounds();
        smc->SetStaticMesh(staticMesh);
        return staticMesh.get();
    }

    ENGINEAPI void* SkeletalMeshComponent_PlayAnimation(void* component, const char* animName)
    {
        if (!component) return nullptr;
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        if (animName)
        {
            skmc->PlayAnimation(std::string(animName));
        }
        return component;
    }

    ENGINEAPI void SkeletalMeshComponent_StopAnimation(void* component)
    {
        if (!component) return;
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        skmc->StopAnimation();
    }

    ENGINEAPI int SkeletalMeshComponent_GetAnimationCount(void* component)
    {
        if (!component) return 0;
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        return static_cast<int>(skmc->GetAnimationCount());
    }

    ENGINEAPI void Actor_SetVisibility(void* actor, bool visible)
    {
        if (!actor) return;
        KActor* kactor = static_cast<KActor*>(actor);
        kactor->SetVisible(visible);
    }

    ENGINEAPI bool Actor_IsVisible(void* actor)
    {
        if (!actor) return false;
        KActor* kactor = static_cast<KActor*>(actor);
        return kactor->IsVisible();
    }

ENGINEAPI void* Model_LoadAndGetStaticMesh(void* engine, const wchar_t* path)
{
    if (!engine || !path) return nullptr;
    if (PathUtils::ContainsTraversal(std::wstring(path))) return nullptr;
    FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
    auto model = wrapper->ModelLoader->LoadModel(std::wstring(path));
    if (model && model->StaticMesh)
    {
        return model->StaticMesh.get();
    }
    return nullptr;
}

ENGINEAPI void* Model_LoadAndGetSkeletalMesh(void* engine, const wchar_t* path)
{
    if (!engine || !path) return nullptr;
    if (PathUtils::ContainsTraversal(std::wstring(path))) return nullptr;
    FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
    auto model = wrapper->ModelLoader->LoadModel(std::wstring(path));
    if (model && model->SkeletalMesh)
    {
        return model->SkeletalMesh.get();
    }
    return nullptr;
}

ENGINEAPI void SkeletalMeshComponent_SetSkeletalMeshFromModel(void* component, void* engine, const wchar_t* path)
{
    if (!component || !engine || !path) return;
    if (PathUtils::ContainsTraversal(std::wstring(path))) return;
    KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
    FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
    auto model = wrapper->ModelLoader->LoadModel(std::wstring(path));
    if (model && model->SkeletalMesh)
    {
        skmc->SetSkeletalMesh(model->SkeletalMesh);
    }
}

    ENGINEAPI void Renderer_SetSSAOEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetSSAOEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetPostProcessEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetPostProcessEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetShadowEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetShadowEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetSkyEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetSkyEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetTAAEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetTAAEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetDebugUIEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetDebugUIEnabled(enabled);
    }

    ENGINEAPI void Renderer_SetSSREnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetSSREnabled(enabled);
    }

    ENGINEAPI void Renderer_SetVolumetricFogEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetVolumetricFogEnabled(enabled);
    }

    ENGINEAPI void Camera_SetFOV(void* camera, float fovY)
    {
        if (!camera) return;
        if (fovY <= 0.0f || fovY >= 3.14159265f) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        kcam->SetPerspective(fovY, kcam->GetAspectRatio(), kcam->GetNearZ(), kcam->GetFarZ());
    }

    ENGINEAPI float Camera_GetFOV(void* camera)
    {
        if (!camera) return 0.0f;
        KCamera* kcam = static_cast<KCamera*>(camera);
        return kcam->GetFovY();
    }

    ENGINEAPI void Camera_SetNearFar(void* camera, float nearZ, float farZ)
    {
        if (!camera) return;
        if (nearZ <= 0.0f || farZ <= 0.0f || nearZ >= farZ) return;
        KCamera* kcam = static_cast<KCamera*>(camera);
        kcam->SetPerspective(kcam->GetFovY(), kcam->GetAspectRatio(), nearZ, farZ);
    }

    ENGINEAPI float Camera_GetNearZ(void* camera)
    {
        if (!camera) return 0.0f;
        return static_cast<KCamera*>(camera)->GetNearZ();
    }

    ENGINEAPI float Camera_GetFarZ(void* camera)
    {
        if (!camera) return 0.0f;
        return static_cast<KCamera*>(camera)->GetFarZ();
    }

    ENGINEAPI void Renderer_SetDirectionalLight(void* renderer, float dirX, float dirY, float dirZ,
                                                 float colorR, float colorG, float colorB, float colorA,
                                                 float ambR, float ambG, float ambB, float ambA)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        FDirectionalLight light;
        light.Direction = XMFLOAT3(dirX, dirY, dirZ);
        light.Color = XMFLOAT4(colorR, colorG, colorB, colorA);
        light.AmbientColor = XMFLOAT4(ambR, ambG, ambB, ambA);
        krenderer->SetDirectionalLight(light);
    }

    ENGINEAPI void Renderer_SetDirectionalLightDirection(void* renderer, float x, float y, float z)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->GetDirectionalLightMutable().Direction = XMFLOAT3(x, y, z);
    }

    ENGINEAPI void Renderer_SetDirectionalLightColor(void* renderer, float r, float g, float b, float a)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->GetDirectionalLightMutable().Color = XMFLOAT4(r, g, b, a);
    }

    ENGINEAPI void Renderer_SetDirectionalLightAmbient(void* renderer, float r, float g, float b, float a)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->GetDirectionalLightMutable().AmbientColor = XMFLOAT4(r, g, b, a);
    }

    ENGINEAPI void Renderer_AddPointLight(void* renderer, float posX, float posY, float posZ,
                                           float colorR, float colorG, float colorB, float intensity,
                                           float radius, float falloff)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        FPointLight light;
        light.Position = XMFLOAT3(posX, posY, posZ);
        light.Color = XMFLOAT4(colorR, colorG, colorB, 1.0f);
        light.Intensity = intensity;
        light.Radius = radius;
        light.Falloff = falloff;
        krenderer->AddPointLight(light);
    }

    ENGINEAPI void Renderer_ClearPointLights(void* renderer)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->ClearPointLights();
    }

    ENGINEAPI void Renderer_AddSpotLight(void* renderer, float posX, float posY, float posZ,
                                          float dirX, float dirY, float dirZ,
                                          float colorR, float colorG, float colorB, float intensity,
                                          float innerCone, float outerCone, float radius, float falloff)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        FSpotLight light;
        light.Position = XMFLOAT3(posX, posY, posZ);
        light.Direction = XMFLOAT3(dirX, dirY, dirZ);
        light.Color = XMFLOAT4(colorR, colorG, colorB, 1.0f);
        light.Intensity = intensity;
        light.InnerCone = innerCone;
        light.OuterCone = outerCone;
        light.Radius = radius;
        light.Falloff = falloff;
        krenderer->AddSpotLight(light);
    }

    ENGINEAPI void Renderer_ClearSpotLights(void* renderer)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->ClearSpotLights();
    }

    ENGINEAPI void Renderer_GetDirectionalLight(void* renderer, float* dirX, float* dirY, float* dirZ,
                                                 float* colorR, float* colorG, float* colorB, float* colorA,
                                                 float* ambR, float* ambG, float* ambB, float* ambA)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        const FDirectionalLight& light = krenderer->GetDirectionalLight();
        if (dirX) *dirX = light.Direction.x;
        if (dirY) *dirY = light.Direction.y;
        if (dirZ) *dirZ = light.Direction.z;
        if (colorR) *colorR = light.Color.x;
        if (colorG) *colorG = light.Color.y;
        if (colorB) *colorB = light.Color.z;
        if (colorA) *colorA = light.Color.w;
        if (ambR) *ambR = light.AmbientColor.x;
        if (ambG) *ambG = light.AmbientColor.y;
        if (ambB) *ambB = light.AmbientColor.z;
        if (ambA) *ambA = light.AmbientColor.w;
    }

    ENGINEAPI int Renderer_GetPointLightCount(void* renderer)
    {
        if (!renderer) return 0;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        return static_cast<int>(krenderer->GetNumPointLights());
    }

    ENGINEAPI void Renderer_GetPointLight(void* renderer, int index,
                                           float* posX, float* posY, float* posZ,
                                           float* colorR, float* colorG, float* colorB, float* intensity,
                                           float* radius, float* falloff)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        if (index < 0 || index >= static_cast<int>(krenderer->GetNumPointLights())) return;
        const FPointLight& light = krenderer->GetPointLight(static_cast<UINT32>(index));
        if (posX) *posX = light.Position.x;
        if (posY) *posY = light.Position.y;
        if (posZ) *posZ = light.Position.z;
        if (colorR) *colorR = light.Color.x;
        if (colorG) *colorG = light.Color.y;
        if (colorB) *colorB = light.Color.z;
        if (intensity) *intensity = light.Intensity;
        if (radius) *radius = light.Radius;
        if (falloff) *falloff = light.Falloff;
    }

    ENGINEAPI void Renderer_SetDirectionalLightIntensity(void* renderer, float intensity)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->GetDirectionalLightMutable().Intensity = intensity;
    }

    ENGINEAPI float Renderer_GetDirectionalLightIntensity(void* renderer)
    {
        if (!renderer) return 0.0f;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        return krenderer->GetDirectionalLight().Intensity;
    }

    ENGINEAPI void Renderer_SetShadowSceneBounds(void* renderer, float centerX, float centerY, float centerZ, float radius)
    {
        if (!renderer) return;
        KRenderer* krenderer = static_cast<KRenderer*>(renderer);
        krenderer->SetShadowSceneBounds(XMFLOAT3(centerX, centerY, centerZ), radius);
    }

ENGINEAPI void* Texture_Load(void* engine, const wchar_t* path)
{
    if (!engine || !path) return nullptr;
    if (PathUtils::ContainsTraversal(std::wstring(path))) return nullptr;
    FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        if (!wrapper->Engine) return nullptr;
        auto* renderer = wrapper->Engine->GetRenderer();
        if (!renderer) return nullptr;
        auto* textureMgr = renderer->GetTextureManager();
        if (!textureMgr) return nullptr;
        auto* graphicsDevice = wrapper->Engine->GetGraphicsDevice();
        if (!graphicsDevice) return nullptr;
        auto texture = textureMgr->LoadTexture(graphicsDevice->GetDevice(), std::wstring(path));
        return texture ? texture.get() : nullptr;
    }

    ENGINEAPI void Texture_Unload(void* engine, const wchar_t* path)
    {
        if (!engine || !path) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        auto* renderer = wrapper->Engine->GetRenderer();
        if (!renderer) return;
        auto* textureMgr = renderer->GetTextureManager();
        if (textureMgr)
        {
            std::wstring wpath(path);
            textureMgr->RemoveTexture(wpath);
        }
    }

    ENGINEAPI void SkeletalMeshComponent_PauseAnimation(void* component)
    {
        if (!component) return;
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        skmc->PauseAnimation();
    }

    ENGINEAPI void SkeletalMeshComponent_ResumeAnimation(void* component)
    {
        if (!component) return;
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        skmc->ResumeAnimation();
    }

    ENGINEAPI const char* SkeletalMeshComponent_GetAnimationName(void* component, int index)
    {
        if (!component) return "";
        KSkeletalMeshComponent* skmc = static_cast<KSkeletalMeshComponent*>(component);
        static thread_local std::string nameBuffer;
        nameBuffer = skmc->GetAnimationName(index);
        return nameBuffer.c_str();
    }

    ENGINEAPI int Model_HasSkeleton(void* model)
    {
        if (!model) return 0;
        FLoadedModel* loadedModel = static_cast<FLoadedModel*>(model);
        return (loadedModel->Skeleton && !loadedModel->Skeleton->GetBones().empty()) ? 1 : 0;
    }

    ENGINEAPI int Model_GetAnimationCount(void* model)
    {
        if (!model) return 0;
        FLoadedModel* loadedModel = static_cast<FLoadedModel*>(model);
        return static_cast<int>(loadedModel->Animations.size());
    }

    ENGINEAPI const char* Model_GetAnimationName(void* model, int index)
    {
        if (!model) return "";
        FLoadedModel* loadedModel = static_cast<FLoadedModel*>(model);
        if (index >= 0 && index < static_cast<int>(loadedModel->Animations.size()))
        {
            static thread_local std::string nameBuffer;
            nameBuffer = loadedModel->Animations[index]->GetName();
            return nameBuffer.c_str();
        }
        return "";
    }

    ENGINEAPI bool Actor_AddChild(void* parent, void* child)
    {
        if (!parent || !child) return false;
        KActor* parentActor = static_cast<KActor*>(parent);
        KActor* childActor = static_cast<KActor*>(child);

        if (parentActor == childActor) return false;

        KActor* ancestor = parentActor;
        while (ancestor->GetParent())
        {
            if (ancestor->GetParent() == childActor)
            {
                LOG_WARNING("Actor_AddChild: cycle detected in hierarchy");
                return false;
            }
            ancestor = ancestor->GetParent();
        }

        const auto& children = parentActor->GetChildren();
        for (const auto& c : children)
        {
            if (c.get() == childActor)
            {
                return true;
            }
        }

        if (childActor->GetParent())
        {
            childActor->GetParent()->RemoveChild(childActor);
        }

        if (!g_EngineWrapper || !g_EngineWrapper->Engine) return false;
        auto sceneSharedPtr = g_EngineWrapper->Engine->GetSceneManager().GetActiveScene();
        if (!sceneSharedPtr) return false;
        KScene* scene = sceneSharedPtr.get();

        const auto& actors = scene->GetActors();
        for (const auto& actor : actors)
        {
            if (actor.get() == childActor)
            {
                parentActor->AddChild(actor);
                return true;
            }
        }

        LOG_WARNING("Actor_AddChild: child actor not found in active scene");
        return false;
    }

    ENGINEAPI int Actor_GetChildCount(void* actor)
    {
        if (!actor) return 0;
        KActor* kactor = static_cast<KActor*>(actor);
        return static_cast<int>(kactor->GetChildren().size());
    }

    ENGINEAPI void* Actor_GetChild(void* actor, int index)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        const auto& children = kactor->GetChildren();
        if (index >= 0 && index < static_cast<int>(children.size()))
        {
            return children[index].get();
        }
        return nullptr;
    }

    ENGINEAPI void* Actor_GetParent(void* actor)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        return kactor->GetParent();
    }

    ENGINEAPI void DebugRenderer_DrawGrid(void* engine, float centerX, float centerY, float centerZ, float size, float cellSize, int subdivisions)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        if (!wrapper->DebugRenderer || !wrapper->DebugRenderer->IsEnabled()) return;
        wrapper->DebugRenderer->DrawGrid(XMFLOAT3(centerX, centerY, centerZ), size, cellSize,
                                         XMFLOAT4(0.3f, 0.3f, 0.3f, 0.5f), subdivisions);
    }

    ENGINEAPI void DebugRenderer_DrawAxis(void* engine, float originX, float originY, float originZ, float length)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        if (!wrapper->DebugRenderer || !wrapper->DebugRenderer->IsEnabled()) return;
        wrapper->DebugRenderer->DrawAxis(XMFLOAT3(originX, originY, originZ), length);
    }

    ENGINEAPI void DebugRenderer_SetEnabled(void* engine, bool enabled)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        if (wrapper->DebugRenderer)
        {
            wrapper->DebugRenderer->SetEnabled(enabled);
        }
    }

    ENGINEAPI void DebugRenderer_RenderFrame(void* engine, float deltaTime)
    {
        if (!engine) return;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        if (!wrapper->DebugRenderer || !wrapper->DebugRenderer->IsInitialized() || !wrapper->DebugRenderer->IsEnabled()) return;

        KGraphicsDevice* device = wrapper->Engine->GetGraphicsDevice();
        KCamera* camera = wrapper->Engine->GetCamera();
        if (!device || !camera) return;

        wrapper->DebugRenderer->BeginFrame();
        wrapper->DebugRenderer->Render(device->GetContext(), camera);
        wrapper->DebugRenderer->EndFrame(deltaTime);
    }

    ENGINEAPI void Renderer_SetCascadedShadowsEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* r = static_cast<KRenderer*>(renderer);
        r->SetCascadedShadowsEnabled(enabled);
    }

    ENGINEAPI bool Renderer_IsCascadedShadowsEnabled(void* renderer)
    {
        if (!renderer) return false;
        return static_cast<KRenderer*>(renderer)->IsCascadedShadowsEnabled();
    }

    ENGINEAPI void Renderer_SetIBLEnabled(void* renderer, bool enabled)
    {
        if (!renderer) return;
        KRenderer* r = static_cast<KRenderer*>(renderer);
        r->SetIBLEnabled(enabled);
    }

    ENGINEAPI bool Renderer_IsIBLEnabled(void* renderer)
    {
        if (!renderer) return false;
        return static_cast<KRenderer*>(renderer)->IsIBLEnabled();
    }

    ENGINEAPI HRESULT Renderer_LoadEnvironmentMap(void* renderer, const wchar_t* hdrPath)
    {
        if (!renderer || !hdrPath) return E_INVALIDARG;
        if (PathUtils::ContainsTraversal(std::wstring(hdrPath))) return E_INVALIDARG;
        return static_cast<KRenderer*>(renderer)->LoadEnvironmentMap(std::wstring(hdrPath));
    }

    ENGINEAPI void Material_SetTexture(void* component, int textureSlot, const wchar_t* texturePath)
    {
        if (!component || !texturePath) return;
        if (PathUtils::ContainsTraversal(std::wstring(texturePath))) return;
        KStaticMeshComponent* smc = static_cast<KStaticMeshComponent*>(component);
        KMaterial* material = smc->GetMaterial();
        if (!material) return;

        KRenderer* renderer = g_EngineWrapper && g_EngineWrapper->Engine ? g_EngineWrapper->Engine->GetRenderer() : nullptr;
        if (!renderer) return;

        KGraphicsDevice* graphicsDevice = g_EngineWrapper->Engine->GetGraphicsDevice();
        if (!graphicsDevice) return;

        auto* textureMgr = renderer->GetTextureManager();
        if (!textureMgr) return;

        std::shared_ptr<KTexture> texture = textureMgr->LoadTexture(graphicsDevice->GetDevice(), std::wstring(texturePath));
        if (!texture) return;

        material->SetTexture(static_cast<EMaterialTextureSlot>(textureSlot), texture);
    }
}
