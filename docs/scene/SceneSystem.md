# Scene System

The Scene system provides an Actor-Component architecture for organizing game objects. Actors serve as containers with hierarchical transforms, while Components encapsulate reusable behavior (meshes, physics, audio, etc.).

**Source files:** `Engine/Scene/Actor.h`, `Engine/Scene/Actor.cpp`, `Engine/Scene/SceneManager.h`, `Engine/Scene/SceneManager.cpp`

## Architecture

```
KSceneManager                    owns multiple
├── KScene (active)              owns multiple
│   ├── KActor (root)            owns multiple
│   │   ├── KActorComponent[]
│   │   └── KActor (children)
│   └── KActor (root)
└── KScene
```

Only root actors (those without a parent) are ticked and rendered by `KScene`. Child actors are traversed recursively through the hierarchy.

## EComponentType

```cpp
enum class EComponentType : uint32
{
    Base = 0,
    StaticMesh = 1,
    SkeletalMesh = 2,
    Water = 3,
    Terrain = 4,
    Light = 5,
};
```

Components are instantiated by type ID during deserialization via `CreateComponentByType()`.

| Value | Component Class | Header |
|-------|----------------|--------|
| `StaticMesh` | `KStaticMeshComponent` | `Engine/Assets/StaticMeshComponent.h` |
| `SkeletalMesh` | `KSkeletalMeshComponent` | `Engine/Assets/SkeletalMeshComponent.h` |
| `Water` | `KWaterComponent` | `Engine/Graphics/Water/Water.h` |
| `Terrain` | `KTerrainComponent` | `Engine/Graphics/Terrain/Terrain.h` |
| `Light` | `KLightComponent` | `Engine/Assets/LightComponent.h` |

## KActorComponent

Base class for all components. Attached to a `KActor` to provide behavior.

```cpp
class KActorComponent
{
public:
    virtual ~KActorComponent() = default;

    virtual void Tick(float DeltaTime);
    virtual void Render(KRenderer* Renderer);

    virtual EComponentType GetComponentTypeID() const;

    void SetOwner(KActor* InOwner);
    KActor* GetOwner() const;

    virtual void Serialize(KBinaryArchive& Archive);
    virtual void Deserialize(KBinaryArchive& Archive);

protected:
    KActor* Owner = nullptr;
};
```

### Creating a Custom Component

Override `Tick()`, `Render()`, `GetComponentTypeID()`, and serialization methods. Register the type in `CreateComponentByType()` (`Actor.cpp`) and add a new `EComponentType` entry.

```cpp
class KMyComponent : public KActorComponent
{
public:
    void Tick(float DeltaTime) override
    {
        KActor* owner = GetOwner();
        // update logic
    }

    EComponentType GetComponentTypeID() const override
    {
        return EComponentType::Base; // add new enum value for custom types
    }

    void Serialize(KBinaryArchive& Archive) override
    {
        KActorComponent::Serialize(Archive);
        Archive << MyData;
    }

    void Deserialize(KBinaryArchive& Archive) override
    {
        Archive >> MyData;
    }
};
```

## KActor

An entity in the scene with a transform, visibility state, components, and child actors.

```cpp
class KActor
{
public:
    virtual void BeginPlay();
    virtual void Tick(float DeltaTime);
    virtual void Render(KRenderer* Renderer);

    // Components
    void AddComponent(ComponentPtr Component);
    void RemoveComponent(KActorComponent* Component);
    template<typename T> T* GetComponent() const;
    template<typename T> std::vector<T*> GetComponents() const;

    // Hierarchy
    void SetParent(KActor* InParent);
    KActor* GetParent() const;
    void AddChild(ActorPtr Child);
    void RemoveChild(KActor* Child);
    const std::vector<ActorPtr>& GetChildren() const;

    // Transform
    void SetWorldTransform(const FTransform& InTransform);
    const FTransform& GetWorldTransform() const;
    FTransform& GetWorldTransformMutable();
    void SetWorldPosition(const XMFLOAT3& Position);
    void SetWorldRotation(const XMFLOAT4& Rotation);
    void SetWorldScale(const XMFLOAT3& Scale);
    XMMATRIX GetWorldMatrix() const;

    // Identity
    void SetName(const std::string& InName);
    const std::string& GetName() const;
    bool IsVisible() const;
    void SetVisible(bool bInVisible);

    // Serialization
    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

Ownership is managed via `std::shared_ptr<KActor>` (`ActorPtr`) and `std::shared_ptr<KActorComponent>` (`ComponentPtr`). Copy construction and assignment are deleted.

## KScene

Container for actors. Only ticks/renders root-level actors (parent == nullptr); children are traversed recursively.

```cpp
class KScene
{
public:
    HRESULT Load(const std::wstring& Path);
    HRESULT Save(const std::wstring& Path);
    void Tick(float DeltaTime);
    void Render(KRenderer* Renderer);

