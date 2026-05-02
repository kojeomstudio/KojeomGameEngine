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
    ENGINEAPI void Model_Unload(void* engine, const wchar_t* path);

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
    ENGINEAPI void Material_SetAO(void* material, float value);
    ENGINEAPI void Material_GetAlbedo(void* material, float* r, float* g, float* b, float* a);
    ENGINEAPI float Material_GetMetallic(void* material);
    ENGINEAPI float Material_GetRoughness(void* material);
    ENGINEAPI float Material_GetAO(void* material);

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

    ENGINEAPI void Renderer_SetSkyEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsSkyEnabled(void* renderer);
    ENGINEAPI void Renderer_SetTAAEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsTAAEnabled(void* renderer);
    ENGINEAPI void Renderer_SetDebugUIEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsDebugUIEnabled(void* renderer);
    ENGINEAPI void Renderer_SetSSREnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsSSREnabled(void* renderer);
    ENGINEAPI void Renderer_SetVolumetricFogEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsVolumetricFogEnabled(void* renderer);
    ENGINEAPI void Renderer_SetSSAOEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsSSAOEnabled(void* renderer);
    ENGINEAPI void Renderer_SetPostProcessEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsPostProcessEnabled(void* renderer);
    ENGINEAPI void Renderer_SetShadowEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsShadowEnabled(void* renderer);

    ENGINEAPI void Camera_SetFOV(void* camera, float fovY);
    ENGINEAPI float Camera_GetFOV(void* camera);
    ENGINEAPI void Camera_SetNearFar(void* camera, float nearZ, float farZ);
    ENGINEAPI float Camera_GetNearZ(void* camera);
    ENGINEAPI float Camera_GetFarZ(void* camera);

    ENGINEAPI void Renderer_SetDirectionalLight(void* renderer, float dirX, float dirY, float dirZ,
                                                 float colorR, float colorG, float colorB, float colorA,
                                                 float ambR, float ambG, float ambB, float ambA);
    ENGINEAPI void Renderer_AddPointLight(void* renderer, float posX, float posY, float posZ,
                                           float colorR, float colorG, float colorB, float intensity,
                                           float radius, float falloff);
    ENGINEAPI void Renderer_ClearPointLights(void* renderer);
    ENGINEAPI void Renderer_AddSpotLight(void* renderer, float posX, float posY, float posZ,
                                          float dirX, float dirY, float dirZ,
                                          float colorR, float colorG, float colorB, float intensity,
                                          float innerCone, float outerCone, float radius, float falloff);
    ENGINEAPI void Renderer_ClearSpotLights(void* renderer);
    ENGINEAPI void Renderer_SetShadowSceneBounds(void* renderer, float centerX, float centerY, float centerZ, float radius);

    ENGINEAPI void* Texture_Load(void* engine, const wchar_t* path);
    ENGINEAPI void Texture_Unload(void* engine, const wchar_t* path);

    ENGINEAPI void SkeletalMeshComponent_PauseAnimation(void* component);
    ENGINEAPI void SkeletalMeshComponent_ResumeAnimation(void* component);
    ENGINEAPI const char* SkeletalMeshComponent_GetAnimationName(void* component, int index);

    ENGINEAPI int Model_HasSkeleton(void* model);
    ENGINEAPI int Model_GetAnimationCount(void* model);
    ENGINEAPI const char* Model_GetAnimationName(void* model, int index);

    ENGINEAPI bool Actor_AddChild(void* parent, void* child);
    ENGINEAPI int Actor_GetChildCount(void* actor);
    ENGINEAPI void* Actor_GetChild(void* actor, int index);
    ENGINEAPI void* Actor_GetParent(void* actor);

    ENGINEAPI void Renderer_SetDirectionalLightDirection(void* renderer, float x, float y, float z);
    ENGINEAPI void Renderer_SetDirectionalLightColor(void* renderer, float r, float g, float b, float a);
    ENGINEAPI void Renderer_SetDirectionalLightAmbient(void* renderer, float r, float g, float b, float a);

    ENGINEAPI void Actor_SetName(void* actor, const char* name);

    ENGINEAPI void* StaticMeshComponent_GetMaterial(void* component);
    ENGINEAPI void* StaticMeshComponent_CreateDefaultMesh(void* component);

    ENGINEAPI void Renderer_GetDirectionalLight(void* renderer, float* dirX, float* dirY, float* dirZ,
                                                 float* colorR, float* colorG, float* colorB, float* colorA,
                                                 float* ambR, float* ambG, float* ambB, float* ambA);
    ENGINEAPI int Renderer_GetPointLightCount(void* renderer);
    ENGINEAPI void Renderer_GetPointLight(void* renderer, int index,
                                           float* posX, float* posY, float* posZ,
                                           float* colorR, float* colorG, float* colorB, float* intensity,
                                           float* radius, float* falloff);

    ENGINEAPI void Renderer_SetDirectionalLightIntensity(void* renderer, float intensity);
    ENGINEAPI float Renderer_GetDirectionalLightIntensity(void* renderer);

    ENGINEAPI void* Model_LoadAndGetSkeletalMesh(void* engine, const wchar_t* path);
    ENGINEAPI void SkeletalMeshComponent_SetSkeletalMeshFromModel(void* component, void* engine, const wchar_t* path);

    ENGINEAPI void DebugRenderer_DrawGrid(void* engine, float centerX, float centerY, float centerZ, float size, float cellSize, int subdivisions);
    ENGINEAPI void DebugRenderer_DrawAxis(void* engine, float originX, float originY, float originZ, float length);
    ENGINEAPI void DebugRenderer_SetEnabled(void* engine, bool enabled);
    ENGINEAPI void DebugRenderer_RenderFrame(void* engine, float deltaTime);

    ENGINEAPI void Renderer_SetCascadedShadowsEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsCascadedShadowsEnabled(void* renderer);
    ENGINEAPI void Renderer_SetIBLEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsIBLEnabled(void* renderer);
    ENGINEAPI HRESULT Renderer_LoadEnvironmentMap(void* renderer, const wchar_t* hdrPath);
    ENGINEAPI void Material_SetTexture(void* component, int textureSlot, const wchar_t* texturePath);
    ENGINEAPI void Material_SetEmissive(void* component, float r, float g, float b, float intensity);
    ENGINEAPI void Material_GetEmissive(void* component, float* r, float* g, float* b, float* intensity);
    ENGINEAPI int Material_HasTexture(void* component, int textureSlot);
    ENGINEAPI void* Scene_FindActor(void* scene, const char* name);
    ENGINEAPI const char* Scene_GetName(void* scene);
    ENGINEAPI int Renderer_GetSpotLightCount(void* renderer);
    ENGINEAPI void Renderer_GetSpotLight(void* renderer, int index,
                                          float* posX, float* posY, float* posZ,
                                          float* dirX, float* dirY, float* dirZ,
                                          float* colorR, float* colorG, float* colorB, float* intensity,
                                          float* innerCone, float* outerCone, float* radius, float* falloff);
    ENGINEAPI void Renderer_SetSSGIEnabled(void* renderer, bool enabled);
    ENGINEAPI bool Renderer_IsSSGIEnabled(void* renderer);

    ENGINEAPI void* BlendTree_Create();
    ENGINEAPI void BlendTree_Destroy(void* blendTree);
    ENGINEAPI void BlendTree_SetSkeleton(void* blendTree, void* skeleton);
    ENGINEAPI void BlendTree_SetParameterName(void* blendTree, const char* name);
    ENGINEAPI int BlendTree_AddChild(void* blendTree, void* animation, float parameterValue);
    ENGINEAPI void BlendTree_RemoveChild(void* blendTree, int index);
    ENGINEAPI int BlendTree_GetChildCount(void* blendTree);
    ENGINEAPI void BlendTree_Update(void* blendTree, float deltaTime, float parameterValue);
    ENGINEAPI int BlendTree_GetBoneMatrixCount(void* blendTree);
    ENGINEAPI const float* BlendTree_GetBoneMatrixData(void* blendTree);
    ENGINEAPI float BlendTree_GetChildWeight(void* blendTree, int index);

    ENGINEAPI void AnimationInstance_SetRootMotionBoneIndex(void* component, int boneIndex);
    ENGINEAPI int AnimationInstance_GetRootMotionBoneIndex(void* component);
    ENGINEAPI void AnimationInstance_SetRootMotionMode(void* component, int mode);
    ENGINEAPI int AnimationInstance_GetRootMotionMode(void* component);
    ENGINEAPI void AnimationInstance_ExtractRootMotion(void* component, float* posX, float* posY, float* posZ,
                                                          float* rotX, float* rotY, float* rotZ, float* rotW);

    ENGINEAPI void BlendTree_SetChildSpeed(void* blendTree, int index, float speed);
    ENGINEAPI float BlendTree_GetChildSpeed(void* blendTree, int index);
    ENGINEAPI void BlendTree_SetChildLooping(void* blendTree, int index, bool looping);
    ENGINEAPI bool BlendTree_IsChildLooping(void* blendTree, int index);

    ENGINEAPI void* AnimStateMachine_Create();
    ENGINEAPI void AnimStateMachine_Destroy(void* stateMachine);
    ENGINEAPI void AnimStateMachine_SetSkeleton(void* stateMachine, void* skeleton);
    ENGINEAPI void* AnimStateMachine_AddState(void* stateMachine, const char* name, void* animation);
    ENGINEAPI void* AnimStateMachine_AddBlendTreeState(void* stateMachine, const char* name, void* blendTree);
    ENGINEAPI void AnimStateMachine_RemoveState(void* stateMachine, const char* name);
    ENGINEAPI const char* AnimStateMachine_GetCurrentStateName(void* stateMachine);
    ENGINEAPI int AnimStateMachine_GetStateCount(void* stateMachine);
    ENGINEAPI const char* AnimStateMachine_GetStateName(void* stateMachine, int index);
    ENGINEAPI void AnimStateMachine_SetDefaultState(void* stateMachine, const char* name);
    ENGINEAPI void* AnimStateMachine_AddTransition(void* stateMachine, const char* fromState, const char* toState,
                                                     float blendDuration, int hasExitTime, float exitTime);
    ENGINEAPI void AnimStateMachine_RemoveTransition(void* stateMachine, const char* fromState, const char* toState);
    ENGINEAPI int AnimStateMachine_GetTransitionCount(void* stateMachine);
    ENGINEAPI void AnimStateMachine_SetFloatParameter(void* stateMachine, const char* name, float value);
    ENGINEAPI float AnimStateMachine_GetFloatParameter(void* stateMachine, const char* name);
    ENGINEAPI void AnimStateMachine_SetBoolParameter(void* stateMachine, const char* name, int value);
    ENGINEAPI int AnimStateMachine_GetBoolParameter(void* stateMachine, const char* name);
    ENGINEAPI void AnimStateMachine_TriggerTransition(void* stateMachine, const char* targetState);
    ENGINEAPI void AnimStateMachine_ForceState(void* stateMachine, const char* stateName, float blendDuration);
    ENGINEAPI void AnimStateMachine_Update(void* stateMachine, float deltaTime);
    ENGINEAPI int AnimStateMachine_GetBoneMatrixCount(void* stateMachine);
    ENGINEAPI const float* AnimStateMachine_GetBoneMatrixData(void* stateMachine);
    ENGINEAPI int AnimStateMachine_IsTransitioning(void* stateMachine);
    ENGINEAPI float AnimStateMachine_GetBlendProgress(void* stateMachine);
    ENGINEAPI const char* AnimStateMachine_GetPreviousStateName(void* stateMachine);
    ENGINEAPI void AnimStateMachine_AddFloatCondition(void* stateMachine, const char* fromState, const char* toState,
                                                        const char* paramName, int comparisonType, float compareValue);
    ENGINEAPI void AnimStateMachine_AddBoolCondition(void* stateMachine, const char* fromState, const char* toState,
                                                      const char* paramName, int comparisonType);
    ENGINEAPI void AnimStateMachine_SetBlendTreeParameter(void* stateMachine, const char* stateName, float value);
    ENGINEAPI float AnimStateMachine_GetBlendTreeParameter(void* stateMachine, const char* stateName);

    ENGINEAPI void* Terrain_Create(void* graphicsDevice, int resolution, float scale, float heightScale, int lodCount);
    ENGINEAPI void Terrain_Destroy(void* terrain);
    ENGINEAPI void Terrain_SetHeightMapFromRaw(void* terrain, const wchar_t* path, int width, int height);
    ENGINEAPI void Terrain_GeneratePerlinNoise(void* terrain, int width, int height, float scale, int octaves, float persistence);
    ENGINEAPI void Terrain_GenerateFlat(void* terrain, int width, int height, float flatHeight);
    ENGINEAPI float Terrain_GetHeightAtWorldPosition(void* terrain, float worldX, float worldZ);
    ENGINEAPI void Terrain_SetWorldPosition(void* terrain, float x, float y, float z);
    ENGINEAPI void Terrain_GetWorldPosition(void* terrain, float* x, float* y, float* z);
    ENGINEAPI void Terrain_SetWorldScale(void* terrain, float x, float y, float z);
    ENGINEAPI void Terrain_GetWorldScale(void* terrain, float* x, float* y, float* z);
    ENGINEAPI int Terrain_IsInitialized(void* terrain);
    ENGINEAPI int Terrain_GetResolution(void* terrain);
}
