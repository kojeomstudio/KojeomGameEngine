#define NOMINMAX
#include "EngineAPI.h"
#include <Engine/Core/Engine.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Assets/ModelLoader.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/Actor.h>
#include <Engine/Assets/StaticMeshComponent.h>
#include <Engine/Assets/SkeletalMeshComponent.h>

class FEngineWrapper
{
public:
    std::unique_ptr<KEngine> Engine;
    std::unique_ptr<KModelLoader> ModelLoader;

    FEngineWrapper()
    {
        Engine = std::make_unique<KEngine>();
        ModelLoader = std::make_unique<KModelLoader>();
    }

    ~FEngineWrapper()
    {
        ModelLoader.reset();
        Engine.reset();
    }
};

extern "C"
{
    static FEngineWrapper* g_EngineWrapper = nullptr;

    ENGINEAPI void* Engine_Create()
    {
        g_EngineWrapper = new FEngineWrapper();
        return g_EngineWrapper;
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
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        HRESULT hr = wrapper->Engine->Initialize(hInst, L"KojeomEditor", width, height);
        if (SUCCEEDED(hr))
        {
            wrapper->ModelLoader->Initialize();
        }
        return hr;
    }

    ENGINEAPI HRESULT Engine_InitializeEmbedded(void* engine, HWND hwnd, int width, int height)
    {
        if (!engine) return E_INVALIDARG;
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        
        HRESULT hr = wrapper->Engine->InitializeWithExternalHwnd(hwnd, width, height);
        if (SUCCEEDED(hr))
        {
            wrapper->ModelLoader->Initialize();
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
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        return mgr->CreateScene(std::string(name)).get();
    }

    ENGINEAPI void* Scene_Load(void* sceneMgr, const wchar_t* path)
    {
        if (!sceneMgr || !path) return nullptr;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        return mgr->LoadScene(std::wstring(path)).get();
    }

    ENGINEAPI HRESULT Scene_Save(void* sceneMgr, const wchar_t* path, void* scene)
    {
        if (!sceneMgr || !path || !scene) return E_INVALIDARG;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        auto scenePtr = std::shared_ptr<KScene>(static_cast<KScene*>(scene), [](KScene*) {});
        return mgr->SaveScene(std::wstring(path), scenePtr);
    }

    ENGINEAPI void Scene_SetActive(void* sceneMgr, void* scene)
    {
        if (!sceneMgr || !scene) return;
        KSceneManager* mgr = static_cast<KSceneManager*>(sceneMgr);
        auto scenePtr = std::shared_ptr<KScene>(static_cast<KScene*>(scene), [](KScene*) {});
        mgr->SetActiveScene(scenePtr);
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
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        return wrapper->ModelLoader->LoadModel(std::wstring(path)).get();
    }

    ENGINEAPI void Model_Unload(void* model)
    {
        if (!model) return;
        FLoadedModel* loadedModel = static_cast<FLoadedModel*>(model);
        delete loadedModel;
    }

    ENGINEAPI void Renderer_SetRenderPath(void* renderer, int path)
    {
        if (!renderer) return;
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

    static const char* COMPONENT_NAMES[] = {
        "Unknown", "Transform", "StaticMesh", "SkeletalMesh",
        "Light", "Camera", "Audio", "Physics"
    };

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
            if (dynamic_cast<KStaticMeshComponent*>(comp)) return 2;
            if (dynamic_cast<KSkeletalMeshComponent*>(comp)) return 3;
        }
        return 0;
    }

    ENGINEAPI void Material_SetAlbedo(void* material, float r, float g, float b, float a)
    {
        if (!material) return;
        FPBRMaterialParams* mat = static_cast<FPBRMaterialParams*>(material);
        mat->AlbedoColor = XMFLOAT4(r, g, b, a);
    }

    ENGINEAPI void Material_SetMetallic(void* material, float value)
    {
        if (!material) return;
        FPBRMaterialParams* mat = static_cast<FPBRMaterialParams*>(material);
        mat->Metallic = value;
    }

    ENGINEAPI void Material_SetRoughness(void* material, float value)
    {
        if (!material) return;
        FPBRMaterialParams* mat = static_cast<FPBRMaterialParams*>(material);
        mat->Roughness = value;
    }

    ENGINEAPI void* Actor_AddComponent(void* actor, int componentType)
    {
        if (!actor) return nullptr;
        KActor* kactor = static_cast<KActor*>(actor);
        ComponentPtr newComp;
        switch (componentType)
        {
        case 2:
            newComp = std::make_shared<KStaticMeshComponent>();
            break;
        case 3:
            newComp = std::make_shared<KSkeletalMeshComponent>();
            break;
        default:
            newComp = std::make_shared<KActorComponent>();
            break;
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
        return kactor->GetComponent<KActorComponent>();
    }

    ENGINEAPI void* StaticMeshComponent_SetMesh(void* component, const wchar_t* meshPath)
    {
        if (!component) return nullptr;
        KStaticMeshComponent* smc = static_cast<KStaticMeshComponent*>(component);
        
        if (g_EngineWrapper && g_EngineWrapper->ModelLoader && meshPath && wcslen(meshPath) > 0)
        {
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
        if (skmc->HasAnimation("")) return 0;
        return 0;
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
        FEngineWrapper* wrapper = static_cast<FEngineWrapper*>(engine);
        auto model = wrapper->ModelLoader->LoadModel(std::wstring(path));
        if (model && model->StaticMesh)
        {
            return model->StaticMesh.get();
        }
        return nullptr;
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
}
