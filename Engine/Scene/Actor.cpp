#include "Actor.h"
#include "../Graphics/Renderer.h"
#include "../Assets/StaticMeshComponent.h"
#include "../Assets/SkeletalMeshComponent.h"
#include "../Assets/LightComponent.h"
#include "../Graphics/Water/Water.h"
#include "../Graphics/Terrain/Terrain.h"

void KActorComponent::Serialize(KBinaryArchive& Archive)
{
    uint32 typeID = static_cast<uint32>(GetComponentTypeID());
    Archive << typeID;
}

void KActorComponent::Deserialize(KBinaryArchive& Archive)
{
}

ComponentPtr CreateComponentByType(EComponentType Type)
{
    switch (Type)
    {
    case EComponentType::StaticMesh:
        return std::make_shared<KStaticMeshComponent>();
    case EComponentType::SkeletalMesh:
        return std::make_shared<KSkeletalMeshComponent>();
    case EComponentType::Light:
        return std::make_shared<KLightComponent>();
    case EComponentType::Water:
        return std::make_shared<KWaterComponent>();
    case EComponentType::Terrain:
        return std::make_shared<KTerrainComponent>();
    default:
        return nullptr;
    }
}

void KActor::Tick(float DeltaTime)
{
    for (auto& comp : Components)
    {
        comp->Tick(DeltaTime);
    }

    for (auto& child : Children)
    {
        child->Tick(DeltaTime);
    }
}

void KActor::Render(KRenderer* Renderer)
{
    if (!bVisible) return;

    for (auto& comp : Components)
    {
        comp->Render(Renderer);
    }

    for (auto& child : Children)
    {
        child->Render(Renderer);
    }
}

void KActor::AddComponent(ComponentPtr Component)
{
    if (Component)
    {
        Component->SetOwner(this);
        Components.push_back(std::move(Component));
    }
}

void KActor::RemoveComponent(KActorComponent* Component)
{
    for (auto it = Components.begin(); it != Components.end(); ++it)
    {
        if (it->get() == Component)
        {
            (*it)->SetOwner(nullptr);
            Components.erase(it);
            return;
        }
    }
}

void KActor::SetParent(KActor* InParent)
{
    if (Parent == InParent) return;

    if (Parent)
    {
        Parent->RemoveChild(this);
    }

    Parent = InParent;
}

void KActor::AddChild(ActorPtr Child)
{
    if (Child)
    {
        Child->Parent = this;
        Children.push_back(std::move(Child));
    }
}

void KActor::RemoveChild(KActor* Child)
{
    for (auto it = Children.begin(); it != Children.end(); ++it)
    {
        if (it->get() == Child)
        {
            (*it)->Parent = nullptr;
            Children.erase(it);
            return;
        }
    }
}

void KActor::Serialize(KBinaryArchive& Archive)
{
    Archive << Name;
    Archive << WorldTransform.Position.x << WorldTransform.Position.y << WorldTransform.Position.z;
    Archive << WorldTransform.Rotation.x << WorldTransform.Rotation.y << WorldTransform.Rotation.z << WorldTransform.Rotation.w;
    Archive << WorldTransform.Scale.x << WorldTransform.Scale.y << WorldTransform.Scale.z;
    Archive << bVisible;

    uint32 NumComponents = static_cast<uint32>(Components.size());
    Archive << NumComponents;

    for (auto& comp : Components)
    {
        comp->Serialize(Archive);
    }

    uint32 NumChildren = static_cast<uint32>(Children.size());
    Archive << NumChildren;

    for (auto& child : Children)
    {
        child->Serialize(Archive);
    }
}

void KActor::Deserialize(KBinaryArchive& Archive)
{
    Archive >> Name;
    Archive >> WorldTransform.Position.x >> WorldTransform.Position.y >> WorldTransform.Position.z;
    Archive >> WorldTransform.Rotation.x >> WorldTransform.Rotation.y >> WorldTransform.Rotation.z >> WorldTransform.Rotation.w;
    Archive >> WorldTransform.Scale.x >> WorldTransform.Scale.y >> WorldTransform.Scale.z;
    Archive >> bVisible;

    uint32 NumComponents;
    Archive >> NumComponents;

    for (uint32 i = 0; i < NumComponents; ++i)
    {
        uint32 componentTypeID = 0;
        Archive >> componentTypeID;

        ComponentPtr component = CreateComponentByType(static_cast<EComponentType>(componentTypeID));
        if (component)
        {
            component->Deserialize(Archive);
            AddComponent(component);
        }
    }

    uint32 NumChildren;
    Archive >> NumChildren;

    for (uint32 i = 0; i < NumChildren; ++i)
    {
        auto child = std::make_shared<KActor>();
        child->Deserialize(Archive);
        child->Parent = this;
        Children.push_back(std::move(child));
    }
}

