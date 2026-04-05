# Physics System

> **Source:** `Engine/Physics/` — `PhysicsWorld.h/.cpp`, `RigidBody.h/.cpp`, `PhysicsTypes.h`, `PhysicsSubsystem.h`

## Table of Contents

- [Overview](#overview)
- [Rigid Body (`KRigidBody`)](#rigid-body-krigidbody)
- [Physics World (`KPhysicsWorld`)](#physics-world-kphysicsworld)
- [Collision Types](#collision-types)
- [Physics Types Reference](#physics-types-reference)
- [Physics Subsystem (`KPhysicsSubsystem`)](#physics-subsystem-kphysicssubsystem)
- [Code Examples](#code-examples)

---

## Overview

The physics system provides a lightweight, built-in physics simulation with rigid body dynamics, collision detection, and raycasting. It is designed around three core components:

| Component | File | Role |
|-----------|------|------|
| `KRigidBody` | `RigidBody.h/.cpp` | Individual physics body with transform, velocity, forces, and collider |
| `KPhysicsWorld` | `PhysicsWorld.h/.cpp` | Manages all bodies, runs the simulation loop, handles collision detection and response |
| `KPhysicsSubsystem` | `PhysicsSubsystem.h` | `ISubsystem` adapter that wraps `KPhysicsWorld` for registration with `KSubsystemRegistry` |

The system uses a fixed-timestep update loop with configurable sub-stepping, AABB-based broad phase, and discrete collision detection for Sphere-Sphere, Sphere-Box, and Box-Box pairs.

---

## Rigid Body (`KRigidBody`)

`KRigidBody` represents a single physics object in the world. Each body is owned by a `KPhysicsWorld` via `std::unique_ptr` and identified by a `uint32_t BodyId`.

### Transform Properties

| Property | Getter | Setter | Default |
|----------|--------|--------|---------|
| Position | `GetPosition()` | `SetPosition()` | `(0, 0, 0)` |
| Rotation | `GetRotation()` | `SetRotation()` | `(0, 0, 0, 1)` (identity quaternion) |
| Velocity | `GetVelocity()` | `SetVelocity()` | `(0, 0, 0)` |
| Angular Velocity | `GetAngularVelocity()` | `SetAngularVelocity()` | `(0, 0, 0)` |

### Mass and Inertia

| Property | Getter | Setter | Notes |
|----------|--------|--------|-------|
| Mass | `GetMass()` | `SetMass()` | Default `1.0`. Setting `<= 0` makes body infinite mass |
| Inverse Mass | `GetInverseMass()` | — | Computed as `1.0 / Mass` (or `0.0` for infinite mass) |
| Inertia Tensor | — | — | `XMFLOAT3(1, 1, 1)` — stored per-component |
| Inverse Inertia Tensor | — | — | `XMFLOAT3(1, 1, 1)` — used by angular impulse integration |

The inertia tensor is not automatically recalculated from collider shape. Set it manually if needed for accurate rotational dynamics.

### Collider Configuration

Setting collider properties automatically updates `ColliderType`:

```cpp
body->SetSphereRadius(1.0f);           // Sets ColliderType = Sphere
body->SetBoxHalfExtents({1, 1, 1});    // Sets ColliderType = Box
body->SetCapsuleParams(0.5f, 1.0f);    // Sets ColliderType = Capsule
```

Directly set collider type with `SetColliderType(EColliderType)`.

### Body Type

Controlled via `SetBodyType(EPhysicsBodyType)`:

| Type | Behavior |
|------|----------|
| `EPhysicsBodyType::Static` | Zero inverse mass, unaffected by forces/gravity. Used for floors, walls. |
| `EPhysicsBodyType::Dynamic` | Full physics simulation — gravity, forces, collision response. Default. |
| `EPhysicsBodyType::Kinematic` | Moved via `SetPosition`/`SetVelocity` by user code; not simulated but affects dynamic bodies. |

### Forces and Impulses

```cpp
body->ApplyForce(XMFLOAT3(0, 100, 0));       // Accumulated, applied during Integrate
body->ApplyImpulse(XMFLOAT3(0, 10, 0));      // Instantaneous velocity change
body->ApplyTorque(XMFLOAT3(0, 0, 50));       // Accumulated angular force
body->ApplyAngularImpulse(XMFLOAT3(0, 5, 0)); // Instantaneous angular velocity change
```

Force accumulators are cleared each integration step. Impulses modify velocity immediately.

### Trigger and Gravity Flags

```cpp
body->SetIsTrigger(true);   // Body detects collisions but does not resolve them
body->SetUseGravity(false); // Excludes body from gravitational acceleration
```

### Other Methods

| Method | Description |
|--------|-------------|
| `GetAABB()` | Returns axis-aligned bounding box based on current collider type |
| `Integrate(DeltaTime, Gravity)` | Performs semi-implicit Euler integration (called by `KPhysicsWorld`) |
| `GetTransformMatrix()` | Returns `XMMATRIX` combining rotation quaternion and translation |

Integration applies linear/angular damping of `0.98` per step.

---

## Physics World (`KPhysicsWorld`)

`KPhysicsWorld` owns all rigid bodies and runs the physics simulation.

### Lifecycle

```cpp
KPhysicsWorld world;

FPhysicsSettings settings;
settings.Gravity = XMFLOAT3(0, -9.81f, 0);
world.Initialize(settings);

// ... create bodies, simulate ...

world.Shutdown();
```

### Body Management

| Method | Description |
|--------|-------------|
| `CreateRigidBody()` | Creates a new body, returns its `uint32_t` ID |
| `DestroyRigidBody(BodyId)` | Removes a body from the simulation |
| `GetRigidBody(BodyId)` | Returns raw pointer to body, or `nullptr` if not found |
| `GetBodyCount()` | Returns number of active bodies |

### Configuration

```cpp
world.SetGravity(XMFLOAT3(0, -20.0f, 0));  // Override gravity
XMFLOAT3 gravity = world.GetGravity();      // Query current gravity

world.SetCollisionCallback([](uint32_t bodyA, uint32_t bodyB, const FCollisionInfo& info) {
    LOG_INFO("Collision between " << bodyA << " and " << bodyB
             << " depth=" << info.PenetrationDepth);
});
```

The `CollisionCallback` is a `std::function<void(uint32_t, uint32_t, const FCollisionInfo&)>`. It fires during `NarrowPhase` for every detected collision, including trigger overlaps.

### Query Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `Raycast` | `(Origin, Direction, MaxDistance, OutHit) -> bool` | Casts a ray, returns closest hit against Sphere and Box colliders |
| `SphereCast` | `(Origin, Radius, Direction, MaxDistance, OutHit) -> bool` | Reserved — currently returns `false` |
| `RaycastAll` | `(Origin, Direction, MaxDistance) -> vector<Hit>` | Returns all hits along a ray, sorted by distance |

### Update Pipeline

`Update(float DeltaTime)` uses a fixed-timestep loop with sub-stepping:

```
AccumulatedTime += DeltaTime
while (AccumulatedTime >= FixedDeltaTime && SubSteps < MaxSubSteps):
    1. IntegrateBodies   — Semi-implicit Euler for all dynamic bodies
    2. BroadPhase        — AABB overlap test produces candidate pairs
    3. NarrowPhase       — Exact collision tests on candidate pairs
    4. ResolveCollisions — Impulse-based response + positional correction
    AccumulatedTime -= FixedDeltaTime
```

**BroadPhase** (`BroadPhase`): Builds AABBs for all non-static bodies, then brute-force O(n²) AABB intersection to produce `PotentialPairs`.

**NarrowPhase** (`NarrowPhase`): Dispatches to specific collision tests based on collider type pair. Fires the collision callback on each hit.

**ResolveCollisions** (`ResolveCollisions`): For each collision:
- Skips pairs where either body is a trigger
- `ResolveCollision`: Computes impulse magnitude using coefficient of restitution, applies velocity changes weighted by inverse mass (static bodies get `0` inverse mass)
- `PositionalCorrection`: Baumgarte stabilization to resolve penetration, using configurable `Slop` and `BaumgarteStabilization` parameters

---

## Collision Types

Defined in `EColliderType`:

| Value | Collider | Collision Support |
|-------|----------|-------------------|
| `None` | No collider | — |
| `Sphere` | Sphere defined by `SphereRadius` | Sphere-Sphere, Sphere-Box |
| `Box` | Axis-aligned box defined by `BoxHalfExtents` | Sphere-Box, Box-Box |
| `Capsule` | Capsule defined by `CapsuleRadius` + `CapsuleHalfHeight` | AABB generation only (no narrow phase) |
| `Plane` | Infinite plane | Enum only — not implemented |

### Supported Collision Pairs

| Pair | Narrow Phase Method | Notes |
|------|---------------------|-------|
| Sphere–Sphere | `SphereSphereCollision` | Distance-based, produces contact normal and penetration depth |
| Sphere–Box | `SphereBoxCollision` | Closest-point-on-AABB test |
| Box–Box | `BoxBoxCollision` | AABB overlap test with minimum penetration axis selection |

Box-Box collision is axis-aligned (does not use oriented bounding boxes at the narrow phase level). The `FBox` struct stores a rotation quaternion, but collision detection treats boxes as axis-aligned.

---

## Physics Types Reference

All types defined in `Engine/Physics/PhysicsTypes.h`.

### `EPhysicsBodyType`

```cpp
enum class EPhysicsBodyType : uint8_t
{
    Static,    // Immovable, zero inverse mass
    Dynamic,   // Fully simulated
    Kinematic  // User-controlled, affects dynamics
};
```

### `FPhysicsMaterial`

```cpp
struct FPhysicsMaterial
{
    float Friction = 0.5f;       // Not yet used in collision response
    float Restitution = 0.3f;    // Coefficient of restitution (bounciness)
    float Density = 1.0f;        // Not yet used for auto-mass calculation
};
```

Restitution is used in impulse resolution: `E = min(A.Restitution, B.Restitution)`.

### `FPhysicsSettings`

```cpp
struct FPhysicsSettings
{
    XMFLOAT3 Gravity = XMFLOAT3(0.0f, -9.81f, 0.0f);
    float FixedDeltaTime = 1.0f / 60.0f;   // 60 Hz physics
    uint32_t MaxSubSteps = 4;                // Cap per-frame sub-steps
    float BaumgarteStabilization = 0.2f;     // Penetration correction percentage
    float Slop = 0.01f;                      // Allowed penetration before correction
};
```

### `FCollisionInfo`

```cpp
struct FCollisionInfo
{
    bool bCollided = false;
    XMFLOAT3 ContactPoint;         // World-space contact location
    XMFLOAT3 ContactNormal;        // Normal pointing from A to B
    float PenetrationDepth = 0.0f; // Overlap distance
    uint32_t BodyA = 0;           // First body ID
    uint32_t BodyB = 0;           // Second body ID
};
```

### `FPhysicsRaycastHit`

```cpp
struct FPhysicsRaycastHit
{
    bool bHit = false;
    XMFLOAT3 Point;       // World-space hit location
    XMFLOAT3 Normal;      // Surface normal at hit point
    float Distance = 0.0f; // Distance along ray
    uint32_t BodyId = 0;  // Hit body ID
};
```

### Shape Structs

#### `FAABB` — Axis-Aligned Bounding Box

```cpp
struct FAABB
{
    XMFLOAT3 Min, Max;

    XMFLOAT3 GetCenter() const;    // (Min + Max) * 0.5
    XMFLOAT3 GetExtents() const;   // (Max - Min) * 0.5
    bool Intersects(const FAABB&) const;  // AABB overlap test
};
```

#### `FSphere` — Bounding Sphere

```cpp
struct FSphere
{
    XMFLOAT3 Center = {0, 0, 0};
    float Radius = 0.5f;

    FAABB ToAABB() const;
};
```

#### `FBox` — Oriented Box

```cpp
struct FBox
{
    XMFLOAT3 Center = {0, 0, 0};
    XMFLOAT3 HalfExtents = {0.5, 0.5, 0.5};
    XMFLOAT4 Rotation = {0, 0, 0, 1};  // Quaternion (not used in collision)

    FAABB ToAABB() const;  // Conservative AABB using max half-extent
};
```

#### `FCapsule` — Capsule Shape

```cpp
struct FCapsule
{
    XMFLOAT3 Center = {0, 0, 0};
    float Radius = 0.5f;
    float HalfHeight = 1.0f;

    FAABB ToAABB() const;
};
```

---

## Physics Subsystem (`KPhysicsSubsystem`)

`KPhysicsSubsystem` adapts `KPhysicsWorld` to the `ISubsystem` interface, allowing it to be registered with `KSubsystemRegistry` for uniform lifecycle management.

```cpp
// Engine/Core/Subsystem.h
class ISubsystem
{
    virtual HRESULT Initialize() = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual const std::string& GetName() const = 0;
};
```

### Registration and Usage

```cpp
auto physicsSubsystem = std::make_shared<KPhysicsSubsystem>();

FPhysicsSettings settings;
settings.Gravity = XMFLOAT3(0, -9.81f, 0);
settings.MaxSubSteps = 8;
physicsSubsystem->SetSettings(settings);  // Must be called before Initialize

registry.Register<KPhysicsSubsystem>(physicsSubsystem);
registry.InitializeAll();

// Access the underlying world
KPhysicsWorld* world = physicsSubsystem->GetPhysicsWorld();
uint32_t bodyId = world->CreateRigidBody();

// Tick is automatic via KSubsystemRegistry::TickAll(DeltaTime)
// physicsSubsystem->Tick(DeltaTime) calls world->Update(DeltaTime)

registry.ShutdownAll();
```

### Key Points

- `GetName()` returns `"PhysicsSubsystem"`
- Settings must be configured via `SetSettings()` **before** `Initialize()` is called
- `GetPhysicsWorld()` provides direct access to the underlying `KPhysicsWorld` for body creation, raycasting, etc.
- The subsystem transitions through `ESubsystemState`: `Uninitialized` → `Initialized` → `Running` (during tick) → `Shutdown`

---

## Code Examples

### Creating and Simulating a Falling Sphere

```cpp
#include "Physics/PhysicsWorld.h"
#include "Physics/RigidBody.h"

KPhysicsWorld world;
world.Initialize();

uint32_t floorId = world.CreateRigidBody();
KRigidBody* floor = world.GetRigidBody(floorId);
floor->SetBodyType(EPhysicsBodyType::Static);
floor->SetPosition(XMFLOAT3(0, 0, 0));
floor->SetBoxHalfExtents(XMFLOAT3(10, 0.5f, 10));
floor->SetMaterial({0.8f, 0.2f, 1.0f});

uint32_t sphereId = world.CreateRigidBody();
KRigidBody* sphere = world.GetRigidBody(sphereId);
sphere->SetSphereRadius(0.5f);
sphere->SetPosition(XMFLOAT3(0, 10, 0));
sphere->SetMass(2.0f);

world.SetCollisionCallback([](uint32_t a, uint32_t b, const FCollisionInfo& info) {
    LOG_INFO("Hit! Bodies: " << a << ", " << b
             << " Penetration: " << info.PenetrationDepth);
});

float dt = 1.0f / 60.0f;
for (int i = 0; i < 300; ++i)
{
    world.Update(dt);
    XMFLOAT3 pos = sphere->GetPosition();
    LOG_INFO("Step " << i << " Y=" << pos.y);
}

world.Shutdown();
```

### Raycasting

```cpp
FPhysicsRaycastHit hit;
XMFLOAT3 origin = XMFLOAT3(0, 10, 0);
XMFLOAT3 direction = XMFLOAT3(0, -1, 0);

if (world.Raycast(origin, direction, 100.0f, hit))
{
    LOG_INFO("Hit body " << hit.BodyId
             << " at (" << hit.Point.x << ", " << hit.Point.y << ", " << hit.Point.z << ")"
             << " distance=" << hit.Distance);
}

auto allHits = world.RaycastAll(origin, direction, 100.0f);
for (const auto& h : allHits)
{
    LOG_INFO("Body " << h.BodyId << " at distance " << h.Distance);
}
```

### Trigger Volume Detection

```cpp
uint32_t triggerId = world.CreateRigidBody();
KRigidBody* trigger = world.GetRigidBody(triggerId);
trigger->SetBodyType(EPhysicsBodyType::Static);
trigger->SetSphereRadius(3.0f);
trigger->SetPosition(XMFLOAT3(0, 5, 0));
trigger->SetIsTrigger(true);

world.SetCollisionCallback([](uint32_t bodyA, uint32_t bodyB, const FCollisionInfo& info) {
    KRigidBody* a = /* obtain bodyA from your game code */;
    KRigidBody* b = /* obtain bodyB from your game code */;
    if (a && a->GetIsTrigger())
    {
        LOG_INFO("Object entered trigger zone!");
    }
});
```

Note: Triggers are detected in `NarrowPhase` via the collision callback, but are skipped during `ResolveCollisions`, so they produce no physical response.
