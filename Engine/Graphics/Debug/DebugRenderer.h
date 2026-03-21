#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Utils/Math.h"
#include "../GraphicsDevice.h"
#include "../Camera.h"
#include <vector>
#include <deque>

struct FDebugVertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;
};

struct FDebugLine
{
    XMFLOAT3 Start;
    XMFLOAT3 End;
    XMFLOAT4 Color;
    float LifeTime;
    float RemainingLifeTime;
    bool bPersistent;
};

struct FDebugSphere
{
    XMFLOAT3 Center;
    float Radius;
    XMFLOAT4 Color;
    float LifeTime;
    float RemainingLifeTime;
    int32 Segments;
};

struct FDebugBox
{
    XMFLOAT3 Center;
    XMFLOAT3 Extent;
    XMFLOAT4 Color;
    XMMATRIX Rotation;
    float LifeTime;
    float RemainingLifeTime;
};

struct FDebugCapsule
{
    XMFLOAT3 Center;
    float Radius;
    float HalfHeight;
    XMFLOAT4 Color;
    float LifeTime;
    float RemainingLifeTime;
};

struct FDebugArrow
{
    XMFLOAT3 Start;
    XMFLOAT3 End;
    XMFLOAT4 Color;
    float LifeTime;
    float RemainingLifeTime;
    float HeadSize;
};

struct FDebugGrid
{
    XMFLOAT3 Center;
    float GridSize;
    float CellSize;
    XMFLOAT4 Color;
    int32 Divisions;
};

class KDebugRenderer
{
public:
    KDebugRenderer() = default;
    ~KDebugRenderer() = default;

    KDebugRenderer(const KDebugRenderer&) = delete;
    KDebugRenderer& operator=(const KDebugRenderer&) = delete;

    HRESULT Initialize(KGraphicsDevice* InGraphicsDevice);

    void BeginFrame();
    void Render(ID3D11DeviceContext* Context, KCamera* Camera);
    void EndFrame(float DeltaTime);

    void DrawLine(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color, 
                  float LifeTime = 0.0f, bool bPersistent = false);
    void DrawRay(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float Length, 
                 const XMFLOAT4& Color, float LifeTime = 0.0f);
    void DrawArrow(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color,
                   float HeadSize = 0.3f, float LifeTime = 0.0f);
    
    void DrawSphere(const XMFLOAT3& Center, float Radius, const XMFLOAT4& Color,
                    int32 Segments = 16, float LifeTime = 0.0f);
    void DrawBox(const XMFLOAT3& Center, const XMFLOAT3& Extent, const XMFLOAT4& Color,
                 float LifeTime = 0.0f);
    void DrawBox(const XMFLOAT3& Center, const XMFLOAT3& Extent, const XMMATRIX& Rotation,
                 const XMFLOAT4& Color, float LifeTime = 0.0f);
    void DrawBox(const FBoundingBox& Box, const XMFLOAT4& Color, float LifeTime = 0.0f);
    
    void DrawCapsule(const XMFLOAT3& Center, float Radius, float HalfHeight, 
                     const XMFLOAT4& Color, float LifeTime = 0.0f);
    
    void DrawFrustum(const XMMATRIX& ViewProjection, const XMFLOAT4& Color, float LifeTime = 0.0f);
    void DrawFrustum(const XMFLOAT3* Corners, const XMFLOAT4& Color, float LifeTime = 0.0f);
    
    void DrawGrid(const XMFLOAT3& Center, float GridSize, float CellSize, 
                  const XMFLOAT4& Color, int32 Divisions = 10);
    void DrawAxis(const XMFLOAT3& Origin, float AxisLength = 1.0f, float LifeTime = 0.0f);
    
    void DrawCoordinateSystem(const XMMATRIX& Transform, float Scale = 1.0f, float LifeTime = 0.0f);
    
    void DrawCone(const XMFLOAT3& Apex, const XMFLOAT3& Direction, float Height, 
                  float Radius, const XMFLOAT4& Color, int32 Segments = 16, float LifeTime = 0.0f);
    
    void DrawCylinder(const XMFLOAT3& Start, const XMFLOAT3& End, float Radius,
                      const XMFLOAT4& Color, int32 Segments = 16, float LifeTime = 0.0f);

    void ClearPersistent();
    void ClearAll();

    bool IsInitialized() const { return bInitialized; }
    void SetEnabled(bool bEnabled) { bEnabled = bEnabled; }
    bool IsEnabled() const { return bEnabled; }

private:
    HRESULT CreateShaders();
    HRESULT CreateBuffers();
    void UpdateDynamicBuffer(ID3D11DeviceContext* Context);
    
    void AddLineVertices(const XMFLOAT3& Start, const XMFLOAT3& End, const XMFLOAT4& Color);
    void GenerateSphereVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                                float Radius, const XMFLOAT4& Color, int32 Segments);
    void GenerateBoxVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                             const XMFLOAT3& Extent, const XMMATRIX& Rotation, const XMFLOAT4& Color);
    void GenerateCapsuleVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Center,
                                  float Radius, float HalfHeight, const XMFLOAT4& Color);
    void GenerateConeVertices(std::vector<FDebugVertex>& Vertices, const XMFLOAT3& Apex,
                               const XMFLOAT3& Direction, float Height, float Radius, 
                               const XMFLOAT4& Color, int32 Segments);

private:
    KGraphicsDevice* GraphicsDevice = nullptr;
    bool bInitialized = false;
    bool bEnabled = true;

    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> ConstantBuffer;
    ComPtr<ID3D11DepthStencilState> DepthStencilState;
    ComPtr<ID3D11DepthStencilState> DepthDisabledState;
    ComPtr<ID3D11BlendState> BlendState;

    static const UINT32 MaxVertices = 65536;
    std::vector<FDebugVertex> FrameVertices;
    
    std::deque<FDebugLine> Lines;
    std::deque<FDebugSphere> Spheres;
    std::deque<FDebugBox> Boxes;
    std::deque<FDebugCapsule> Capsules;
    std::deque<FDebugArrow> Arrows;
};
