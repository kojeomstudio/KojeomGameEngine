#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include "Math/Transform.h"
#include "Renderer/RenderScene.h"
#include "Assets/AssetStore.h"
#include "Renderer/Renderer.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"

namespace Kojeom
{
using EntityId = uint64_t;
constexpr EntityId INVALID_ENTITY_ID = 0;

class World;

struct IComponent
{
    virtual ~IComponent() = default;
    virtual void Tick(float deltaSeconds) {}
};

struct TransformComponent : public IComponent
{
    Transform transform;
};

struct CameraComponent : public IComponent
{
    CameraData cameraData;
    bool isActive = false;

    void Tick(float deltaSeconds) override
    {
        cameraData.UpdateMatrices();
    }
};

struct MeshRendererComponent : public IComponent
{
    AssetHandle meshHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;

    MeshRendererComponent() = default;
    MeshRendererComponent(AssetHandle mesh, AssetHandle mat = INVALID_HANDLE)
        : meshHandle(mesh), materialHandle(mat) {}
};

struct TerrainComponent : public IComponent
{
    AssetHandle terrainHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;
    float cellSize = 1.0f;
};

struct SkeletalMeshComponent : public IComponent
{
    AssetHandle skeletalMeshHandle = INVALID_HANDLE;
    AssetHandle materialHandle = INVALID_HANDLE;
    uint32_t boneCount = 0;
};

struct AnimatorComponent : public IComponent
{
    AssetHandle skeletonHandle = INVALID_HANDLE;
    AssetHandle currentClipHandle = INVALID_HANDLE;
    float playbackTime = 0.0f;
    float speed = 1.0f;
    bool loop = true;
    bool playing = false;
    AssetHandle boneMatricesHandle = INVALID_HANDLE;

    Animator internalAnimator;
    AnimationStateMachine stateMachine;
    bool useStateMachine = false;
    bool needsInit = true;
    std::vector<Mat4> cachedSkinMatrices;

    void Tick(float deltaSeconds) override;
};

class Entity
{
public:
    Entity(EntityId id, const std::string& name = "Entity")
        : m_id(id), m_name(name) {}

    EntityId GetId() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    TransformComponent* GetTransform() { return &m_transform; }
    const TransformComponent* GetTransform() const { return &m_transform; }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = comp.get();
        m_components.push_back(std::move(comp));
        return ptr;
    }

    void Tick(float deltaSeconds)
    {
        for (auto& comp : m_components)
        {
            comp->Tick(deltaSeconds);
        }
    }

    CameraComponent* GetCameraComponent()
    {
        for (auto& comp : m_components)
        {
            auto cam = dynamic_cast<CameraComponent*>(comp.get());
            if (cam) return cam;
        }
        return nullptr;
    }

    MeshRendererComponent* GetMeshRendererComponent()
    {
        for (auto& comp : m_components)
        {
            auto mr = dynamic_cast<MeshRendererComponent*>(comp.get());
            if (mr) return mr;
        }
        return nullptr;
    }

    TerrainComponent* GetTerrainComponent()
    {
        for (auto& comp : m_components)
        {
            auto tc = dynamic_cast<TerrainComponent*>(comp.get());
            if (tc) return tc;
        }
        return nullptr;
    }

    SkeletalMeshComponent* GetSkeletalMeshComponent()
    {
        for (auto& comp : m_components)
        {
            auto sm = dynamic_cast<SkeletalMeshComponent*>(comp.get());
            if (sm) return sm;
        }
        return nullptr;
    }

    AnimatorComponent* GetAnimatorComponent()
    {
        for (auto& comp : m_components)
        {
            auto ac = dynamic_cast<AnimatorComponent*>(comp.get());
            if (ac) return ac;
        }
        return nullptr;
    }

private:
    EntityId m_id;
    std::string m_name;
    TransformComponent m_transform;
    std::vector<std::unique_ptr<IComponent>> m_components;
};

class World
{
public:
    World() = default;

    void SetAssetStore(const AssetStore* assetStore) { m_assetStore = assetStore; }

