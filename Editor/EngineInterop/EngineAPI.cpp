#define NOMINMAX
#define ENGINEAPI_EXPORTS
#include "EngineAPI.h"
#include <Engine/Core/Engine.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Assets/ModelLoader.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/Actor.h>

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
    ENGINEAPI void* Engine_Create()
    {
        return new FEngineWrapper();
    }

    ENGINEAPI void Engine_Destroy(void* engine)
    {
        if (engine)
        {
            delete static_cast<FEngineWrapper*>(engine);
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
        return kactor->GetName().c_str();
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
}
