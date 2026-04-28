#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include "../Utils/Math.h"
#include "../Serialization/BinaryArchive.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

class KActor;
class KActorComponent;
class KScene;

enum class EComponentType : uint32
{
    Base = 0,
    StaticMesh = 1,
    SkeletalMesh = 2,
    Water = 3,
    Terrain = 4,
    Light = 5,
};

using ActorPtr = std::shared_ptr<KActor>;
using ComponentPtr = std::shared_ptr<KActorComponent>;

ComponentPtr CreateComponentByType(EComponentType Type);

constexpr uint32 SCENE_VERSION = 1;

class KActorComponent
{
public:
    virtual ~KActorComponent() = default;

    virtual void Tick(float DeltaTime) {}
    virtual void Render(class KRenderer* Renderer) {}

    virtual EComponentType GetComponentTypeID() const { return EComponentType::Base; }

    void SetOwner(KActor* InOwner) { Owner = InOwner; }
    KActor* GetOwner() const { return Owner; }

    virtual void Serialize(KBinaryArchive& Archive);
    virtual void Deserialize(KBinaryArchive& Archive);

protected:
    KActor* Owner = nullptr;
};

class KActor : public std::enable_shared_from_this<KActor>
{
public:
    KActor() = default;
    virtual ~KActor() = default;

    KActor(const KActor&) = delete;
    KActor& operator=(const KActor&) = delete;

    virtual void BeginPlay() {}
    virtual void Tick(float DeltaTime);
    virtual void Render(class KRenderer* Renderer);

    void AddComponent(ComponentPtr Component);
    void RemoveComponent(KActorComponent* Component);

    template<typename T>
    T* GetComponent() const
    {
        for (const auto& comp : Components)
        {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) return casted;
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() const
    {
        std::vector<T*> result;
        for (const auto& comp : Components)
        {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) result.push_back(casted);
        }
        return result;
    }

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void SetWorldTransform(const FTransform& InTransform) { WorldTransform = InTransform; bTransformDirty = true; }
    const FTransform& GetWorldTransform() const { return WorldTransform; }
    FTransform& GetWorldTransformMutable() { return WorldTransform; }

    void SetWorldPosition(const XMFLOAT3& Position);
    void SetWorldRotation(const XMFLOAT4& Rotation);
    void SetWorldScale(const XMFLOAT3& Scale);

    XMMATRIX GetWorldMatrix() const;

    void SetLocalPosition(const XMFLOAT3& Position) { LocalTransform.Position = Position; bTransformDirty = true; }
    void SetLocalRotation(const XMFLOAT4& Rotation) { LocalTransform.Rotation = Rotation; bTransformDirty = true; }
    void SetLocalScale(const XMFLOAT3& Scale) { LocalTransform.Scale = Scale; bTransformDirty = true; }

    const FTransform& GetLocalTransform() const { return LocalTransform; }

    void SetParent(KActor* InParent);
    KActor* GetParent() const { return Parent; }

    void AddChild(ActorPtr Child);
    void RemoveChild(KActor* Child);
    const std::vector<ActorPtr>& GetChildren() const { return Children; }
    const std::vector<ComponentPtr>& GetComponents() const { return Components; }

    bool IsVisible() const { return bVisible; }
    void SetVisible(bool bInVisible) { bVisible = bInVisible; }

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);

protected:
    std::string Name;
    FTransform WorldTransform;
    FTransform LocalTransform;
    bool bTransformDirty = false;
    std::vector<ComponentPtr> Components;
    KActor* Parent = nullptr;
    std::vector<ActorPtr> Children;
    bool bVisible = true;
};

class KScene
{
public:
    KScene() = default;
    ~KScene() = default;

    KScene(const KScene&) = delete;
    KScene& operator=(const KScene&) = delete;

    HRESULT Load(const std::wstring& Path);
    HRESULT Save(const std::wstring& Path);

    void Tick(float DeltaTime);
    void Render(class KRenderer* Renderer);

    ActorPtr CreateActor(const std::string& Name);
    void AddActor(ActorPtr Actor);
    void RemoveActor(const std::string& Name);
    void RemoveActor(KActor* Actor);

    ActorPtr FindActor(const std::string& Name) const;
    KActor* FindActorRaw(const std::string& Name) const;

    const std::vector<ActorPtr>& GetActors() const { return Actors; }
    std::vector<ActorPtr>& GetActorsMutable() { return Actors; }

    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }

    void Clear();

    template<typename FuncType>
    void ForEachActor(FuncType&& InFunc)
    {
        for (auto& actor : Actors)
        {
            InFunc(actor);
        }
    }

private:
    std::string Name;
    std::vector<ActorPtr> Actors;
    std::unordered_map<std::string, ActorPtr> ActorMap;
};
