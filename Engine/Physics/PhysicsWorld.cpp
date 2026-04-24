#include "PhysicsWorld.h"
#include "../Utils/Logger.h"

KPhysicsWorld::KPhysicsWorld()
{
}

KPhysicsWorld::~KPhysicsWorld()
{
    Shutdown();
}

HRESULT KPhysicsWorld::Initialize(const FPhysicsSettings& InSettings)
{
    Settings = InSettings;
    Bodies.clear();
    PotentialPairs.clear();
    Collisions.clear();
    NextBodyId = 1;
    AccumulatedTime = 0.0f;
    LOG_INFO("Physics world initialized");
    return S_OK;
}

void KPhysicsWorld::Shutdown()
{
    Bodies.clear();
    PotentialPairs.clear();
    Collisions.clear();
    LOG_INFO("Physics world shutdown");
}

void KPhysicsWorld::Update(float DeltaTime)
{
    AccumulatedTime += DeltaTime;

    uint32_t SubSteps = 0;
    while (AccumulatedTime >= Settings.FixedDeltaTime && SubSteps < Settings.MaxSubSteps)
    {
        IntegrateBodies(Settings.FixedDeltaTime);
        BroadPhase();
        NarrowPhase();
        ResolveCollisions();
        AccumulatedTime -= Settings.FixedDeltaTime;
        SubSteps++;
    }
}

uint32_t KPhysicsWorld::CreateRigidBody()
{
    uint32_t BodyId = NextBodyId++;
    auto Body = std::make_unique<KRigidBody>();
    Body->BodyId = BodyId;
    Bodies[BodyId] = std::move(Body);
    return BodyId;
}

void KPhysicsWorld::DestroyRigidBody(uint32_t BodyId)
{
    Bodies.erase(BodyId);
}

KRigidBody* KPhysicsWorld::GetRigidBody(uint32_t BodyId) const
{
    auto It = Bodies.find(BodyId);
    if (It != Bodies.end())
    {
        return It->second.get();
    }
    return nullptr;
}

void KPhysicsWorld::SetGravity(const XMFLOAT3& Gravity)
{
    Settings.Gravity = Gravity;
}

void KPhysicsWorld::IntegrateBodies(float DeltaTime)
{
    for (auto& Pair : Bodies)
    {
        Pair.second->Integrate(DeltaTime, Settings.Gravity);
    }
}

void KPhysicsWorld::BroadPhase()
{
    PotentialPairs.clear();

    std::vector<std::pair<uint32_t, FAABB>> dynamicAABBs;
    std::vector<std::pair<uint32_t, FAABB>> staticAABBs;
    for (const auto& Pair : Bodies)
    {
        if (Pair.second->GetBodyType() == EPhysicsBodyType::Static)
        {
            staticAABBs.push_back({ Pair.first, Pair.second->GetAABB() });
        }
        else
        {
            dynamicAABBs.push_back({ Pair.first, Pair.second->GetAABB() });
        }
    }

    for (size_t i = 0; i < dynamicAABBs.size(); ++i)
    {
        for (size_t j = i + 1; j < dynamicAABBs.size(); ++j)
        {
            if (dynamicAABBs[i].second.Intersects(dynamicAABBs[j].second))
            {
                PotentialPairs.push_back({ dynamicAABBs[i].first, dynamicAABBs[j].first });
            }
        }
    }

    for (size_t i = 0; i < dynamicAABBs.size(); ++i)
    {
        for (size_t j = 0; j < staticAABBs.size(); ++j)
        {
            if (dynamicAABBs[i].second.Intersects(staticAABBs[j].second))
            {
                PotentialPairs.push_back({ dynamicAABBs[i].first, staticAABBs[j].first });
            }
        }
    }
}

void KPhysicsWorld::NarrowPhase()
{
    Collisions.clear();

    for (const auto& Pair : PotentialPairs)
    {
        auto* BodyA = GetRigidBody(Pair.first);
        auto* BodyB = GetRigidBody(Pair.second);

        if (!BodyA || !BodyB)
            continue;

        FCollisionInfo Info;
        Info.BodyA = Pair.first;
        Info.BodyB = Pair.second;

        if (TestCollision(BodyA, BodyB, Info))
        {
            Collisions.push_back(Info);

            if (OnCollision)
            {
                OnCollision(Pair.first, Pair.second, Info);
            }
        }
    }
}