HRESULT KScene::Load(const std::wstring& Path)
{
    KBinaryArchive Archive(KBinaryArchive::EMode::Read);

    if (!Archive.Open(Path))
    {
        LOG_ERROR("Failed to open scene file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    uint32 Version;
    Archive >> Version;

    if (Version != SCENE_VERSION)
    {
        LOG_ERROR("Unsupported scene version: " + std::to_string(Version));
        Archive.Close();
        return E_FAIL;
    }

    Archive >> Name;

    Clear();

    uint32 NumRootActors;
    Archive >> NumRootActors;

    for (uint32 i = 0; i < NumRootActors; ++i)
    {
        auto actor = std::make_shared<KActor>();
        actor->Deserialize(Archive);
        AddActor(actor);
    }

    Archive.Close();

    LOG_INFO("Loaded scene: " + Name + " with " + std::to_string(Actors.size()) + " actors");
    return S_OK;
}

HRESULT KScene::Save(const std::wstring& Path)
{
    KBinaryArchive Archive(KBinaryArchive::EMode::Write);

    if (!Archive.Open(Path))
    {
        LOG_ERROR("Failed to create scene file: " + StringUtils::WideToMultiByte(Path));
        return E_FAIL;
    }

    Archive << SCENE_VERSION;
    Archive << Name;

    uint32 NumRootActors = 0;
    for (auto& actor : Actors)
    {
        if (!actor->GetParent())
        {
            ++NumRootActors;
        }
    }
    Archive << NumRootActors;

    for (auto& actor : Actors)
    {
        if (!actor->GetParent())
        {
            actor->Serialize(Archive);
        }
    }

    Archive.Close();

    LOG_INFO("Saved scene: " + Name);
    return S_OK;
}

void KScene::Tick(float DeltaTime)
{
    for (auto& actor : Actors)
    {
        if (!actor->GetParent())
        {
            actor->Tick(DeltaTime);
        }
    }
}

void KScene::Render(KRenderer* Renderer)
{
    for (auto& actor : Actors)
    {
        if (!actor->GetParent())
        {
            actor->Render(Renderer);
        }
    }
}

ActorPtr KScene::CreateActor(const std::string& ActorName)
{
    auto actor = std::make_shared<KActor>();
    actor->SetName(ActorName);
    AddActor(actor);
    return actor;
}

    void KScene::AddActor(ActorPtr Actor)
    {
        if (!Actor) return;

        const std::string& actorName = Actor->GetName();
        auto it = ActorMap.find(actorName);
        if (it != ActorMap.end())
        {
            ActorMap.erase(it);
            for (auto ait = Actors.begin(); ait != Actors.end(); ++ait)
            {
                if ((*ait)->GetName() == actorName)
                {
                    Actors.erase(ait);
                    break;
                }
            }
        }

        ActorMap[actorName] = Actor;
        Actor->BeginPlay();
        Actors.push_back(std::move(Actor));
    }

    void KScene::RemoveActor(const std::string& ActorName)
    {
        auto it = ActorMap.find(ActorName);
        if (it != ActorMap.end())
        {
            ActorPtr actor = it->second;

            if (actor->GetParent())
            {
                actor->GetParent()->RemoveChild(actor.get());
            }

            auto childrenCopy = actor->GetChildren();
            for (auto& child : childrenCopy)
            {
                child->SetParent(nullptr);
                const std::string& childName = child->GetName();
                if (!childName.empty())
                {
                    auto childIt = ActorMap.find(childName);
                    if (childIt != ActorMap.end() && childIt->second == child)
                    {
                        ActorMap.erase(childIt);
                    }
                }
                for (auto ait = Actors.begin(); ait != Actors.end(); ++ait)
                {
                    if (*ait == child)
                    {
                        ait = Actors.erase(ait);
                        break;
                    }
                }
            }

            ActorMap.erase(it);

            for (auto ait = Actors.begin(); ait != Actors.end(); ++ait)
            {
                if (*ait == actor)
                {
                    Actors.erase(ait);
                    break;
                }
            }
        }
    }

void KScene::RemoveActor(KActor* Actor)
{
    if (Actor)
    {
        RemoveActor(Actor->GetName());
    }
}

ActorPtr KScene::FindActor(const std::string& ActorName) const
{
    auto it = ActorMap.find(ActorName);
    if (it != ActorMap.end())
    {
        return it->second;
    }
    return nullptr;
}

KActor* KScene::FindActorRaw(const std::string& ActorName) const
{
    auto it = ActorMap.find(ActorName);
    if (it != ActorMap.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void KScene::Clear()
{
    Actors.clear();
    ActorMap.clear();
}


