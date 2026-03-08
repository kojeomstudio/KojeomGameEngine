#pragma once

#include <DirectXMath.h>
#include <cstdint>
#include <algorithm>

using namespace DirectX;

enum class EColliderType : uint8_t
{
    None,
    Sphere,
    Box,
    Capsule,
    Plane
};

enum class EPhysicsBodyType : uint8_t
{
    Static,
    Dynamic,
    Kinematic
};

struct FPhysicsMaterial
{
    float Friction = 0.5f;
    float Restitution = 0.3f;
    float Density = 1.0f;
};

struct FPhysicsSettings
{
    XMFLOAT3 Gravity = XMFLOAT3(0.0f, -9.81f, 0.0f);
    float FixedDeltaTime = 1.0f / 60.0f;
    uint32_t MaxSubSteps = 4;
    float BaumgarteStabilization = 0.2f;
    float Slop = 0.01f;
};

struct FCollisionInfo
{
    bool bCollided = false;
    XMFLOAT3 ContactPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 ContactNormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float PenetrationDepth = 0.0f;
    uint32_t BodyA = 0;
    uint32_t BodyB = 0;
};

struct FPhysicsRaycastHit
{
    bool bHit = false;
    XMFLOAT3 Point = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float Distance = 0.0f;
    uint32_t BodyId = 0;
};

struct FAABB
{
    XMFLOAT3 Min = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 Max = XMFLOAT3(0.0f, 0.0f, 0.0f);

    FAABB() = default;
    FAABB(const XMFLOAT3& InMin, const XMFLOAT3& InMax) : Min(InMin), Max(InMax) {}

    XMFLOAT3 GetCenter() const
    {
        return XMFLOAT3((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f, (Min.z + Max.z) * 0.5f);
    }

    XMFLOAT3 GetExtents() const
    {
        return XMFLOAT3((Max.x - Min.x) * 0.5f, (Max.y - Min.y) * 0.5f, (Max.z - Min.z) * 0.5f);
    }

    bool Intersects(const FAABB& Other) const
    {
        return (Min.x <= Other.Max.x && Max.x >= Other.Min.x) &&
               (Min.y <= Other.Max.y && Max.y >= Other.Min.y) &&
               (Min.z <= Other.Max.z && Max.z >= Other.Min.z);
    }
};

struct FSphere
{
    XMFLOAT3 Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float Radius = 0.5f;

    FSphere() = default;
    FSphere(const XMFLOAT3& InCenter, float InRadius) : Center(InCenter), Radius(InRadius) {}

    FAABB ToAABB() const
    {
        return FAABB(
            XMFLOAT3(Center.x - Radius, Center.y - Radius, Center.z - Radius),
            XMFLOAT3(Center.x + Radius, Center.y + Radius, Center.z + Radius)
        );
    }
};

struct FBox
{
    XMFLOAT3 Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3 HalfExtents = XMFLOAT3(0.5f, 0.5f, 0.5f);
    XMFLOAT4 Rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

    FBox() = default;
    FBox(const XMFLOAT3& InCenter, const XMFLOAT3& InHalfExtents)
        : Center(InCenter), HalfExtents(InHalfExtents) {}

    FAABB ToAABB() const
    {
        float MaxExtent = XMMax(XMMax(HalfExtents.x, HalfExtents.y), HalfExtents.z);
        return FAABB(
            XMFLOAT3(Center.x - MaxExtent, Center.y - MaxExtent, Center.z - MaxExtent),
            XMFLOAT3(Center.x + MaxExtent, Center.y + MaxExtent, Center.z + MaxExtent)
        );
    }
};

struct FCapsule
{
    XMFLOAT3 Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float Radius = 0.5f;
    float HalfHeight = 1.0f;

    FCapsule() = default;
    FCapsule(const XMFLOAT3& InCenter, float InRadius, float InHalfHeight)
        : Center(InCenter), Radius(InRadius), HalfHeight(InHalfHeight) {}

    FAABB ToAABB() const
    {
        float TotalHalfHeight = HalfHeight + Radius;
        return FAABB(
            XMFLOAT3(Center.x - Radius, Center.y - TotalHalfHeight, Center.z - Radius),
            XMFLOAT3(Center.x + Radius, Center.y + TotalHalfHeight, Center.z + Radius)
        );
    }
};
