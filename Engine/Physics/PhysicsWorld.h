#pragma once

#include "PhysicsTypes.h"
#include "RigidBody.h"
#include <Windows.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

class KPhysicsWorld
{
public:
    using CollisionCallback = std::function<void(uint32_t, uint32_t, const FCollisionInfo&)>;

    KPhysicsWorld();
    ~KPhysicsWorld();

    HRESULT Initialize(const FPhysicsSettings& Settings = FPhysicsSettings());
    void Shutdown();

    void Update(float DeltaTime);

    uint32_t CreateRigidBody();
    void DestroyRigidBody(uint32_t BodyId);
    KRigidBody* GetRigidBody(uint32_t BodyId) const;

    void SetGravity(const XMFLOAT3& Gravity);
    XMFLOAT3 GetGravity() const { return Settings.Gravity; }

    void SetCollisionCallback(CollisionCallback Callback) { OnCollision = Callback; }

    bool Raycast(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const;
    bool SphereCast(const XMFLOAT3& Origin, float Radius, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const;
    std::vector<FPhysicsRaycastHit> RaycastAll(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float MaxDistance) const;

    size_t GetBodyCount() const { return Bodies.size(); }
    const FPhysicsSettings& GetSettings() const { return Settings; }

private:
    void BroadPhase();
    void NarrowPhase();
    void ResolveCollisions();
    void IntegrateBodies(float DeltaTime);
    void SyncTransforms();

    bool TestCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const;
    bool SphereSphereCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const;
    bool SphereBoxCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const;
    bool BoxBoxCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const;

    void ResolveCollision(KRigidBody* BodyA, KRigidBody* BodyB, const FCollisionInfo& Info);
    void PositionalCorrection(KRigidBody* BodyA, KRigidBody* BodyB, const FCollisionInfo& Info);

    float AccumulatedTime = 0.0f;
    FPhysicsSettings Settings;
    std::unordered_map<uint32_t, std::unique_ptr<KRigidBody>> Bodies;
    std::vector<std::pair<uint32_t, uint32_t>> PotentialPairs;
    std::vector<FCollisionInfo> Collisions;
    uint32_t NextBodyId = 1;

    CollisionCallback OnCollision;
};