    Entity* CreateEntity(const std::string& name = "Entity")
    {
        EntityId id = m_nextEntityId++;
        auto entity = std::make_unique<Entity>(id, name);
        Entity* ptr = entity.get();
        m_entities.push_back(std::move(entity));
        m_entityMap[id] = ptr;
        return ptr;
    }

    void RemoveEntity(EntityId id)
    {
        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                [id](const std::unique_ptr<Entity>& e) { return e->GetId() == id; }),
            m_entities.end());
        m_entityMap.erase(id);
    }

    Entity* GetEntity(EntityId id)
    {
        auto it = m_entityMap.find(id);
        return (it != m_entityMap.end()) ? it->second : nullptr;
    }

    void Tick(float deltaSeconds)
    {
        for (auto& entity : m_entities)
        {
            entity->Tick(deltaSeconds);
        }
    }

    RenderScene BuildRenderScene() const
    {
        RenderScene scene;

        if (m_defaultLight)
            scene.light = *m_defaultLight;

        for (const auto& entity : m_entities)
        {
            auto* cam = entity->GetCameraComponent();
            if (cam && cam->isActive)
            {
                scene.camera = cam->cameraData;
            }

            auto* mr = entity->GetMeshRendererComponent();
            if (mr && mr->meshHandle != INVALID_HANDLE)
            {
                StaticDrawCommand cmd;
                cmd.meshHandle = mr->meshHandle;
                cmd.materialHandle = mr->materialHandle;
                cmd.worldMatrix = entity->GetTransform()->transform.ToMatrix();

                if (m_assetStore)
                {
                    auto* meshData = m_assetStore->GetMesh(mr->meshHandle);
                    if (meshData)
                    {
                        cmd.boundsCenter = (meshData->boundsMin + meshData->boundsMax) * 0.5f;
                        cmd.boundsRadius = glm::length(meshData->boundsMax - meshData->boundsMin) * 0.5f;
                    }
                }

                scene.staticDrawCommands.push_back(cmd);
            }

            auto* tc = entity->GetTerrainComponent();
            if (tc && tc->terrainHandle != INVALID_HANDLE)
            {
                TerrainDrawCommand cmd;
                cmd.terrainHandle = tc->terrainHandle;
                cmd.materialHandle = tc->materialHandle;
                cmd.worldMatrix = entity->GetTransform()->transform.ToMatrix();

                if (m_assetStore)
                {
                    auto* terrain = m_assetStore->GetTerrain(tc->terrainHandle);
                    if (terrain)
                    {
                        float halfW = terrain->width * terrain->cellSize * 0.5f;
                        float halfH = terrain->height * terrain->cellSize * 0.5f;
                        cmd.boundsCenter = Vec3(halfW, terrain->maxHeight * 0.5f, halfH);
                        cmd.boundsRadius = glm::length(Vec3(halfW, terrain->maxHeight, halfH));
                    }
                }
                else
                {
                    cmd.boundsCenter = Vec3(0.0f);
                    cmd.boundsRadius = 200.0f;
                }

                scene.terrainDrawCommands.push_back(cmd);
            }

            auto* sm = entity->GetSkeletalMeshComponent();
            auto* anim = entity->GetAnimatorComponent();
            if (sm && sm->skeletalMeshHandle != INVALID_HANDLE)
            {
                SkinnedDrawCommand cmd;
                cmd.skeletalMeshHandle = sm->skeletalMeshHandle;
                cmd.materialHandle = sm->materialHandle;
                cmd.worldMatrix = entity->GetTransform()->transform.ToMatrix();
                cmd.boneCount = sm->boneCount;
                if (anim && !anim->cachedSkinMatrices.empty())
                {
                    cmd.boneCount = static_cast<uint32_t>(anim->cachedSkinMatrices.size());
                }
                if (anim && anim->boneMatricesHandle != INVALID_HANDLE)
                {
                    cmd.boneMatricesHandle = anim->boneMatricesHandle;
                }
                scene.skinnedDrawCommands.push_back(cmd);
            }
        }

        return scene;
    }

    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_entities; }
    size_t GetEntityCount() const { return m_entities.size(); }

    void SetAssetStore(AssetStore* store)
    {
        m_assetStore = store;
        s_assetStore = store;
    }
    static AssetStore* GetAssetStore() { return s_assetStore; }

    void Clear()
    {
        m_entities.clear();
        m_entityMap.clear();
    }

    struct SceneLoadResult
    {
        bool success = false;
        int entityCount = 0;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    SceneLoadResult LoadFromJson(const std::string& scenePath,
        AssetStore* assetStore, Renderer* renderer)
    {
        SceneLoadResult result;
        auto content = FileSystem::ReadTextFile(scenePath);
        if (content.empty())
        {
            result.errors.push_back("Scene file not found: " + scenePath);
            return result;
        }

        nlohmann::json sceneJson;
        try
        {
            sceneJson = nlohmann::json::parse(content);
        }
        catch (const std::exception& e)
        {
            result.errors.push_back("JSON parse error: " + std::string(e.what()));
            return result;
        }

        if (sceneJson.contains("light"))
        {
            m_defaultLight = LightData{};
            const auto& light = sceneJson["light"];
            if (light.contains("direction") && light["direction"].is_array() && light["direction"].size() >= 3)
                m_defaultLight->direction = Vec3(light["direction"][0], light["direction"][1], light["direction"][2]);
            if (light.contains("color") && light["color"].is_array() && light["color"].size() >= 3)
                m_defaultLight->color = Vec3(light["color"][0], light["color"][1], light["color"][2]);
            if (light.contains("intensity"))
                m_defaultLight->intensity = light["intensity"];
            if (light.contains("ambient") && light["ambient"].is_array() && light["ambient"].size() >= 3)
                m_defaultLight->ambientColor = Vec3(light["ambient"][0], light["ambient"][1], light["ambient"][2]);

            if (light.contains("pointLights") && light["pointLights"].is_array())
            {
                int plIdx = 0;
                for (const auto& pl : light["pointLights"])
                {
                    if (plIdx >= static_cast<int>(LightData::MAX_POINT_LIGHTS)) break;
                    if (pl.contains("position") && pl["position"].is_array() && pl["position"].size() >= 3)
                        m_defaultLight->pointLights[plIdx].position = Vec3(pl["position"][0], pl["position"][1], pl["position"][2]);
                    if (pl.contains("color") && pl["color"].is_array() && pl["color"].size() >= 3)
                        m_defaultLight->pointLights[plIdx].color = Vec3(pl["color"][0], pl["color"][1], pl["color"][2]);
                    if (pl.contains("range"))
                        m_defaultLight->pointLights[plIdx].range = pl["range"];
                    if (pl.contains("intensity"))
                        m_defaultLight->pointLights[plIdx].intensity = pl["intensity"];
                    ++plIdx;
                }
                m_defaultLight->pointLightCount = plIdx;
            }
        }

        if (!sceneJson.contains("entities"))
        {
            result.errors.push_back("Scene has no entities array");
            return result;
        }

        for (const auto& entJson : sceneJson["entities"])
        {
            std::string name = entJson.value("name", "Entity");
            Entity* entity = CreateEntity(name);
            result.entityCount++;

            if (entJson.contains("transform"))
            {
                const auto& tr = entJson["transform"];
                auto* transform = entity->GetTransform();
                if (tr.contains("position") && tr["position"].is_array() && tr["position"].size() >= 3)
                    transform->transform.position = Vec3(tr["position"][0], tr["position"][1], tr["position"][2]);
                if (tr.contains("rotation") && tr["rotation"].is_array() && tr["rotation"].size() >= 3)
                {
                    Vec3 euler(tr["rotation"][0], tr["rotation"][1], tr["rotation"][2]);
                    transform->transform.rotation = glm::quat(euler);
                }
                if (tr.contains("scale") && tr["scale"].is_array() && tr["scale"].size() >= 3)
                    transform->transform.scale = Vec3(tr["scale"][0], tr["scale"][1], tr["scale"][2]);
            }

            if (entJson.contains("camera"))
            {
                const auto& camJson = entJson["camera"];
                auto* cam = entity->AddComponent<CameraComponent>();
                cam->isActive = camJson.value("active", false);
                if (camJson.contains("fov")) cam->cameraData.fov = camJson["fov"];
                if (camJson.contains("near")) cam->cameraData.nearPlane = camJson["near"];
                if (camJson.contains("far")) cam->cameraData.farPlane = camJson["far"];
                if (camJson.contains("forward") && camJson["forward"].is_array() && camJson["forward"].size() >= 3)
                    cam->cameraData.forward = Vec3(camJson["forward"][0], camJson["forward"][1], camJson["forward"][2]);
            }

            if (entJson.contains("mesh"))
            {
                auto* mr = entity->AddComponent<MeshRendererComponent>();
                std::string meshPath = entJson["mesh"].get<std::string>();
                if (assetStore && renderer)
                {
                    if (meshPath == "$default_cube")
                    {
                        mr->meshHandle = renderer->GetDefaultCubeHandle();
                    }
                    else if (meshPath == "$default_plane")
                    {
                        mr->meshHandle = renderer->GetDefaultPlaneHandle();
                    }
                    else
                    {
                        mr->meshHandle = assetStore->LoadMesh(meshPath);
                        if (mr->meshHandle == INVALID_HANDLE)
                            result.warnings.push_back("Failed to load mesh: " + meshPath);
                    }
                }
            }

            if (entJson.contains("material"))
            {
                MaterialData materialDef{};
                const auto& matJson = entJson["material"];
                if (matJson.contains("albedo") && matJson["albedo"].is_array() && matJson["albedo"].size() >= 3)
                    materialDef.albedo = Vec3(matJson["albedo"][0], matJson["albedo"][1], matJson["albedo"][2]);
                if (matJson.contains("metallic")) materialDef.metallic = matJson["metallic"];
                if (matJson.contains("roughness")) materialDef.roughness = matJson["roughness"];
                if (matJson.contains("ao")) materialDef.ao = matJson["ao"];
                if (matJson.contains("emissive") && matJson["emissive"].is_array() && matJson["emissive"].size() >= 3)
                    materialDef.emissive = Vec3(matJson["emissive"][0], matJson["emissive"][1], matJson["emissive"][2]);
                if (matJson.contains("albedoTexture"))
                    materialDef.albedoTexturePath = matJson["albedoTexture"].get<std::string>();
                if (matJson.contains("normalTexture"))
                    materialDef.normalTexturePath = matJson["normalTexture"].get<std::string>();
                if (matJson.contains("metallicRoughnessTexture"))
                    materialDef.metallicRoughnessTexturePath = matJson["metallicRoughnessTexture"].get<std::string>();

                if (assetStore)
                {
                    AssetHandle matHandle = assetStore->RegisterMaterial(materialDef);
                    auto* mr = entity->GetMeshRendererComponent();
                    if (mr) mr->materialHandle = matHandle;
                }
            }

            if (entJson.contains("terrain"))
            {
                const auto& terJson = entJson["terrain"];
                auto* tc = entity->AddComponent<TerrainComponent>();
                tc->cellSize = terJson.value("cellSize", 1.0f);

                if (terJson.contains("heightmap") && assetStore)
                {
                    std::string heightmapPath = terJson["heightmap"].get<std::string>();
                    float maxH = terJson.value("maxHeight", 10.0f);
                    tc->terrainHandle = assetStore->LoadTerrainFromHeightmap(heightmapPath, tc->cellSize, maxH);
                }
                else if (terJson.contains("heightmapImage") && assetStore)
                {
                    std::string heightmapPath = terJson["heightmapImage"].get<std::string>();
                    float maxH = terJson.value("maxHeight", 10.0f);
                    tc->terrainHandle = assetStore->LoadTerrainFromHeightmap(heightmapPath, tc->cellSize, maxH);
                }
                else if (terJson.contains("size") && assetStore)
                {
                    int tw = terJson["size"][0];
                    int th = terJson["size"][1];
                    float maxH = terJson.value("maxHeight", 10.0f);
                    std::vector<float> heights(static_cast<size_t>(tw) * th, 0.0f);
                    if (terJson.contains("heights") && terJson["heights"].is_array())
                    {
                        for (int i = 0; i < std::min(static_cast<int>(terJson["heights"].size()), tw * th); ++i)
                            heights[i] = terJson["heights"][i].get<float>();
                    }
                    tc->terrainHandle = assetStore->CreateTerrain("terrain", tw, th, tc->cellSize, maxH, heights);
                }
            }

            if (entJson.contains("skeletalMesh") && assetStore)
            {
                const auto& smJson = entJson["skeletalMesh"];
                std::string meshPath = smJson.value("path", "");
                std::string skeletonPath = smJson.value("skeleton", "");

                AssetHandle skelHandle = INVALID_HANDLE;
                if (!skeletonPath.empty())
                    skelHandle = assetStore->LoadSkeleton(skeletonPath);

                auto* smComp = entity->AddComponent<SkeletalMeshComponent>();
                if (!meshPath.empty())
                {
                    smComp->skeletalMeshHandle = assetStore->LoadSkinnedMesh(meshPath);
                    if (smComp->skeletalMeshHandle != INVALID_HANDLE)
                    {
                        auto* skelMesh = assetStore->GetSkinnedMesh(smComp->skeletalMeshHandle);
                        if (skelMesh)
                            smComp->boneCount = static_cast<uint32_t>(
                                skelMesh->vertices.empty() ? 0 : 128);
                    }
                }

                if (entJson.contains("animation") && skelHandle != INVALID_HANDLE)
                {
                    const auto& animJson = entJson["animation"];
                    std::string clipPath = animJson.value("clip", "");
                    if (!clipPath.empty())
                    {
                        auto* animComp = entity->AddComponent<AnimatorComponent>();
                        animComp->skeletonHandle = skelHandle;
                        animComp->currentClipHandle = assetStore->LoadAnimationClip(clipPath, skelHandle);
                        animComp->loop = animJson.value("loop", true);
                        animComp->speed = animJson.value("speed", 1.0f);
                        animComp->playing = animJson.value("autoPlay", true);
                        animComp->needsInit = true;
                    }
                }
            }
        }

        result.success = result.errors.empty();
        if (result.success)
            KE_LOG_INFO("Scene loaded: {} ({} entities)", scenePath, result.entityCount);
        else
            KE_LOG_ERROR("Scene load completed with {} errors", result.errors.size());
        return result;
    }

    void SetDefaultLight(const LightData& light) { m_defaultLight = light; }
    bool HasDefaultLight() const { return m_defaultLight.has_value(); }
    const LightData& GetDefaultLight() const { return m_defaultLight.value(); }

    bool SaveToJson(const std::string& scenePath, const AssetStore* assetStore = nullptr) const
    {
        nlohmann::json sceneJson;

        if (m_defaultLight)
        {
            sceneJson["light"]["direction"] = {
                m_defaultLight->direction.x, m_defaultLight->direction.y, m_defaultLight->direction.z };
            sceneJson["light"]["color"] = {
                m_defaultLight->color.x, m_defaultLight->color.y, m_defaultLight->color.z };
            sceneJson["light"]["intensity"] = m_defaultLight->intensity;
            sceneJson["light"]["ambient"] = {
                m_defaultLight->ambientColor.x, m_defaultLight->ambientColor.y, m_defaultLight->ambientColor.z };

            if (m_defaultLight->pointLightCount > 0)
            {
                nlohmann::json pointLightsJson = nlohmann::json::array();
                for (int i = 0; i < m_defaultLight->pointLightCount; ++i)
                {
                    nlohmann::json pl;
                    const auto& light = m_defaultLight->pointLights[i];
                    pl["position"] = { light.position.x, light.position.y, light.position.z };
                    pl["color"] = { light.color.x, light.color.y, light.color.z };
                    pl["range"] = light.range;
                    pl["intensity"] = light.intensity;
                    pointLightsJson.push_back(pl);
                }
                sceneJson["light"]["pointLights"] = pointLightsJson;
            }
        }

        nlohmann::json entitiesJson = nlohmann::json::array();
        for (const auto& entity : m_entities)
        {
            nlohmann::json entJson;
            entJson["name"] = entity->GetName();

            auto* transform = entity->GetTransform();
            entJson["transform"]["position"] = {
                transform->transform.position.x,
                transform->transform.position.y,
                transform->transform.position.z };
            glm::vec3 euler = glm::eulerAngles(transform->transform.rotation);
            entJson["transform"]["rotation"] = { euler.x, euler.y, euler.z };
            entJson["transform"]["scale"] = {
                transform->transform.scale.x,
                transform->transform.scale.y,
                transform->transform.scale.z };

            auto* cam = entity->GetCameraComponent();
            if (cam)
            {
                entJson["camera"]["active"] = cam->isActive;
                entJson["camera"]["fov"] = cam->cameraData.fov;
                entJson["camera"]["near"] = cam->cameraData.nearPlane;
                entJson["camera"]["far"] = cam->cameraData.farPlane;
                entJson["camera"]["forward"] = {
                    cam->cameraData.forward.x, cam->cameraData.forward.y, cam->cameraData.forward.z };
            }

            auto* mr = entity->GetMeshRendererComponent();
            if (mr)
            {
                if (assetStore)
                {
                    std::string meshPath = assetStore->FindMeshPath(mr->meshHandle);
                    entJson["mesh"] = meshPath.empty() ? "$default_cube" : meshPath;
                }
                else
                {
                    entJson["mesh"] = "$default_cube";
                }

                if (mr->materialHandle != INVALID_HANDLE && assetStore)
                {
                    auto* matData = assetStore->GetMaterial(mr->materialHandle);
                    if (matData)
                    {
                        nlohmann::json matJson;
                        matJson["albedo"] = { matData->albedo.x, matData->albedo.y, matData->albedo.z };
                        matJson["metallic"] = matData->metallic;
                        matJson["roughness"] = matData->roughness;
                        matJson["ao"] = matData->ao;
                        matJson["emissive"] = { matData->emissive.x, matData->emissive.y, matData->emissive.z };
                        if (!matData->albedoTexturePath.empty())
                            matJson["albedoTexture"] = matData->albedoTexturePath;
                        if (!matData->normalTexturePath.empty())
                            matJson["normalTexture"] = matData->normalTexturePath;
                        if (!matData->metallicRoughnessTexturePath.empty())
                            matJson["metallicRoughnessTexture"] = matData->metallicRoughnessTexturePath;
                        entJson["material"] = matJson;
                    }
                }
            }

            auto* tc = entity->GetTerrainComponent();
            if (tc)
            {
                entJson["terrain"]["cellSize"] = tc->cellSize;
            }

            auto* sm = entity->GetSkeletalMeshComponent();
            auto* anim = entity->GetAnimatorComponent();
            if (sm)
            {
                if (assetStore)
                {
                    std::string skelMeshPath = assetStore->FindSkinnedMeshPath(sm->skeletalMeshHandle);
                    entJson["skeletalMesh"]["path"] = skelMeshPath.empty() ? "$skelmesh" : skelMeshPath;
                }
                else
                {
                    entJson["skeletalMesh"]["path"] = "$skelmesh";
                }
            }
            if (anim && assetStore)
            {
                std::string skelPath = assetStore->FindSkeletonPath(anim->skeletonHandle);
                std::string clipPath = assetStore->FindAnimationClipPath(anim->currentClipHandle);
                if (!skelPath.empty()) entJson["skeletalMesh"]["skeleton"] = skelPath;
                if (!clipPath.empty())
                {
                    entJson["animation"]["clip"] = clipPath;
                    entJson["animation"]["loop"] = anim->loop;
                    entJson["animation"]["speed"] = anim->speed;
                    entJson["animation"]["autoPlay"] = anim->playing;
                }
            }

            entitiesJson.push_back(entJson);
        }
        sceneJson["entities"] = entitiesJson;

        return FileSystem::WriteTextFile(scenePath, sceneJson.dump(2));
    }

private:
    EntityId m_nextEntityId = 1;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityId, Entity*> m_entityMap;
    std::optional<LightData> m_defaultLight;
    const AssetStore* m_assetStore = nullptr;
    static AssetStore* s_assetStore;
};
}
