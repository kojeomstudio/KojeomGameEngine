#pragma once

#include <windows.h>
#include <Engine/Utils/Common.h>

#ifdef ENGINEAPI_EXPORTS
#define ENGINEAPI __declspec(dllexport)
#else
#define ENGINEAPI __declspec(dllimport)
#endif

extern "C"
{
    ENGINEAPI void* Engine_Create();
    ENGINEAPI void Engine_Destroy(void* engine);
    ENGINEAPI HRESULT Engine_Initialize(void* engine, HWND hwnd, int width, int height);
    ENGINEAPI void Engine_Tick(void* engine, float deltaTime);
    ENGINEAPI void Engine_Render(void* engine);
    ENGINEAPI void Engine_Resize(void* engine, int width, int height);

    ENGINEAPI HRESULT Engine_InitializeEmbedded(void* engine, HWND hwnd, int width, int height);

    ENGINEAPI void* Engine_GetSceneManager(void* engine);
    ENGINEAPI void* Engine_GetRenderer(void* engine);

    ENGINEAPI void* Scene_Create(void* sceneMgr, const char* name);
    ENGINEAPI void* Scene_Load(void* sceneMgr, const wchar_t* path);
    ENGINEAPI HRESULT Scene_Save(void* sceneMgr, const wchar_t* path, void* scene);
    ENGINEAPI void Scene_SetActive(void* sceneMgr, void* scene);
    ENGINEAPI void* Scene_GetActive(void* sceneMgr);

    ENGINEAPI void* Actor_Create(void* scene, const char* name);
    ENGINEAPI void Actor_Destroy(void* scene, void* actor);
    ENGINEAPI void Actor_SetPosition(void* actor, float x, float y, float z);
    ENGINEAPI void Actor_SetRotation(void* actor, float x, float y, float z, float w);
    ENGINEAPI void Actor_SetScale(void* actor, float x, float y, float z);
    ENGINEAPI void Actor_GetPosition(void* actor, float* x, float* y, float* z);
    ENGINEAPI void Actor_GetRotation(void* actor, float* x, float* y, float* z, float* w);
    ENGINEAPI void Actor_GetScale(void* actor, float* x, float* y, float* z);
    ENGINEAPI const char* Actor_GetName(void* actor);

    ENGINEAPI void* Camera_GetMain(void* engine);
    ENGINEAPI void Camera_SetPosition(void* camera, float x, float y, float z);
    ENGINEAPI void Camera_SetRotation(void* camera, float pitch, float yaw, float roll);
    ENGINEAPI void Camera_GetPosition(void* camera, float* x, float* y, float* z);
    ENGINEAPI void Camera_GetViewMatrix(void* camera, float* matrix);
    ENGINEAPI void Camera_GetProjectionMatrix(void* camera, float* matrix);

    ENGINEAPI void* Model_Load(void* engine, const wchar_t* path);
    ENGINEAPI void Model_Unload(void* model);

    ENGINEAPI void Renderer_SetRenderPath(void* renderer, int path);
    ENGINEAPI void Renderer_SetDebugMode(void* renderer, bool enabled);
    ENGINEAPI void Renderer_GetStats(void* renderer, int* drawCalls, int* vertexCount, float* frameTime);

    ENGINEAPI void* Scene_Raycast(void* scene, float originX, float originY, float originZ,
                                  float dirX, float dirY, float dirZ,
                                  float* outHitX, float* outHitY, float* outHitZ);
    ENGINEAPI int Scene_GetActorCount(void* scene);
    ENGINEAPI void* Scene_GetActorAt(void* scene, int index);

    ENGINEAPI void Actor_GetComponentCount(void* actor, int* outCount);
    ENGINEAPI const char* Actor_GetComponentName(void* actor, int index);
    ENGINEAPI int Actor_GetComponentType(void* actor, int index);

    ENGINEAPI void Material_SetAlbedo(void* material, float r, float g, float b, float a);
    ENGINEAPI void Material_SetMetallic(void* material, float value);
    ENGINEAPI void Material_SetRoughness(void* material, float value);

    ENGINEAPI void* Actor_AddComponent(void* actor, int componentType);
    ENGINEAPI void* Actor_GetStaticMeshComponent(void* actor);
    ENGINEAPI void* Actor_GetSkeletalMeshComponent(void* actor);
    ENGINEAPI void* Actor_GetLightComponent(void* actor);
    ENGINEAPI void* StaticMeshComponent_SetMesh(void* component, const wchar_t* meshPath);
    ENGINEAPI void* SkeletalMeshComponent_PlayAnimation(void* component, const char* animName);
    ENGINEAPI void SkeletalMeshComponent_StopAnimation(void* component);
    ENGINEAPI int SkeletalMeshComponent_GetAnimationCount(void* component);
    ENGINEAPI void Actor_SetVisibility(void* actor, bool visible);
    ENGINEAPI bool Actor_IsVisible(void* actor);
    ENGINEAPI void* Model_LoadAndGetStaticMesh(void* engine, const wchar_t* path);
    ENGINEAPI void Renderer_SetSSAOEnabled(void* renderer, bool enabled);
    ENGINEAPI void Renderer_SetPostProcessEnabled(void* renderer, bool enabled);
    ENGINEAPI void Renderer_SetShadowEnabled(void* renderer, bool enabled);
}
