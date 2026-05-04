#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Math/Transform.h"
#include "Renderer/RenderScene.h"

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
                scene.staticDrawCommands.push_back(cmd);
            }

            auto* tc = entity->GetTerrainComponent();
            if (tc && tc->terrainHandle != INVALID_HANDLE)
            {
                TerrainDrawCommand cmd;
                cmd.terrainHandle = tc->terrainHandle;
                cmd.materialHandle = tc->materialHandle;
                cmd.worldMatrix = entity->GetTransform()->transform.ToMatrix();
                scene.terrainDrawCommands.push_back(cmd);
            }

            auto* sm = entity->GetSkeletalMeshComponent();
            if (sm && sm->skeletalMeshHandle != INVALID_HANDLE)
            {
                SkinnedDrawCommand cmd;
                cmd.skeletalMeshHandle = sm->skeletalMeshHandle;
                cmd.materialHandle = sm->materialHandle;
                cmd.worldMatrix = entity->GetTransform()->transform.ToMatrix();
                cmd.boneCount = sm->boneCount;
                scene.skinnedDrawCommands.push_back(cmd);
            }
        }

        return scene;
    }

    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_entities; }
    size_t GetEntityCount() const { return m_entities.size(); }

    void Clear()
    {
        m_entities.clear();
        m_entityMap.clear();
    }

private:
    EntityId m_nextEntityId = 1;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityId, Entity*> m_entityMap;
};
}
