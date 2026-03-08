#include "RigidBody.h"

KRigidBody::KRigidBody()
{
}

void KRigidBody::SetPosition(const XMFLOAT3& InPosition)
{
    Position = InPosition;
}

void KRigidBody::SetRotation(const XMFLOAT4& InRotation)
{
    Rotation = InRotation;
}

void KRigidBody::SetVelocity(const XMFLOAT3& InVelocity)
{
    Velocity = InVelocity;
}

void KRigidBody::SetAngularVelocity(const XMFLOAT3& InAngularVelocity)
{
    AngularVelocity = InAngularVelocity;
}

void KRigidBody::SetMass(float InMass)
{
    if (InMass <= 0.0f)
    {
        Mass = 0.0f;
        InverseMass = 0.0f;
    }
    else
    {
        Mass = InMass;
        InverseMass = 1.0f / InMass;
    }
}

void KRigidBody::SetSphereRadius(float InRadius)
{
    ColliderType = EColliderType::Sphere;
    SphereRadius = InRadius;
}

void KRigidBody::SetBoxHalfExtents(const XMFLOAT3& InHalfExtents)
{
    ColliderType = EColliderType::Box;
    BoxHalfExtents = InHalfExtents;
}

void KRigidBody::SetCapsuleParams(float InRadius, float InHalfHeight)
{
    ColliderType = EColliderType::Capsule;
    CapsuleRadius = InRadius;
    CapsuleHalfHeight = InHalfHeight;
}

void KRigidBody::GetCapsuleParams(float& OutRadius, float& OutHalfHeight) const
{
    OutRadius = CapsuleRadius;
    OutHalfHeight = CapsuleHalfHeight;
}

void KRigidBody::ApplyForce(const XMFLOAT3& InForce)
{
    ForceAccumulator.x += InForce.x;
    ForceAccumulator.y += InForce.y;
    ForceAccumulator.z += InForce.z;
}

void KRigidBody::ApplyImpulse(const XMFLOAT3& InImpulse)
{
    Velocity.x += InImpulse.x * InverseMass;
    Velocity.y += InImpulse.y * InverseMass;
    Velocity.z += InImpulse.z * InverseMass;
}

void KRigidBody::ApplyTorque(const XMFLOAT3& InTorque)
{
    TorqueAccumulator.x += InTorque.x;
    TorqueAccumulator.y += InTorque.y;
    TorqueAccumulator.z += InTorque.z;
}

void KRigidBody::ApplyAngularImpulse(const XMFLOAT3& InImpulse)
{
    AngularVelocity.x += InImpulse.x * InverseInertiaTensor.x;
    AngularVelocity.y += InImpulse.y * InverseInertiaTensor.y;
    AngularVelocity.z += InImpulse.z * InverseInertiaTensor.z;
}

FAABB KRigidBody::GetAABB() const
{
    switch (ColliderType)
    {
    case EColliderType::Sphere:
        return FSphere(Position, SphereRadius).ToAABB();
    case EColliderType::Box:
        return FBox(Position, BoxHalfExtents).ToAABB();
    case EColliderType::Capsule:
        return FCapsule(Position, CapsuleRadius, CapsuleHalfHeight).ToAABB();
    default:
        return FAABB(Position, Position);
    }
}

void KRigidBody::Integrate(float DeltaTime, const XMFLOAT3& Gravity)
{
    if (BodyType != EPhysicsBodyType::Dynamic || InverseMass <= 0.0f)
    {
        return;
    }

    if (bUseGravity)
    {
        ForceAccumulator.x += Gravity.x * Mass;
        ForceAccumulator.y += Gravity.y * Mass;
        ForceAccumulator.z += Gravity.z * Mass;
    }

    XMFLOAT3 Acceleration = XMFLOAT3(
        ForceAccumulator.x * InverseMass,
        ForceAccumulator.y * InverseMass,
        ForceAccumulator.z * InverseMass
    );

    Velocity.x += Acceleration.x * DeltaTime;
    Velocity.y += Acceleration.y * DeltaTime;
    Velocity.z += Acceleration.z * DeltaTime;

    XMFLOAT3 AngularAcceleration = XMFLOAT3(
        TorqueAccumulator.x * InverseInertiaTensor.x,
        TorqueAccumulator.y * InverseInertiaTensor.y,
        TorqueAccumulator.z * InverseInertiaTensor.z
    );

    AngularVelocity.x += AngularAcceleration.x * DeltaTime;
    AngularVelocity.y += AngularAcceleration.y * DeltaTime;
    AngularVelocity.z += AngularAcceleration.z * DeltaTime;

    const float LinearDamping = 0.98f;
    const float AngularDamping = 0.98f;

    Velocity.x *= LinearDamping;
    Velocity.y *= LinearDamping;
    Velocity.z *= LinearDamping;

    AngularVelocity.x *= AngularDamping;
    AngularVelocity.y *= AngularDamping;
    AngularVelocity.z *= AngularDamping;

    Position.x += Velocity.x * DeltaTime;
    Position.y += Velocity.y * DeltaTime;
    Position.z += Velocity.z * DeltaTime;

    XMVECTOR Quat = XMLoadFloat4(&Rotation);
    XMVECTOR AngVel = XMLoadFloat3(&AngularVelocity);
    float AngVelMag = XMVectorGetX(XMVector3Length(AngVel));
    
    if (AngVelMag > 0.0001f)
    {
        XMVECTOR DeltaQuat = XMQuaternionRotationAxis(AngVel / AngVelMag, AngVelMag * DeltaTime * 0.5f);
        Quat = XMQuaternionMultiply(DeltaQuat, Quat);
        Quat = XMQuaternionNormalize(Quat);
        XMStoreFloat4(&Rotation, Quat);
    }

    ForceAccumulator = XMFLOAT3(0.0f, 0.0f, 0.0f);
    TorqueAccumulator = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

XMMATRIX KRigidBody::GetTransformMatrix() const
{
    XMMATRIX Translation = XMMatrixTranslation(Position.x, Position.y, Position.z);
    XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&Rotation));
    return RotationMatrix * Translation;
}