bool KPhysicsWorld::TestCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const
{
    EColliderType TypeA = BodyA->GetColliderType();
    EColliderType TypeB = BodyB->GetColliderType();

    if (TypeA == EColliderType::Sphere && TypeB == EColliderType::Sphere)
    {
        return SphereSphereCollision(BodyA, BodyB, OutInfo);
    }
    else if (TypeA == EColliderType::Sphere && TypeB == EColliderType::Box)
    {
        return SphereBoxCollision(BodyA, BodyB, OutInfo);
    }
    else if (TypeA == EColliderType::Box && TypeB == EColliderType::Sphere)
    {
        bool Result = SphereBoxCollision(BodyB, BodyA, OutInfo);
        OutInfo.ContactNormal = XMFLOAT3(-OutInfo.ContactNormal.x, -OutInfo.ContactNormal.y, -OutInfo.ContactNormal.z);
        std::swap(OutInfo.BodyA, OutInfo.BodyB);
        return Result;
    }
    else if (TypeA == EColliderType::Box && TypeB == EColliderType::Box)
    {
        return BoxBoxCollision(BodyA, BodyB, OutInfo);
    }

    return false;
}

bool KPhysicsWorld::SphereSphereCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const
{
    XMFLOAT3 PosA = BodyA->GetPosition();
    XMFLOAT3 PosB = BodyB->GetPosition();
    float RadiusA = BodyA->GetSphereRadius();
    float RadiusB = BodyB->GetSphereRadius();

    float DX = PosB.x - PosA.x;
    float DY = PosB.y - PosA.y;
    float DZ = PosB.z - PosA.z;

    float DistanceSq = DX * DX + DY * DY + DZ * DZ;
    float RadiusSum = RadiusA + RadiusB;

    if (DistanceSq > RadiusSum * RadiusSum)
    {
        return false;
    }

    float Distance = sqrtf(DistanceSq);
    OutInfo.bCollided = true;

    if (Distance > 0.0001f)
    {
        OutInfo.ContactNormal = XMFLOAT3(DX / Distance, DY / Distance, DZ / Distance);
    }
    else
    {
        OutInfo.ContactNormal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    }

    OutInfo.PenetrationDepth = RadiusSum - Distance;
    OutInfo.ContactPoint = XMFLOAT3(
        PosA.x + OutInfo.ContactNormal.x * RadiusA,
        PosA.y + OutInfo.ContactNormal.y * RadiusA,
        PosA.z + OutInfo.ContactNormal.z * RadiusA
    );

    return true;
}

