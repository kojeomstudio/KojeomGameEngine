#pragma once

#include "../Scene/Actor.h"
#include "../Graphics/Light.h"
#include <memory>

enum class ELightType : uint32
{
    Directional = 0,
    Point = 1,
    Spot = 2
};

class KLightComponent : public KActorComponent
{
public:
    KLightComponent();
    virtual ~KLightComponent() = default;

    virtual void Tick(float DeltaTime) override;

    void SetLightType(ELightType InType) { LightType = InType; }
    ELightType GetLightType() const { return LightType; }

    void SetDirectionalLight(const FDirectionalLight& InLight) { DirectionalLight = InLight; LightType = ELightType::Directional; }
    const FDirectionalLight& GetDirectionalLight() const { return DirectionalLight; }
    FDirectionalLight& GetDirectionalLightMutable() { return DirectionalLight; }

    void SetPointLight(const FPointLight& InLight) { PointLight = InLight; LightType = ELightType::Point; }
    const FPointLight& GetPointLight() const { return PointLight; }
    FPointLight& GetPointLightMutable() { return PointLight; }

    void SetSpotLight(const FSpotLight& InLight) { SpotLight = InLight; LightType = ELightType::Spot; }
    const FSpotLight& GetSpotLight() const { return SpotLight; }
    FSpotLight& GetSpotLightMutable() { return SpotLight; }

    void SetIntensity(float InIntensity);
    float GetIntensity() const;

    void SetColor(float R, float G, float B, float A = 1.0f);
    void GetColor(float& R, float& G, float& B, float& A) const;

    bool GetCastShadow() const { return bCastShadow; }
    void SetCastShadow(bool bCast) { bCastShadow = bCast; }

    virtual EComponentType GetComponentTypeID() const override { return EComponentType::Light; }

    virtual void Serialize(KBinaryArchive& Archive) override;
    virtual void Deserialize(KBinaryArchive& Archive) override;

private:
    ELightType LightType = ELightType::Point;
    FDirectionalLight DirectionalLight;
    FPointLight PointLight;
    FSpotLight SpotLight;
    bool bCastShadow = true;
    bool bRegisteredWithRenderer = false;
};
