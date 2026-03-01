#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Math.h"

enum class EFrustumPlane
{
    Left = 0,
    Right,
    Top,
    Bottom,
    Near,
    Far
};

struct FFrustumPlane
{
    XMFLOAT3 Normal;
    float D;

    FFrustumPlane() : Normal(0, 0, 0), D(0) {}
    FFrustumPlane(const XMFLOAT3& InNormal, float InD) : Normal(InNormal), D(InD) {}

    float DistanceToPoint(const XMFLOAT3& Point) const
    {
        return Normal.x * Point.x + Normal.y * Point.y + Normal.z * Point.z + D;
    }

    void Normalize()
    {
        XMVECTOR n = XMLoadFloat3(&Normal);
        float length = XMVectorGetX(XMVector3Length(n));
        if (length > 0.0001f)
        {
            Normal.x /= length;
            Normal.y /= length;
            Normal.z /= length;
            D /= length;
        }
    }
};

class KFrustum
{
public:
    KFrustum() = default;
    ~KFrustum() = default;

    void UpdateFromMatrix(const XMMATRIX& ViewProjection);

    bool ContainsPoint(const XMFLOAT3& Point) const;
    bool ContainsSphere(const FBoundingSphere& Sphere) const;
    bool ContainsSphere(const XMFLOAT3& Center, float Radius) const;
    bool ContainsBox(const FBoundingBox& Box) const;
    bool IntersectsBox(const FBoundingBox& Box) const;

    const FFrustumPlane& GetPlane(EFrustumPlane Plane) const { return Planes[static_cast<int>(Plane)]; }
    const XMFLOAT3& GetCorner(int Index) const { return Corners[Index]; }

    void GetCorners(XMFLOAT3 OutCorners[8]) const;

private:
    void ExtractPlanes(const XMMATRIX& ViewProjection);
    void ComputeCorners();

private:
    FFrustumPlane Planes[6];
    XMFLOAT3 Corners[8];
    XMMATRIX LastViewProjection;
};
