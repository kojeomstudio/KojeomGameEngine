#include "Frustum.h"

void KFrustum::UpdateFromMatrix(const XMMATRIX& ViewProjection)
{
    LastViewProjection = ViewProjection;
    ExtractPlanes(ViewProjection);
    ComputeCorners();
}

void KFrustum::ExtractPlanes(const XMMATRIX& Matrix)
{
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, XMMatrixTranspose(Matrix));

    Planes[static_cast<int>(EFrustumPlane::Left)].Normal = XMFLOAT3(
        m._14 + m._11,
        m._24 + m._21,
        m._34 + m._31
    );
    Planes[static_cast<int>(EFrustumPlane::Left)].D = m._44 + m._41;

    Planes[static_cast<int>(EFrustumPlane::Right)].Normal = XMFLOAT3(
        m._14 - m._11,
        m._24 - m._21,
        m._34 - m._31
    );
    Planes[static_cast<int>(EFrustumPlane::Right)].D = m._44 - m._41;

    Planes[static_cast<int>(EFrustumPlane::Top)].Normal = XMFLOAT3(
        m._14 - m._12,
        m._24 - m._22,
        m._34 - m._32
    );
    Planes[static_cast<int>(EFrustumPlane::Top)].D = m._44 - m._42;

    Planes[static_cast<int>(EFrustumPlane::Bottom)].Normal = XMFLOAT3(
        m._14 + m._12,
        m._24 + m._22,
        m._34 + m._32
    );
    Planes[static_cast<int>(EFrustumPlane::Bottom)].D = m._44 + m._42;

    Planes[static_cast<int>(EFrustumPlane::Near)].Normal = XMFLOAT3(
        m._13,
        m._23,
        m._33
    );
    Planes[static_cast<int>(EFrustumPlane::Near)].D = m._43;

    Planes[static_cast<int>(EFrustumPlane::Far)].Normal = XMFLOAT3(
        m._14 - m._13,
        m._24 - m._23,
        m._34 - m._33
    );
    Planes[static_cast<int>(EFrustumPlane::Far)].D = m._44 - m._43;

    for (int i = 0; i < 6; ++i)
    {
        Planes[i].Normalize();
    }
}

void KFrustum::ComputeCorners()
{
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, LastViewProjection);

    static const XMFLOAT3 ndcCorners[8] = {
        { -1.0f,  1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        { -1.0f,  1.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f },
        {  1.0f, -1.0f, 1.0f }
    };

    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR corner = XMLoadFloat3(&ndcCorners[i]);
        corner = XMVector3TransformCoord(corner, invViewProj);
        XMStoreFloat3(&Corners[i], corner);
    }
}

bool KFrustum::ContainsPoint(const XMFLOAT3& Point) const
{
    for (int i = 0; i < 6; ++i)
    {
        if (Planes[i].DistanceToPoint(Point) < 0.0f)
        {
            return false;
        }
    }
    return true;
}

bool KFrustum::ContainsSphere(const FBoundingSphere& Sphere) const
{
    return ContainsSphere(Sphere.Center, Sphere.Radius);
}

bool KFrustum::ContainsSphere(const XMFLOAT3& Center, float Radius) const
{
    for (int i = 0; i < 6; ++i)
    {
        float distance = Planes[i].DistanceToPoint(Center);
        if (distance < -Radius)
        {
            return false;
        }
    }
    return true;
}

bool KFrustum::ContainsBox(const FBoundingBox& Box) const
{
    if (!Box.IsValid())
    {
        return false;
    }

    XMFLOAT3 corners[8];
    corners[0] = XMFLOAT3(Box.Min.x, Box.Min.y, Box.Min.z);
    corners[1] = XMFLOAT3(Box.Max.x, Box.Min.y, Box.Min.z);
    corners[2] = XMFLOAT3(Box.Min.x, Box.Max.y, Box.Min.z);
    corners[3] = XMFLOAT3(Box.Max.x, Box.Max.y, Box.Min.z);
    corners[4] = XMFLOAT3(Box.Min.x, Box.Min.y, Box.Max.z);
    corners[5] = XMFLOAT3(Box.Max.x, Box.Min.y, Box.Max.z);
    corners[6] = XMFLOAT3(Box.Min.x, Box.Max.y, Box.Max.z);
    corners[7] = XMFLOAT3(Box.Max.x, Box.Max.y, Box.Max.z);

    for (int i = 0; i < 6; ++i)
    {
        int out = 0;
        for (int j = 0; j < 8; ++j)
        {
            if (Planes[i].DistanceToPoint(corners[j]) < 0.0f)
            {
                out++;
            }
        }
        if (out == 8)
        {
            return false;
        }
    }

    return true;
}

bool KFrustum::IntersectsBox(const FBoundingBox& Box) const
{
    return ContainsBox(Box);
}

void KFrustum::GetCorners(XMFLOAT3 OutCorners[8]) const
{
    for (int i = 0; i < 8; ++i)
    {
        OutCorners[i] = Corners[i];
    }
}