    ActorPtr CreateActor(const std::string& Name);
    void AddActor(ActorPtr Actor);
    void RemoveActor(const std::string& Name);
    void RemoveActor(KActor* Actor);
    ActorPtr FindActor(const std::string& Name) const;
    KActor* FindActorRaw(const std::string& Name) const;

    const std::vector<ActorPtr>& GetActors() const;
    std::vector<ActorPtr>& GetActorsMutable();

    void SetName(const std::string& InName);
    const std::string& GetName() const;
    void Clear();

    template<typename FuncType>
    void ForEachActor(FuncType&& InFunc);
};
```

`FindActor` returns a `shared_ptr`; `FindActorRaw` returns a raw pointer for non-owning access.

## KSceneManager

Manages the lifetime of all scenes. Ticks and renders only the active scene.

```cpp
class KSceneManager
{
public:
    HRESULT Initialize();

    std::shared_ptr<KScene> CreateScene(const std::string& Name);
    std::shared_ptr<KScene> LoadScene(const std::wstring& Path);
    HRESULT SaveScene(const std::wstring& Path, std::shared_ptr<KScene> Scene);

    void SetActiveScene(const std::string& Name);
    void SetActiveScene(std::shared_ptr<KScene> Scene);
    std::shared_ptr<KScene> GetActiveScene() const;

    std::shared_ptr<KScene> GetScene(const std::string& Name) const;
    bool HasScene(const std::string& Name) const;

    void UnloadScene(const std::string& Name);
    void UnloadAllScenes();

    void Tick(float DeltaTime);
    void Render(KRenderer* Renderer);

    template<typename FuncType>
    void ForEachScene(FuncType&& InFunc);
};
```

- `CreateScene` returns the existing scene if the name is already in use (logs a warning).
- `LoadScene` extracts the scene name from the filename if the saved scene name is empty.
- `UnloadScene` clears the active scene pointer if unloading the active scene.

## Serialization Format

Scenes serialize to binary via `KBinaryArchive` with the following layout:

```
[uint32  SCENE_VERSION (1)]
[string SceneName]
[uint32  NumRootActors]
  [Actor] (recursive: name, transform, visibility, components, children)
    [string Name]
    [float3 Position]
    [float4 Rotation]
    [float3 Scale]
    [bool   Visible]
    [uint32 NumComponents]
      [uint32 ComponentTypeID]
      [Component data]
    [uint32 NumChildren]
      [Child Actor...] (recursive)
```

Only root actors (no parent) are written during save. The full hierarchy is preserved because each actor serializes its children recursively.

## Code Examples

### Basic Scene Setup

```cpp
KSceneManager& sceneManager = KEngine::GetInstance().GetSceneManager();
sceneManager.Initialize();

auto scene = sceneManager.CreateScene("MainLevel");
sceneManager.SetActiveScene("MainLevel");
```

### Creating an Actor with a Static Mesh

```cpp
auto scene = sceneManager.GetActiveScene();
auto actor = scene->CreateActor("MyCube");

actor->SetWorldPosition({ 0.0f, 5.0f, 0.0f });

auto meshComp = std::make_shared<KStaticMeshComponent>();
meshComp->SetMesh(someMesh);
meshComp->SetMaterial(someMaterial);
actor->AddComponent(meshComp);

auto* found = actor->GetComponent<KStaticMeshComponent>();
```

### Actor Hierarchy

```cpp
auto parent = scene->CreateActor("ParentActor");
auto child = scene->CreateActor("ChildActor");

child->SetWorldPosition({ 2.0f, 0.0f, 0.0f });

parent->AddChild(child);

for (auto& c : parent->GetChildren())
{
    LOG_INFO("Child: " + c->GetName());
}

parent->RemoveChild(child.get());
```

### Iterating Over Actors

```cpp
auto scene = sceneManager.GetActiveScene();

scene->ForEachActor([](ActorPtr& actor)
{
    LOG_INFO("Actor: " + actor->GetName());
});

for (const auto& actor : scene->GetActors())
{
    if (actor->IsVisible())
    {
        actor->Render(renderer);
    }
}
```

### Save and Load

```cpp
sceneManager.SaveScene(L"levels/main.kscene", scene);

auto loaded = sceneManager.LoadScene(L"levels/main.kscene");
sceneManager.SetActiveScene(loaded);
```

### Querying Components Across All Actors

```cpp
scene->ForEachActor([](ActorPtr& actor)
{
    if (auto* mesh = actor->GetComponent<KStaticMeshComponent>())
    {
        mesh->SetCastShadow(true);
    }
});
```
