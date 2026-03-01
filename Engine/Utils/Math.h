#pragma once

#include "../Utils/Common.h"

struct FBoundingBox
{
    XMFLOAT3 Min;
    XMFLOAT3 Max;

    FBoundingBox()
        : Min(FLT_MAX, FLT_MAX, FLT_MAX)
        , Max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    {}

    FBoundingBox(const XMFLOAT3& InMin, const XMFLOAT3& InMax)
        : Min(InMin), Max(InMax) {}

    void Expand(const XMFLOAT3& Point)
    {
        Min.x = std::min(Min.x, Point.x);
        Min.y = std::min(Min.y, Point.y);
        Min.z = std::min(Min.z, Point.z);
        Max.x = std::max(Max.x, Point.x);
        Max.y = std::max(Max.y, Point.y);
        Max.z = std::max(Max.z, Point.z);
    }

    void Expand(const FBoundingBox& Other)
    {
        Expand(Other.Min);
        Expand(Other.Max);
    }

    XMFLOAT3 GetCenter() const
    {
        return XMFLOAT3(
            (Min.x + Max.x) * 0.5f,
            (Min.y + Max.y) * 0.5f,
            (Min.z + Max.z) * 0.5f
        );
    }

    XMFLOAT3 GetExtent() const
    {
        return XMFLOAT3(
            (Max.x - Min.x) * 0.5f,
            (Max.y - Min.y) * 0.5f,
            (Max.z - Min.z) * 0.5f
        );
    }

    float GetRadius() const
    {
        XMFLOAT3 extent = GetExtent();
        XMVECTOR extentVec = XMLoadFloat3(&extent);
        return XMVectorGetX(XMVector3Length(extentVec));
    }

    bool IsValid() const
    {
        return Min.x <= Max.x && Min.y <= Max.y && Min.z <= Max.z;
    }

    void Reset()
    {
        Min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
        Max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }
};

struct FBoundingSphere
{
    XMFLOAT3 Center;
    float Radius;

    FBoundingSphere() : Center(0, 0, 0), Radius(0) {}
    FBoundingSphere(const XMFLOAT3& InCenter, float InRadius)
        : Center(InCenter), Radius(InRadius) {}

    void FromBoundingBox(const FBoundingBox& Box)
    {
        Center = Box.GetCenter();
        Radius = Box.GetRadius();
    }
};

struct FTransform
{
    XMFLOAT3 Position;
    XMFLOAT4 Rotation;
    XMFLOAT3 Scale;

    FTransform()
        : Position(0, 0, 0)
        , Rotation(0, 0, 0, 1)
        , Scale(1, 1, 1)
    {}

    FTransform(const XMFLOAT3& InPosition, const XMFLOAT4& InRotation = XMFLOAT4(0, 0, 0, 1),
               const XMFLOAT3& InScale = XMFLOAT3(1, 1, 1))
        : Position(InPosition), Rotation(InRotation), Scale(InScale) {}

    XMMATRIX ToMatrix() const
    {
        XMVECTOR pos = XMLoadFloat3(&Position);
        XMVECTOR rot = XMLoadFloat4(&Rotation);
        XMVECTOR scale = XMLoadFloat3(&Scale);
        return XMMatrixScalingFromVector(scale) * 
               XMMatrixRotationQuaternion(rot) * 
               XMMatrixTranslationFromVector(pos);
    }

    static FTransform Identity()
    {
        return FTransform();
    }

    void Reset()
    {
        Position = XMFLOAT3(0, 0, 0);
        Rotation = XMFLOAT4(0, 0, 0, 1);
        Scale = XMFLOAT3(1, 1, 1);
    }
};

struct FMaterialSlot
{
    std::string Name;
    uint32 StartIndex;
    uint32 IndexCount;
    int32 MaterialIndex;

    FMaterialSlot() : StartIndex(0), IndexCount(0), MaterialIndex(-1) {}
    FMaterialSlot(const std::string& InName, uint32 InStartIndex, uint32 InIndexCount, int32 InMaterialIndex = -1)
        : Name(InName), StartIndex(InStartIndex), IndexCount(InIndexCount), MaterialIndex(InMaterialIndex) {}
};
