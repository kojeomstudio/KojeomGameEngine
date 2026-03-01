#pragma once

#include "../../Utils/Common.h"
#include "../../Utils/Logger.h"
#include "../../Utils/Math.h"
#include "../Mesh.h"
#include "../Shader.h"
#include <vector>

struct FInstanceData
{
    XMMATRIX WorldMatrix;

    FInstanceData() : WorldMatrix(XMMatrixIdentity()) {}
    explicit FInstanceData(const XMMATRIX& InWorld) : WorldMatrix(InWorld) {}
};

class KInstancedRenderer
{
public:
    KInstancedRenderer() = default;
    ~KInstancedRenderer() = default;

    KInstancedRenderer(const KInstancedRenderer&) = delete;
    KInstancedRenderer& operator=(const KInstancedRenderer&) = delete;

    HRESULT Initialize(ID3D11Device* Device, UINT32 MaxInstances = 1024);
    void Cleanup();

    void BeginFrame();
    void AddInstance(const XMMATRIX& WorldMatrix);
    void AddInstances(const std::vector<XMMATRIX>& WorldMatrices);
    void ClearInstances();

    void Render(ID3D11DeviceContext* Context, 
                KMesh* Mesh, 
                KShaderProgram* Shader,
                const XMMATRIX& ViewMatrix,
                const XMMATRIX& ProjectionMatrix);

    void RenderWithTexture(ID3D11DeviceContext* Context,
                          KMesh* Mesh,
                          KShaderProgram* Shader,
                          class KTexture* Texture,
                          const XMMATRIX& ViewMatrix,
                          const XMMATRIX& ProjectionMatrix);

    UINT32 GetInstanceCount() const { return static_cast<UINT32>(Instances.size()); }
    UINT32 GetMaxInstances() const { return MaxInstances; }
    bool IsInitialized() const { return bInitialized; }

    HRESULT SetMaxInstances(ID3D11Device* Device, UINT32 NewMaxInstances);

private:
    HRESULT CreateInstanceBuffer(ID3D11Device* Device);
    HRESULT UpdateInstanceBuffer(ID3D11DeviceContext* Context);
    void SetupInstancedInputLayout(ID3D11DeviceContext* Context, KMesh* Mesh);

private:
    ComPtr<ID3D11Buffer> InstanceBuffer;
    std::vector<FInstanceData> Instances;
    UINT32 MaxInstances = 1024;
    bool bInitialized = false;
    bool bInstanceBufferDirty = true;
};