bool KPhysicsWorld::SphereBoxCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const
{
    XMFLOAT3 SpherePos = BodyA->GetPosition();
    float SphereRadius = BodyA->GetSphereRadius();
    XMFLOAT3 BoxPos = BodyB->GetPosition();
    XMFLOAT3 BoxHalfExtents = BodyB->GetBoxHalfExtents();

    XMFLOAT3 RelPos = XMFLOAT3(
        SpherePos.x - BoxPos.x,
        SpherePos.y - BoxPos.y,
        SpherePos.z - BoxPos.z
    );

    XMFLOAT3 ClosestPoint = XMFLOAT3(
        XMMax(-BoxHalfExtents.x, XMMin(RelPos.x, BoxHalfExtents.x)),
        XMMax(-BoxHalfExtents.y, XMMin(RelPos.y, BoxHalfExtents.y)),
        XMMax(-BoxHalfExtents.z, XMMin(RelPos.z, BoxHalfExtents.z))
    );

    float DX = RelPos.x - ClosestPoint.x;
    float DY = RelPos.y - ClosestPoint.y;
    float DZ = RelPos.z - ClosestPoint.z;

    float DistanceSq = DX * DX + DY * DY + DZ * DZ;

    if (DistanceSq > SphereRadius * SphereRadius)
    {
        return false;
    }

    float Distance = sqrtf(DistanceSq);
    OutInfo.bCollided = true;

    if (Distance > 0.0001f)
    {
        OutInfo.ContactNormal = XMFLOAT3(DX / Distance, DY / Distance, DZ / Distance);
    }
    else
    {
        if (fabsf(RelPos.x) > fabsf(RelPos.y) && fabsf(RelPos.x) > fabsf(RelPos.z))
        {
            OutInfo.ContactNormal = XMFLOAT3(RelPos.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else if (fabsf(RelPos.y) > fabsf(RelPos.z))
        {
            OutInfo.ContactNormal = XMFLOAT3(0.0f, RelPos.y > 0 ? 1.0f : -1.0f, 0.0f);
        }
        else
        {
            OutInfo.ContactNormal = XMFLOAT3(0.0f, 0.0f, RelPos.z > 0 ? 1.0f : -1.0f);
        }
    }

    OutInfo.PenetrationDepth = SphereRadius - Distance;
    OutInfo.ContactPoint = XMFLOAT3(
        BoxPos.x + ClosestPoint.x,
        BoxPos.y + ClosestPoint.y,
        BoxPos.z + ClosestPoint.z
    );

    return true;
}

bool KPhysicsWorld::BoxBoxCollision(const KRigidBody* BodyA, const KRigidBody* BodyB, FCollisionInfo& OutInfo) const
{
    XMFLOAT3 PosA = BodyA->GetPosition();
    XMFLOAT3 HalfA = BodyA->GetBoxHalfExtents();
    XMFLOAT3 PosB = BodyB->GetPosition();
    XMFLOAT3 HalfB = BodyB->GetBoxHalfExtents();

    XMFLOAT3 RelPos = XMFLOAT3(PosB.x - PosA.x, PosB.y - PosA.y, PosB.z - PosA.z);

    float OverlapX = HalfA.x + HalfB.x - fabsf(RelPos.x);
    float OverlapY = HalfA.y + HalfB.y - fabsf(RelPos.y);
    float OverlapZ = HalfA.z + HalfB.z - fabsf(RelPos.z);

    if (OverlapX <= 0.0f || OverlapY <= 0.0f || OverlapZ <= 0.0f)
    {
        return false;
    }

    OutInfo.bCollided = true;

    if (OverlapX <= OverlapY && OverlapX <= OverlapZ)
    {
        OutInfo.PenetrationDepth = OverlapX;
        OutInfo.ContactNormal = XMFLOAT3(RelPos.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
    }
    else if (OverlapY <= OverlapZ)
    {
        OutInfo.PenetrationDepth = OverlapY;
        OutInfo.ContactNormal = XMFLOAT3(0.0f, RelPos.y > 0 ? 1.0f : -1.0f, 0.0f);
    }
    else
    {
        OutInfo.PenetrationDepth = OverlapZ;
        OutInfo.ContactNormal = XMFLOAT3(0.0f, 0.0f, RelPos.z > 0 ? 1.0f : -1.0f);
    }

    OutInfo.ContactPoint = XMFLOAT3(
        (PosA.x + PosB.x) * 0.5f,
        (PosA.y + PosB.y) * 0.5f,
        (PosA.z + PosB.z) * 0.5f
    );

    return true;
}

void KPhysicsWorld::ResolveCollisions()
{
    for (const auto& Collision : Collisions)
    {
        auto* BodyA = GetRigidBody(Collision.BodyA);
        auto* BodyB = GetRigidBody(Collision.BodyB);

        if (!BodyA || !BodyB)
            continue;

        if (BodyA->GetIsTrigger() || BodyB->GetIsTrigger())
            continue;

        ResolveCollision(BodyA, BodyB, Collision);
        PositionalCorrection(BodyA, BodyB, Collision);
    }
}

void KPhysicsWorld::ResolveCollision(KRigidBody* BodyA, KRigidBody* BodyB, const FCollisionInfo& Info)
{
    XMFLOAT3 VelA = BodyA->GetVelocity();
    XMFLOAT3 VelB = BodyB->GetVelocity();

    XMFLOAT3 RelativeVel = XMFLOAT3(
        VelA.x - VelB.x,
        VelA.y - VelB.y,
        VelA.z - VelB.z
    );

    float VelAlongNormal = RelativeVel.x * Info.ContactNormal.x +
                           RelativeVel.y * Info.ContactNormal.y +
                           RelativeVel.z * Info.ContactNormal.z;

    if (VelAlongNormal > 0.0f)
    {
        return;
    }

    float E = XMMin(BodyA->GetMaterial().Restitution, BodyB->GetMaterial().Restitution);

    float InvMassA = BodyA->GetInverseMass();
    float InvMassB = BodyB->GetInverseMass();

    if (BodyA->GetBodyType() == EPhysicsBodyType::Static) InvMassA = 0.0f;
    if (BodyB->GetBodyType() == EPhysicsBodyType::Static) InvMassB = 0.0f;

    float TotalInvMass = InvMassA + InvMassB;
    if (TotalInvMass <= 0.0f)
    {
        return;
    }

    float J = -(1.0f + E) * VelAlongNormal / TotalInvMass;

    XMFLOAT3 Impulse = XMFLOAT3(
        J * Info.ContactNormal.x,
        J * Info.ContactNormal.y,
        J * Info.ContactNormal.z
    );

    if (InvMassA > 0.0f)
    {
        VelA.x += Impulse.x * InvMassA;
        VelA.y += Impulse.y * InvMassA;
        VelA.z += Impulse.z * InvMassA;
        BodyA->SetVelocity(VelA);
    }

    if (InvMassB > 0.0f)
    {
        VelB.x -= Impulse.x * InvMassB;
        VelB.y -= Impulse.y * InvMassB;
        VelB.z -= Impulse.z * InvMassB;
        BodyB->SetVelocity(VelB);
    }
}

void KPhysicsWorld::PositionalCorrection(KRigidBody* BodyA, KRigidBody* BodyB, const FCollisionInfo& Info)
{
    float InvMassA = BodyA->GetInverseMass();
    float InvMassB = BodyB->GetInverseMass();

    if (BodyA->GetBodyType() == EPhysicsBodyType::Static) InvMassA = 0.0f;
    if (BodyB->GetBodyType() == EPhysicsBodyType::Static) InvMassB = 0.0f;

    float TotalInvMass = InvMassA + InvMassB;
    if (TotalInvMass <= 0.0f)
    {
        return;
    }

    float Slop = Settings.Slop;
    float Percent = Settings.BaumgarteStabilization;

    float Correction = XMMax(Info.PenetrationDepth - Slop, 0.0f) / TotalInvMass * Percent;

    XMFLOAT3 CorrectionVec = XMFLOAT3(
        Correction * Info.ContactNormal.x,
        Correction * Info.ContactNormal.y,
        Correction * Info.ContactNormal.z
    );

    if (InvMassA > 0.0f)
    {
        XMFLOAT3 PosA = BodyA->GetPosition();
        BodyA->SetPosition(XMFLOAT3(
            PosA.x - CorrectionVec.x * InvMassA,
            PosA.y - CorrectionVec.y * InvMassA,
            PosA.z - CorrectionVec.z * InvMassA
        ));
    }

    if (InvMassB > 0.0f)
    {
        XMFLOAT3 PosB = BodyB->GetPosition();
        BodyB->SetPosition(XMFLOAT3(
            PosB.x + CorrectionVec.x * InvMassB,
            PosB.y + CorrectionVec.y * InvMassB,
            PosB.z + CorrectionVec.z * InvMassB
        ));
    }
}

bool KPhysicsWorld::Raycast(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const
{
    OutHit.bHit = false;
    OutHit.Distance = MaxDistance;

    XMVECTOR RayOrigin = XMLoadFloat3(&Origin);
    XMVECTOR RayDir = XMVector3Normalize(XMLoadFloat3(&Direction));

    for (const auto& Pair : Bodies)
    {
        const KRigidBody* Body = Pair.second.get();
        
        if (Body->GetColliderType() == EColliderType::Sphere)
        {
            XMFLOAT3 SpherePos = Body->GetPosition();
            XMVECTOR SphereCenter = XMLoadFloat3(&SpherePos);
            float Radius = Body->GetSphereRadius();

            XMVECTOR ToSphere = SphereCenter - RayOrigin;
            float tca = XMVectorGetX(XMVector3Dot(ToSphere, RayDir));

            if (tca < 0.0f)
                continue;

            float d2 = XMVectorGetX(XMVector3LengthSq(ToSphere)) - tca * tca;
            float Radius2 = Radius * Radius;

            if (d2 > Radius2)
                continue;

            float thc = sqrtf(Radius2 - d2);
            float t = tca - thc;

            if (t > 0.0f && t < OutHit.Distance)
            {
                OutHit.bHit = true;
                OutHit.Distance = t;
                OutHit.BodyId = Body->GetBodyId();

                XMVECTOR HitPoint = RayOrigin + RayDir * t;
                XMVECTOR Normal = XMVector3Normalize(HitPoint - SphereCenter);
                XMStoreFloat3(&OutHit.Point, HitPoint);
                XMStoreFloat3(&OutHit.Normal, Normal);
            }
        }
        else if (Body->GetColliderType() == EColliderType::Box)
        {
            XMFLOAT3 BoxPos = Body->GetPosition();
            XMFLOAT3 BoxHalfExtents = Body->GetBoxHalfExtents();
            
            XMFLOAT3 BoxMinF = XMFLOAT3(
                BoxPos.x - BoxHalfExtents.x,
                BoxPos.y - BoxHalfExtents.y,
                BoxPos.z - BoxHalfExtents.z
            );
            XMFLOAT3 BoxMaxF = XMFLOAT3(
                BoxPos.x + BoxHalfExtents.x,
                BoxPos.y + BoxHalfExtents.y,
                BoxPos.z + BoxHalfExtents.z
            );
            
            XMVECTOR BoxMin = XMLoadFloat3(&BoxMinF);
            XMVECTOR BoxMax = XMLoadFloat3(&BoxMaxF);

            XMVECTOR InvDir = XMVectorReciprocal(RayDir);
            XMVECTOR T1 = (BoxMin - RayOrigin) * InvDir;
            XMVECTOR T2 = (BoxMax - RayOrigin) * InvDir;

            XMVECTOR TMin = XMVectorMin(T1, T2);
            XMVECTOR TMax = XMVectorMax(T1, T2);

            float tNear = XMMax(XMMax(XMVectorGetX(TMin), XMVectorGetY(TMin)), XMVectorGetZ(TMin));
            float tFar = XMMin(XMMin(XMVectorGetX(TMax), XMVectorGetY(TMax)), XMVectorGetZ(TMax));

            if (tNear > tFar || tFar < 0.0f)
                continue;

            float t = tNear > 0.0f ? tNear : tFar;

            if (t > 0.0f && t < OutHit.Distance)
            {
                OutHit.bHit = true;
                OutHit.Distance = t;
                OutHit.BodyId = Body->GetBodyId();

                XMVECTOR HitPoint = RayOrigin + RayDir * t;
                XMStoreFloat3(&OutHit.Point, HitPoint);

                XMVECTOR Center = XMLoadFloat3(&BoxPos);
                XMVECTOR HalfExtents = XMLoadFloat3(&BoxHalfExtents);
                XMVECTOR LocalPoint = (HitPoint - Center) / HalfExtents;

                XMFLOAT3 Local;
                XMStoreFloat3(&Local, LocalPoint);

                XMFLOAT3 Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
                if (fabsf(Local.x) > fabsf(Local.y) && fabsf(Local.x) > fabsf(Local.z))
                {
                    Normal.x = Local.x > 0 ? 1.0f : -1.0f;
                }
                else if (fabsf(Local.y) > fabsf(Local.z))
                {
                    Normal.y = Local.y > 0 ? 1.0f : -1.0f;
                }
                else
                {
                    Normal.z = Local.z > 0 ? 1.0f : -1.0f;
                }
                OutHit.Normal = Normal;
            }
        }
    }

    return OutHit.bHit;
}

bool KPhysicsWorld::SphereCast(const XMFLOAT3& Origin, float Radius, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const
{
    (void)Origin;
    (void)Radius;
    (void)Direction;
    (void)MaxDistance;
    OutHit.bHit = false;
    return false;
}

std::vector<FPhysicsRaycastHit> KPhysicsWorld::RaycastAll(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float MaxDistance) const
{
    std::vector<FPhysicsRaycastHit> Hits;

    for (const auto& Pair : Bodies)
    {
        FPhysicsRaycastHit Hit;
        Hit.Distance = MaxDistance;
        Hit.bHit = false;

        const KRigidBody* Body = Pair.second.get();
        XMVECTOR RayOrigin = XMLoadFloat3(&Origin);
        XMVECTOR RayDir = XMVector3Normalize(XMLoadFloat3(&Direction));

        bool bHit = false;
        if (Body->GetColliderType() == EColliderType::Sphere)
        {
            XMFLOAT3 SpherePos = Body->GetPosition();
            XMVECTOR SphereCenter = XMLoadFloat3(&SpherePos);
            float Radius = Body->GetSphereRadius();
            XMVECTOR ToSphere = SphereCenter - RayOrigin;
            float tca = XMVectorGetX(XMVector3Dot(ToSphere, RayDir));
            if (tca >= 0.0f)
            {
                float d2 = XMVectorGetX(XMVector3LengthSq(ToSphere)) - tca * tca;
                float Radius2 = Radius * Radius;
                if (d2 <= Radius2)
                {
                    float thc = sqrtf(Radius2 - d2);
                    float t = tca - thc;
                    if (t > 0.0f)
                    {
                        Hit.bHit = true;
                        Hit.Distance = t;
                        Hit.BodyId = Body->GetBodyId();
                        XMVECTOR HitPoint = RayOrigin + RayDir * t;
                        XMVECTOR Normal = XMVector3Normalize(HitPoint - SphereCenter);
                        XMStoreFloat3(&Hit.Point, HitPoint);
                        XMStoreFloat3(&Hit.Normal, Normal);
                        bHit = true;
                    }
                }
            }
        }
        else if (Body->GetColliderType() == EColliderType::Box)
        {
            XMFLOAT3 BoxPos = Body->GetPosition();
            XMFLOAT3 BoxHalfExtents = Body->GetBoxHalfExtents();
            XMFLOAT3 BoxMinF = XMFLOAT3(BoxPos.x - BoxHalfExtents.x, BoxPos.y - BoxHalfExtents.y, BoxPos.z - BoxHalfExtents.z);
            XMFLOAT3 BoxMaxF = XMFLOAT3(BoxPos.x + BoxHalfExtents.x, BoxPos.y + BoxHalfExtents.y, BoxPos.z + BoxHalfExtents.z);
            XMVECTOR BoxMin = XMLoadFloat3(&BoxMinF);
            XMVECTOR BoxMax = XMLoadFloat3(&BoxMaxF);
            XMVECTOR InvDir = XMVectorReciprocal(RayDir);
            XMVECTOR T1 = (BoxMin - RayOrigin) * InvDir;
            XMVECTOR T2 = (BoxMax - RayOrigin) * InvDir;
            XMVECTOR TMin = XMVectorMin(T1, T2);
            XMVECTOR TMax = XMVectorMax(T1, T2);
            float tNear = XMMax(XMMax(XMVectorGetX(TMin), XMVectorGetY(TMin)), XMVectorGetZ(TMin));
            float tFar = XMMin(XMMin(XMVectorGetX(TMax), XMVectorGetY(TMax)), XMVectorGetZ(TMax));
            if (!(tNear > tFar || tFar < 0.0f))
            {
                float t = tNear > 0.0f ? tNear : tFar;
                if (t > 0.0f)
                {
                    Hit.bHit = true;
                    Hit.Distance = t;
                    Hit.BodyId = Body->GetBodyId();
                    XMVECTOR HitPoint = RayOrigin + RayDir * t;
                    XMStoreFloat3(&Hit.Point, HitPoint);
                    XMVECTOR Center = XMLoadFloat3(&BoxPos);
                    XMVECTOR HalfExtents = XMLoadFloat3(&BoxHalfExtents);
                    XMVECTOR LocalPoint = (HitPoint - Center) / HalfExtents;
                    XMFLOAT3 Local;
                    XMStoreFloat3(&Local, LocalPoint);
                    XMFLOAT3 Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
                    if (fabsf(Local.x) > fabsf(Local.y) && fabsf(Local.x) > fabsf(Local.z))
                        Normal.x = Local.x > 0 ? 1.0f : -1.0f;
                    else if (fabsf(Local.y) > fabsf(Local.z))
                        Normal.y = Local.y > 0 ? 1.0f : -1.0f;
                    else
                        Normal.z = Local.z > 0 ? 1.0f : -1.0f;
                    Hit.Normal = Normal;
                    bHit = true;
                }
            }
        }

        if (bHit)
        {
            Hits.push_back(Hit);
        }
    }

    std::sort(Hits.begin(), Hits.end(), [](const FPhysicsRaycastHit& A, const FPhysicsRaycastHit& B) {
        return A.Distance < B.Distance;
    });

    return Hits;
}
