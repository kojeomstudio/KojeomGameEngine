#pragma once

#include "../Utils/Common.h"

constexpr UINT32 MAX_POINT_LIGHTS = 8;
constexpr UINT32 MAX_SPOT_LIGHTS = 4;

struct FDirectionalLight
{
    XMFLOAT3 Direction   = {  0.5f, -1.0f,  0.5f };
    float    Padding0    = 0.0f;
    XMFLOAT4 Color       = {  1.0f,  0.95f, 0.9f, 1.0f };
    XMFLOAT4 AmbientColor= {  0.15f, 0.15f, 0.2f, 1.0f };

    FDirectionalLight() = default;
    FDirectionalLight(const XMFLOAT3& InDir, const XMFLOAT4& InColor, const XMFLOAT4& InAmbient)
        : Direction(InDir), Color(InColor), AmbientColor(InAmbient) {}
};

struct FPointLight
{
    XMFLOAT3 Position    = { 0.0f, 0.0f, 0.0f };
    float    Intensity   = 1.0f;
    XMFLOAT4 Color       = { 1.0f, 1.0f, 1.0f, 1.0f };
    float    Radius      = 10.0f;
    float    Falloff     = 1.0f;
    float    Padding0    = 0.0f;
    float    Padding1    = 0.0f;

    FPointLight() = default;
    FPointLight(const XMFLOAT3& InPos, const XMFLOAT4& InColor, float InRadius, float InIntensity = 1.0f)
        : Position(InPos), Color(InColor), Radius(InRadius), Intensity(InIntensity), Falloff(1.0f) {}
};

struct FSpotLight
{
    XMFLOAT3 Position    = { 0.0f, 0.0f, 0.0f };
    float    Intensity   = 1.0f;
    XMFLOAT3 Direction   = { 0.0f, -1.0f, 0.0f };
    float    InnerCone   = XM_PIDIV4;
    XMFLOAT4 Color       = { 1.0f, 1.0f, 1.0f, 1.0f };
    float    OuterCone   = XM_PIDIV2;
    float    Radius      = 20.0f;
    float    Falloff     = 1.0f;
    float    Padding0    = 0.0f;

    FSpotLight() = default;
    FSpotLight(const XMFLOAT3& InPos, const XMFLOAT3& InDir, const XMFLOAT4& InColor, 
               float InInnerCone, float InOuterCone, float InRadius, float InIntensity = 1.0f)
        : Position(InPos), Direction(InDir), Color(InColor), 
          InnerCone(InInnerCone), OuterCone(InOuterCone), Radius(InRadius), 
          Intensity(InIntensity), Falloff(1.0f) {}
};

struct FPointLightData
{
    XMFLOAT3 Position;
    float    Intensity;
    XMFLOAT4 Color;
    float    Radius;
    float    Falloff;
    float    Padding0;
    float    Padding1;
};
static_assert(sizeof(FPointLightData) == 48, "FPointLightData size mismatch");

struct FSpotLightData
{
    XMFLOAT3 Position;
    float    Intensity;
    XMFLOAT3 Direction;
    float    InnerCone;
    XMFLOAT4 Color;
    float    OuterCone;
    float    Radius;
    float    Falloff;
    float    Padding0;
};
static_assert(sizeof(FSpotLightData) == 64, "FSpotLightData size mismatch");

struct FLightBuffer
{
    XMFLOAT3 LightDirection;
    float    Padding0;
    XMFLOAT4 LightColor;
    XMFLOAT4 AmbientColor;
    XMFLOAT3 CameraPosition;
    float    Padding1;
};
static_assert(sizeof(FLightBuffer) % 16 == 0, "FLightBuffer must be 16-byte aligned");

struct FMultipleLightBuffer
{
    XMFLOAT3 CameraPosition;
    int32    NumPointLights;
    XMFLOAT3 DirLightDirection;
    int32    NumSpotLights;
    XMFLOAT4 DirLightColor;
    XMFLOAT4 AmbientColor;
    FPointLightData PointLights[MAX_POINT_LIGHTS];
    FSpotLightData SpotLights[MAX_SPOT_LIGHTS];
};
static_assert(sizeof(FMultipleLightBuffer) % 16 == 0, "FMultipleLightBuffer must be 16-byte aligned");
static_assert(sizeof(FMultipleLightBuffer) == 704, "FMultipleLightBuffer size mismatch");

struct FShadowDataBuffer
{
    XMMATRIX LightViewProjection;
    XMFLOAT2 ShadowMapSize;
    float    DepthBias;
    int32    PCFKernelSize;
    int32    bShadowEnabled;
    XMFLOAT3 Padding;
};
static_assert(sizeof(FShadowDataBuffer) % 16 == 0, "FShadowDataBuffer must be 16-byte aligned");
