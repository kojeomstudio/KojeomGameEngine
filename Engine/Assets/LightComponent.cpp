#include "LightComponent.h"
#include "../Graphics/Renderer.h"
#include "../Core/Engine.h"

KLightComponent::KLightComponent()
{
    PointLight = FPointLight(XMFLOAT3(0, 5, 0), XMFLOAT4(1, 1, 1, 1), 10.0f, 1.0f);
}

void KLightComponent::Tick(float DeltaTime)
{
    if (!Owner) return;

    XMFLOAT3 actorPos = Owner->GetWorldTransform().Position;
    XMVECTOR rotVec = XMLoadFloat4(&Owner->GetWorldTransform().Rotation);
    XMFLOAT3 forward(0, 0, -1);
    XMVECTOR fwdVec = XMLoadFloat3(&forward);
    fwdVec = XMVector3Rotate(fwdVec, rotVec);
    XMFLOAT3 direction;
    XMStoreFloat3(&direction, fwdVec);

    switch (LightType)
    {
    case ELightType::Point:
        PointLight.Position = actorPos;
        break;
    case ELightType::Spot:
        SpotLight.Position = actorPos;
        SpotLight.Direction = direction;
        break;
    case ELightType::Directional:
        DirectionalLight.Direction = direction;
        break;
    }
}

void KLightComponent::Render(KRenderer* Renderer)
{
    if (!Renderer) return;

    switch (LightType)
    {
    case ELightType::Directional:
        Renderer->SetDirectionalLight(DirectionalLight);
        break;
    case ELightType::Point:
        Renderer->AddPointLight(PointLight);
        break;
    case ELightType::Spot:
        Renderer->AddSpotLight(SpotLight);
        break;
    }
}

void KLightComponent::SetIntensity(float InIntensity)
{
    switch (LightType)
    {
    case ELightType::Directional:
        DirectionalLight.Intensity = InIntensity;
        break;
    case ELightType::Point:
        PointLight.Intensity = InIntensity;
        break;
    case ELightType::Spot:
        SpotLight.Intensity = InIntensity;
        break;
    }
}

float KLightComponent::GetIntensity() const
{
    switch (LightType)
    {
    case ELightType::Directional: return DirectionalLight.Intensity;
    case ELightType::Point: return PointLight.Intensity;
    case ELightType::Spot: return SpotLight.Intensity;
    default: return 0.0f;
    }
}

void KLightComponent::SetColor(float R, float G, float B, float A)
{
    XMFLOAT4 color(R, G, B, A);
    switch (LightType)
    {
    case ELightType::Directional:
        DirectionalLight.Color = color;
        break;
    case ELightType::Point:
        PointLight.Color = color;
        break;
    case ELightType::Spot:
        SpotLight.Color = color;
        break;
    default:
        break;
    }
}

void KLightComponent::GetColor(float& R, float& G, float& B, float& A) const
{
    R = 0.0f; G = 0.0f; B = 0.0f; A = 1.0f;
    const XMFLOAT4* color = nullptr;
    switch (LightType)
    {
    case ELightType::Directional: color = &DirectionalLight.Color; break;
    case ELightType::Point: color = &PointLight.Color; break;
    case ELightType::Spot: color = &SpotLight.Color; break;
    default: break;
    }
    if (color)
    {
        R = color->x;
        G = color->y;
        B = color->z;
        A = color->w;
    }
}

void KLightComponent::Serialize(KBinaryArchive& Archive)
{
    KActorComponent::Serialize(Archive);
    uint32 lightTypeVal = static_cast<uint32>(LightType);
    Archive << lightTypeVal;
    Archive << bCastShadow;

    switch (LightType)
    {
    case ELightType::Directional:
        Archive << DirectionalLight.Direction.x << DirectionalLight.Direction.y << DirectionalLight.Direction.z;
        Archive << DirectionalLight.Intensity;
        Archive << DirectionalLight.Color.x << DirectionalLight.Color.y << DirectionalLight.Color.z << DirectionalLight.Color.w;
        Archive << DirectionalLight.AmbientColor.x << DirectionalLight.AmbientColor.y << DirectionalLight.AmbientColor.z << DirectionalLight.AmbientColor.w;
        break;
    case ELightType::Point:
        Archive << PointLight.Intensity;
        Archive << PointLight.Color.x << PointLight.Color.y << PointLight.Color.z << PointLight.Color.w;
        Archive << PointLight.Radius << PointLight.Falloff;
        break;
    case ELightType::Spot:
        Archive << SpotLight.Intensity;
        Archive << SpotLight.Direction.x << SpotLight.Direction.y << SpotLight.Direction.z;
        Archive << SpotLight.Color.x << SpotLight.Color.y << SpotLight.Color.z << SpotLight.Color.w;
        Archive << SpotLight.InnerCone << SpotLight.OuterCone << SpotLight.Radius << SpotLight.Falloff;
        break;
    }
}

void KLightComponent::Deserialize(KBinaryArchive& Archive)
{
    KActorComponent::Deserialize(Archive);
    uint32 lightTypeVal = 0;
    Archive >> lightTypeVal;
    if (lightTypeVal > static_cast<uint32>(ELightType::Spot))
    {
        LOG_ERROR("LightComponent::Deserialize: invalid light type: " + std::to_string(lightTypeVal));
        LightType = ELightType::Directional;
    }
    else
    {
        LightType = static_cast<ELightType>(lightTypeVal);
    }
    Archive >> bCastShadow;

    switch (LightType)
    {
    case ELightType::Directional:
        Archive >> DirectionalLight.Direction.x >> DirectionalLight.Direction.y >> DirectionalLight.Direction.z;
        Archive >> DirectionalLight.Intensity;
        Archive >> DirectionalLight.Color.x >> DirectionalLight.Color.y >> DirectionalLight.Color.z >> DirectionalLight.Color.w;
        Archive >> DirectionalLight.AmbientColor.x >> DirectionalLight.AmbientColor.y >> DirectionalLight.AmbientColor.z >> DirectionalLight.AmbientColor.w;
        break;
    case ELightType::Point:
        Archive >> PointLight.Intensity;
        Archive >> PointLight.Color.x >> PointLight.Color.y >> PointLight.Color.z >> PointLight.Color.w;
        Archive >> PointLight.Radius >> PointLight.Falloff;
        break;
    case ELightType::Spot:
        Archive >> SpotLight.Intensity;
        Archive >> SpotLight.Direction.x >> SpotLight.Direction.y >> SpotLight.Direction.z;
        Archive >> SpotLight.Color.x >> SpotLight.Color.y >> SpotLight.Color.z >> SpotLight.Color.w;
        Archive >> SpotLight.InnerCone >> SpotLight.OuterCone >> SpotLight.Radius >> SpotLight.Falloff;
        break;
    }
}
