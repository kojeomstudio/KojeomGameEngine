#pragma once

#include "PhysicsTypes.h"
#include <memory>

class KRigidBody
{
public:
    KRigidBody();
    ~KRigidBody() = default;

    void SetPosition(const XMFLOAT3& Position);
    XMFLOAT3 GetPosition() const { return Position; }

    void SetRotation(const XMFLOAT4& Rotation);
    XMFLOAT4 GetRotation() const { return Rotation; }

    void SetVelocity(const XMFLOAT3& Velocity);
    XMFLOAT3 GetVelocity() const { return Velocity; }

    void SetAngularVelocity(const XMFLOAT3& AngularVelocity);
    XMFLOAT3 GetAngularVelocity() const { return AngularVelocity; }

    void SetMass(float Mass);
    float GetMass() const { return Mass; }
    float GetInverseMass() const { return InverseMass; }

    void SetColliderType(EColliderType Type) { ColliderType = Type; }
    EColliderType GetColliderType() const { return ColliderType; }

    void SetBodyType(EPhysicsBodyType Type) { BodyType = Type; }
    EPhysicsBodyType GetBodyType() const { return BodyType; }

    void SetSphereRadius(float Radius);
    float GetSphereRadius() const { return SphereRadius; }

    void SetBoxHalfExtents(const XMFLOAT3& HalfExtents);
    XMFLOAT3 GetBoxHalfExtents() const { return BoxHalfExtents; }

    void SetCapsuleParams(float Radius, float HalfHeight);
    void GetCapsuleParams(float& OutRadius, float& OutHalfHeight) const;

    void SetMaterial(const FPhysicsMaterial& InMaterial) { Material = InMaterial; }
    FPhysicsMaterial GetMaterial() const { return Material; }

    void SetUseGravity(bool bUseGravity) { bUseGravity = bUseGravity; }
    bool GetUseGravity() const { return bUseGravity; }

    void SetIsTrigger(bool bIsTrigger) { bIsTrigger = bIsTrigger; }
    bool GetIsTrigger() const { return bIsTrigger; }

    uint32_t GetBodyId() const { return BodyId; }

    void ApplyForce(const XMFLOAT3& Force);
    void ApplyImpulse(const XMFLOAT3& Impulse);
    void ApplyTorque(const XMFLOAT3& Torque);
    void ApplyAngularImpulse(const XMFLOAT3& Impulse);

    FAABB GetAABB() const;
    void Integrate(float DeltaTime, const XMFLOAT3& Gravity);

    XMMATRIX GetTransformMatrix() const;

private:
    friend class KPhysicsWorld;

    uint32_t BodyId = 0;
    EPhysicsBodyType BodyType = EPhysicsBodyType::Dynamic;
    EColliderType ColliderType = EColliderType::Sphere;

    XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT4 Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    XMFLOAT3 Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 AngularVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

    XMFLOAT3 ForceAccumulator = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 TorqueAccumulator = XMFLOAT3(0.0f, 0.0f, 0.0f);

    float Mass = 1.0f;
    float InverseMass = 1.0f;
    XMFLOAT3 InertiaTensor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    XMFLOAT3 InverseInertiaTensor = XMFLOAT3(1.0f, 1.0f, 1.0f);

    float SphereRadius = 0.5f;
    XMFLOAT3 BoxHalfExtents = XMFLOAT3(0.5f, 0.5f, 0.5f);
    float CapsuleRadius = 0.5f;
    float CapsuleHalfHeight = 1.0f;

    FPhysicsMaterial Material;
    bool bUseGravity = true;
    bool bIsTrigger = false;
    bool bIsAwake = true;
};
