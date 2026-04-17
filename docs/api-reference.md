# API 레퍼런스

## Core API

### KEngine (`Engine/Core/Engine.h`)

엔진 메인 클래스. 윈도우 관리, 그래픽스 초기화, 메인 루프를 담당합니다.

```cpp
class KEngine
{
public:
    HRESULT Initialize(HINSTANCE InstanceHandle, 
                      const std::wstring& WindowTitle = L"KojeomEngine",
                      UINT32 Width = 1024, UINT32 Height = 768);
    HRESULT InitializeWithExternalHwnd(HWND InExternalHwnd, UINT32 InWidth, UINT32 InHeight);
    int32 Run();
    void Shutdown();
    virtual void Update(float DeltaTime);
    virtual void Render();
    void OnResize(UINT32 NewWidth, UINT32 NewHeight);

    KGraphicsDevice* GetGraphicsDevice() const;
    KCamera* GetCamera() const;
    KRenderer* GetRenderer() const;
    KInputManager* GetInputManager() const;
    KSceneManager& GetSceneManager();
    KAudioSubsystem* GetAudioSubsystem() const;
    KPhysicsSubsystem* GetPhysicsSubsystem() const;
    HWND GetWindowHandle() const;
    KSubsystemRegistry& GetSubsystemRegistry();

    template<typename T> T* GetSubsystem();

    static KEngine* GetInstance();

    template<typename T>
    static int32 RunApplication(HINSTANCE InstanceHandle, 
                             const std::wstring& WindowTitle,
                             UINT32 Width = 1024, UINT32 Height = 768,
                             std::function<HRESULT(T*)> CustomInit = nullptr);
};
```

### ISubsystem / KSubsystemRegistry (`Engine/Core/Subsystem.h`)

```cpp
class ISubsystem
{
public:
    virtual HRESULT Initialize() = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual const std::string& GetName() const = 0;
    ESubsystemState GetState() const;
    bool IsInitialized() const;
};

class KSubsystemRegistry
{
public:
    template<typename T> void Register(std::shared_ptr<T> Subsystem);
    template<typename T> T* Get();
    HRESULT InitializeAll();
    void TickAll(float DeltaTime);
    void ShutdownAll();
    size_t GetCount() const;
};
```

## Graphics API

### KGraphicsDevice (`Engine/Graphics/GraphicsDevice.h`)

```cpp
class KGraphicsDevice
{
public:
    HRESULT Initialize(HWND WindowHandle, UINT32 Width, UINT32 Height, bool bEnableDebugLayer = false);
    void Cleanup();
    void BeginFrame(const float ClearColor[4] = Colors::CornflowerBlue);
    void EndFrame(bool bVSync = true);
    HRESULT ResizeBuffers(UINT32 NewWidth, UINT32 NewHeight);

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetContext() const;
    IDXGISwapChain* GetSwapChain() const;
    ID3D11RenderTargetView* GetRenderTargetView() const;
    ID3D11DepthStencilView* GetDepthStencilView() const;
};
```

### KRenderer (`Engine/Graphics/Renderer.h`)

```cpp
class KRenderer
{
public:
    HRESULT Initialize(KGraphicsDevice* InGraphicsDevice);
    void BeginFrame(KCamera* InCamera, const float ClearColor[4]);
    void EndFrame(bool bVSync = true);
    void RenderObject(const FRenderObject& RenderObject);
    void RenderMesh(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                    std::shared_ptr<KTexture> InTexture = nullptr);
    void RenderMeshBasic(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix);
    void RenderMeshLit(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix,
                       std::shared_ptr<KTexture> InTexture = nullptr);
    void RenderMeshPBR(std::shared_ptr<KMesh> InMesh, const XMMATRIX& WorldMatrix, KMaterial* Material);
    void RenderSkeletalMesh(KSkeletalMesh* InMesh, const XMMATRIX& WorldMatrix, ID3D11Buffer* BoneMatrixBuffer);

    // 조명
    void SetDirectionalLight(const FDirectionalLight& Light);
    void AddPointLight(const FPointLight& Light);
    void AddSpotLight(const FSpotLight& Light);
    void ClearAllLights();

    // 렌더 패스 설정
    void SetRenderPath(ERenderPath Path);
    void SetShadowEnabled(bool bEnabled);
    void SetPostProcessEnabled(bool bEnabled);
    void BeginHDRPass();
    void EndHDRPass();

    // 효과 토글
    void SetSSAOEnabled(bool bEnabled);
    void SetSSREnabled(bool bEnabled);
    void SetTAAEnabled(bool bEnabled);
    void SetVolumetricFogEnabled(bool bEnabled);
    void SetSSGIEnabled(bool bEnabled);
    void SetMotionBlurEnabled(bool bEnabled);
    void SetDepthOfFieldEnabled(bool bEnabled);
    void SetLensEffectsEnabled(bool bEnabled);
    void SetSkyEnabled(bool bEnabled);
    void SetDebugUIEnabled(bool bEnabled);

    // 프리미티브 생성
    std::shared_ptr<KMesh> CreateTriangleMesh();
    std::shared_ptr<KMesh> CreateQuadMesh();
    std::shared_ptr<KMesh> CreateCubeMesh();
    std::shared_ptr<KMesh> CreateSphereMesh(UINT32 Slices = 16, UINT32 Stacks = 16);

    // 서브시스템 접근
    KShadowRenderer* GetShadowRenderer();
    KDeferredRenderer* GetDeferredRenderer();
    KPostProcessor* GetPostProcessor();
    KSSAO* GetSSAO();
    KSSR* GetSSR();
    KTAA* GetTAA();
    KVolumetricFog* GetVolumetricFog();
    KSSGI* GetSSGI();
    KMotionBlur* GetMotionBlur();
    KDepthOfField* GetDepthOfField();
    KLensEffects* GetLensEffects();
    KSkySystem* GetSkySystem();
    KInstancedRenderer* GetInstancedRenderer();
    KGPUTimer* GetGPUTimer();
    KTextureManager* GetTextureManager();
    KojeomEngine::KDebugUI* GetDebugUI();

    int32 GetDrawCallCount() const;
    int32 GetVertexCount() const;
    float GetFrameTime() const;
    void Cleanup();
};
```

### KCamera (`Engine/Graphics/Camera.h`)

```cpp
class KCamera
{
public:
    void SetPosition(const XMFLOAT3& InPosition);
    void SetRotation(const XMFLOAT3& InRotation);
    void LookAt(const XMFLOAT3& Target, const XMFLOAT3& Up = XMFLOAT3(0, 1, 0));
    void SetPerspective(float FovY, float AspectRatio, float NearZ, float FarZ);
    void SetOrthographic(float Width, float Height, float NearZ, float FarZ);
    void UpdateMatrices();
    void Move(const XMFLOAT3& Offset);
    void Rotate(const XMFLOAT3& DeltaRotation);

    const XMMATRIX& GetViewMatrix() const;
    const XMMATRIX& GetProjectionMatrix() const;
    const XMFLOAT3& GetPosition() const;
    XMFLOAT3 GetForward() const;
    XMFLOAT3 GetRight() const;
    XMFLOAT3 GetUp() const;
    float GetFovY() const;
    float GetAspectRatio() const;
    ECameraProjectionType GetProjectionType() const;
};
```

### KShaderProgram (`Engine/Graphics/Shader.h`)

```cpp
class KShaderProgram
{
public:
    HRESULT CreateBasicColorShader(ID3D11Device* Device);
    HRESULT CreatePhongShader(ID3D11Device* Device);
    HRESULT CreatePhongShadowShader(ID3D11Device* Device);
    HRESULT CreatePBRShader(ID3D11Device* Device);
    HRESULT CreateSkinnedShader(ID3D11Device* Device);
    HRESULT CreateWaterShader(ID3D11Device* Device);
    HRESULT CreateDepthOnlyShader(ID3D11Device* Device);
    HRESULT CompileFromSource(ID3D11Device* Device, const std::string& VSSource,
                              const std::string& PSSource, ...);
    void AddShader(std::shared_ptr<KShader> Shader);
    HRESULT CreateInputLayout(ID3D11Device* Device, const D3D11_INPUT_ELEMENT_DESC*, UINT32);
    void Bind(ID3D11DeviceContext* Context) const;
    void Unbind(ID3D11DeviceContext* Context) const;
};
```

### KMesh (`Engine/Graphics/Mesh.h`)

```cpp
class KMesh
{
public:
    HRESULT Initialize(ID3D11Device* Device, const FVertex* Vertices, UINT32 VertexCount,
                      const UINT32* Indices = nullptr, UINT32 IndexCount = 0);
    void Render(ID3D11DeviceContext* Context);
    void UpdateConstantBuffer(ID3D11DeviceContext* Context,
                             const XMMATRIX& World, const XMMATRIX& View, const XMMATRIX& Projection);
    void Cleanup();
    UINT32 GetVertexCount() const;
    UINT32 GetIndexCount() const;

    static std::unique_ptr<KMesh> CreateTriangle(ID3D11Device* Device);
    static std::unique_ptr<KMesh> CreateQuad(ID3D11Device* Device);
    static std::unique_ptr<KMesh> CreateCube(ID3D11Device* Device);
    static std::unique_ptr<KMesh> CreateSphere(ID3D11Device* Device, UINT32 Slices = 16, UINT32 Stacks = 16);
};
```

### KMaterial (`Engine/Graphics/Material.h`)

```cpp
class KMaterial
{
public:
    HRESULT Initialize(KGraphicsDevice* InDevice);
    void SetAlbedoColor(const XMFLOAT4& Color);
    void SetMetallic(float Value);
    void SetRoughness(float Value);
    void SetAO(float Value);
    void SetEmissive(const XMFLOAT4& Color, float Intensity);
    void SetTexture(EMaterialTextureSlot Slot, std::shared_ptr<KTexture> Texture);
    void UpdateConstantBuffer(ID3D11DeviceContext* Context);
    void BindTextures(ID3D11DeviceContext* Context);

    // 프리셋
    static FPBRMaterialParams Metal();
    static FPBRMaterialParams Plastic();
    static FPBRMaterialParams Rubber();
    static FPBRMaterialParams Gold();
    static FPBRMaterialParams Silver();
    static FPBRMaterialParams Copper();
};
```

### KTexture / KTextureManager (`Engine/Graphics/Texture.h`)

```cpp
class KTexture
{
public:
    HRESULT LoadFromFile(ID3D11Device* Device, const std::wstring& Filename);
    HRESULT CreateSolidColor(ID3D11Device* Device, UINT32 Width, UINT32 Height, const XMFLOAT4& Color);
    HRESULT CreateCheckerboard(ID3D11Device* Device, UINT32 Width, UINT32 Height,
                               const XMFLOAT4& Color1, const XMFLOAT4& Color2, UINT32 CheckSize = 32);
    void Bind(ID3D11DeviceContext* Context, UINT32 Slot = 0) const;
    void Cleanup();
};

class KTextureManager
{
public:
    std::shared_ptr<KTexture> LoadTexture(ID3D11Device* Device, const std::wstring& Filename);
    HRESULT CreateDefaultTextures(ID3D11Device* Device);
    std::shared_ptr<KTexture> GetWhiteTexture() const;
    std::shared_ptr<KTexture> GetBlackTexture() const;
    std::shared_ptr<KTexture> GetCheckerboardTexture() const;
};
```

### 조명 구조체 (`Engine/Graphics/Light.h`)

```cpp
struct FDirectionalLight;  // 방향, 강도, 색상, 환경색
struct FPointLight;        // 위치, 강도, 색상, 반경, 감쇠 (최대 8개)
struct FSpotLight;         // 위치, 방향, 색상, 내/외부 원뿔, 반경 (최대 4개)
```

## Scene API

### KActor (`Engine/Scene/Actor.h`)

```cpp
class KActor
{
public:
    virtual void BeginPlay() {}
    virtual void Tick(float DeltaTime);
    virtual void Render(KRenderer* Renderer);

    void AddComponent(ComponentPtr Component);
    void RemoveComponent(KActorComponent* Component);
    template<typename T> T* GetComponent() const;
    template<typename T> std::vector<T*> GetComponents() const;

    void SetName(const std::string& InName);
    void SetWorldTransform(const FTransform& InTransform);
    void SetWorldPosition(const XMFLOAT3& Position);
    void SetWorldRotation(const XMFLOAT4& Rotation);
    void SetWorldScale(const XMFLOAT3& Scale);
    XMMATRIX GetWorldMatrix() const;

    void SetParent(KActor* InParent);
    void AddChild(ActorPtr Child);
    void SetVisible(bool bInVisible);

    void Serialize(KBinaryArchive& Archive);
    void Deserialize(KBinaryArchive& Archive);
};
```

### KScene / KSceneManager (`Engine/Scene/SceneManager.h`)

```cpp
class KScene
{
public:
    HRESULT Load(const std::wstring& Path);
    HRESULT Save(const std::wstring& Path);
    ActorPtr CreateActor(const std::string& Name);
    void AddActor(ActorPtr Actor);
    void RemoveActor(const std::string& Name);
    ActorPtr FindActor(const std::string& Name) const;
    const std::vector<ActorPtr>& GetActors() const;
};

class KSceneManager
{
public:
    HRESULT Initialize();
    std::shared_ptr<KScene> CreateScene(const std::string& Name);
    std::shared_ptr<KScene> LoadScene(const std::wstring& Path);
    HRESULT SaveScene(const std::wstring& Path, std::shared_ptr<KScene> Scene);
    void SetActiveScene(const std::string& Name);
    std::shared_ptr<KScene> GetActiveScene() const;
    std::shared_ptr<KScene> GetScene(const std::string& Name) const;
};
```

## Input API

### KInputManager (`Engine/Input/InputManager.h`)

```cpp
class KInputManager
{
public:
    HRESULT Initialize(HWND WindowHandle);
    void Shutdown();
    void Update();

    bool IsKeyDown(uint32_t KeyCode) const;
    bool IsKeyHeld(uint32_t KeyCode) const;
    bool IsKeyJustPressed(uint32_t KeyCode) const;
    bool IsKeyJustReleased(uint32_t KeyCode) const;

    bool IsMouseButtonDown(EMouseButton::Button Button) const;
    bool IsMouseButtonHeld(EMouseButton::Button Button) const;
    void GetMousePosition(int32_t& OutX, int32_t& OutY) const;
    void GetMouseDelta(int32_t& OutDeltaX, int32_t& OutDeltaY) const;
    int32_t GetMouseWheelDelta() const;

    void SetMouseVisible(bool bVisible);
    void SetMouseCapture(bool bCapture);
    void EnableRawInput(bool bEnable);

    void RegisterAction(const std::string& ActionName, uint32_t PrimaryKey, uint32_t ModifierKey = 0);
    bool IsActionDown(const std::string& ActionName) const;
    bool IsActionPressed(const std::string& ActionName) const;

    void RegisterInputCallback(uint32_t KeyCode, InputCallback Callback);

    static KInputManager* GetInstance();
};
```

## Audio API

### KAudioManager (`Engine/Audio/AudioManager.h`)

```cpp
class KAudioManager
{
public:
    HRESULT Initialize(const FAudioConfig& InConfig = FAudioConfig());
    void Shutdown();
    void Update(float DeltaTime);

    std::shared_ptr<KSound> LoadSound(const std::wstring& FilePath, const std::string& Name = "");
    uint32_t PlaySound(const std::string& Name, const FSoundParams& Params = FSoundParams());
    void StopSound(uint32_t VoiceId);
    void StopAllSounds();
    void PauseSound(uint32_t VoiceId);
    void ResumeSound(uint32_t VoiceId);

    void SetMasterVolume(float Volume);
    void SetSoundVolume(float Volume);
    void SetMusicVolume(float Volume);
    void SetListenerParams(const FListenerParams& Params);

    static KAudioManager* GetInstance();
};
```

## Physics API

### KPhysicsWorld (`Engine/Physics/PhysicsWorld.h`)

```cpp
class KPhysicsWorld
{
public:
    using CollisionCallback = std::function<void(uint32_t, uint32_t, const FCollisionInfo&)>;

    HRESULT Initialize(const FPhysicsSettings& Settings = FPhysicsSettings());
    void Shutdown();
    void Update(float DeltaTime);

    uint32_t CreateRigidBody();
    void DestroyRigidBody(uint32_t BodyId);
    KRigidBody* GetRigidBody(uint32_t BodyId) const;

    void SetGravity(const XMFLOAT3& Gravity);
    void SetCollisionCallback(CollisionCallback Callback);

    bool Raycast(const XMFLOAT3& Origin, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const;
    bool SphereCast(const XMFLOAT3& Origin, float Radius, const XMFLOAT3& Direction, float MaxDistance, FPhysicsRaycastHit& OutHit) const;
};
```

### KRigidBody (`Engine/Physics/RigidBody.h`)

```cpp
class KRigidBody
{
public:
    void SetPosition(const XMFLOAT3& Position);
    void SetVelocity(const XMFLOAT3& Velocity);
    void SetMass(float Mass);
    void SetColliderType(EColliderType Type);  // Sphere, Box, Capsule, Plane
    void SetBodyType(EPhysicsBodyType Type);   // Static, Dynamic, Kinematic
    void SetMaterial(const FPhysicsMaterial& InMaterial);

    void ApplyForce(const XMFLOAT3& Force);
    void ApplyImpulse(const XMFLOAT3& Impulse);
    void ApplyTorque(const XMFLOAT3& Torque);

    FAABB GetAABB() const;
    XMMATRIX GetTransformMatrix() const;
};
```

## UI API

### KUICanvas (`Engine/UI/UICanvas.h`)

```cpp
class KUICanvas
{
public:
    HRESULT Initialize(ID3D11Device* Device, UINT32 Width, UINT32 Height);
    void Update(float DeltaTime);
    void Render(ID3D11DeviceContext* Context);

    void AddElement(std::shared_ptr<KUIElement> Element);
    void RemoveElement(std::shared_ptr<KUIElement> Element);

    void OnMouseMove(float X, float Y);
    void OnMouseDown(float X, float Y, int Button);
    void OnMouseUp(float X, float Y, int Button);

    void AddQuad(float X, float Y, float Width, float Height, const FColor& Color);
    void AddText(KUIFont* Font, const std::wstring& Text, float X, float Y, float Scale, const FColor& Color);
};
```

### UI 위젯

| 클래스 | 설명 |
|--------|------|
| `KUIElement` | 베이스 위젯 (위치, 크기, 앵커, 히트 테스트) |
| `UIButton` | 버튼 (클릭 콜백) |
| `UIText` | 텍스트 표시 |
| `UICheckbox` | 체크박스 |
| `UISlider` | 슬라이더 |
| `UIImage` | 이미지 표시 |
| `UIPanel` | 패널 컨테이너 |

### 레이아웃

| 클래스 | 설명 |
|--------|------|
| `UILayout` | 베이스 레이아웃 |
| `UIGridLayout` | 그리드 레이아웃 |
| `UIHorizontalLayout` | 수평 레이아웃 |
| `UIVerticalLayout` | 수직 레이아웃 |

## Serialization API

### KBinaryArchive (`Engine/Serialization/BinaryArchive.h`)

```cpp
class KBinaryArchive
{
public:
    KBinaryArchive(EMode InMode);  // Read or Write
    bool Open(const std::wstring& Path);
    void Close();

    // 쓰기
    KBinaryArchive& operator<<(int32 Value);
    KBinaryArchive& operator<<(float Value);
    KBinaryArchive& operator<<(const std::string& Value);
    KBinaryArchive& operator<<(const XMFLOAT3& Value);
    KBinaryArchive& operator<<(const std::vector<T>& Value);

    // 읽기
    KBinaryArchive& operator>>(int32& Value);
    KBinaryArchive& operator>>(std::string& Value);
    KBinaryArchive& operator>>(XMFLOAT3& Value);
    KBinaryArchive& operator>>(std::vector<T>& Value);

    void WriteRaw(const void* Data, size_t Size);
    void ReadRaw(void* Data, size_t Size);
};
```

### KJsonArchive (`Engine/Serialization/JsonArchive.h`)

```cpp
class KJsonArchive
{
public:
    bool LoadFromFile(const std::wstring& Path);
    bool SaveToFile(const std::wstring& Path);
    JsonObjectPtr GetRoot() const;
    std::string SerializeToString() const;
    bool DeserializeFromString(const std::string& JsonString);

    static JsonObjectPtr CreateObject();
    static JsonArrayPtr CreateArray();
};

class KJsonObject
{
public:
    void Set(const std::string& Key, JsonValuePtr Value);
    void Set(const std::string& Key, int Value);
    void Set(const std::string& Key, float Value);
    void Set(const std::string& Key, const std::string& Value);
    JsonValuePtr Get(const std::string& Key) const;
    bool GetBool(const std::string& Key, bool Default = false) const;
    int GetInt(const std::string& Key, int Default = 0) const;
    float GetFloat(const std::string& Key, float Default = 0.0f) const;
    std::string GetString(const std::string& Key, const std::string& Default = "") const;
};
```

## EngineInterop C API 레퍼런스

`Editor/EngineInterop/EngineAPI.h`에 정의된 flat C API입니다. 모든 함수는 `extern "C"`로 노출되며, C#에서 P/Invoke로 호출됩니다.

### 엔진 관리

| 함수 | 설명 |
|------|------|
| `Engine_Create()` | 엔진 인스턴스 생성 |
| `Engine_Destroy(void*)` | 엔진 인스턴스 소멸 |
| `Engine_Initialize(void*, HWND, int, int)` | 엔진 초기화 |
| `Engine_InitializeEmbedded(void*, HWND, int, int)` | 외부 HWND에 엔진 임베딩 |
| `Engine_Tick(void*, float)` | 엔진 Tick |
| `Engine_Render(void*)` | 엔진 렌더 |
| `Engine_Resize(void*, int, int)` | 리사이즈 |
| `Engine_GetSceneManager(void*)` | 씬 매니저 가져오기 |
| `Engine_GetRenderer(void*)` | 렌더러 가져오기 |

### 씬 관리

| 함수 | 설명 |
|------|------|
| `Scene_Create(void*, const char*)` | 씬 생성 |
| `Scene_Load(void*, const wchar_t*)` | 씬 로드 |
| `Scene_Save(void*, const wchar_t*, void*)` | 씬 저장 |
| `Scene_SetActive(void*, void*)` | 액티브 씬 설정 |
| `Scene_GetActive(void*)` | 액티브 씬 가져오기 |

### 액터 관리

| 함수 | 설명 |
|------|------|
| `Actor_Create(void*, const char*)` | 액터 생성 |
| `Actor_Destroy(void*, void*)` | 액터 삭제 |
| `Actor_SetPosition(void*, float, float, float)` | 위치 설정 |
| `Actor_SetRotation(void*, float, float, float, float)` | 회전 설정 |
| `Actor_SetScale(void*, float, float, float)` | 스케일 설정 |
| `Actor_GetPosition(void*, float*, float*, float*)` | 위치 가져오기 |
| `Actor_AddComponent(void*, int)` | 컴포넌트 추가 |
| `Actor_SetVisibility(void*, bool)` | 가시성 설정 |
| `Actor_AddChild(void*, void*)` | 자식 액터 추가 |

### 카메라

| 함수 | 설명 |
|------|------|
| `Camera_GetMain(void*)` | 메인 카메라 가져오기 |
| `Camera_SetPosition(void*, float, float, float)` | 카메라 위치 설정 |
| `Camera_SetFOV(void*, float)` | FOV 설정 |
| `Camera_SetNearFar(void*, float, float)` | 근/원거리 평면 설정 |

### 렌더러

| 함수 | 설명 |
|------|------|
| `Renderer_SetRenderPath(void*, int)` | 렌더 패스 설정 (0=Forward, 1=Deferred) |
| `Renderer_SetDebugMode(void*, bool)` | 디버그 모드 설정 |
| `Renderer_GetStats(void*, int*, int*, float*)` | 렌더 통계 가져오기 |
| `Renderer_SetShadowEnabled(void*, bool)` | 섀도우 토글 |
| `Renderer_SetPostProcessEnabled(void*, bool)` | 후처리 토글 |
| `Renderer_SetSSAOEnabled(void*, bool)` | SSAO 토글 |
| `Renderer_SetTAAEnabled(void*, bool)` | TAA 토글 |
| `Renderer_SetSkyEnabled(void*, bool)` | 하늘 토글 |
| `Renderer_SetDebugUIEnabled(void*, bool)` | 디버그 UI 토글 |
| `Renderer_SetSSREnabled(void*, bool)` | SSR 토글 |
| `Renderer_SetVolumetricFogEnabled(void*, bool)` | 볼류메트릭 포그 토글 |
| `Renderer_SetCascadedShadowsEnabled(void*, bool)` | 캐스케이디드 섀도우 토글 |
| `Renderer_IsCascadedShadowsEnabled(void*)` | 캐스케이디드 섀도우 상태 확인 |
| `Renderer_SetIBLEnabled(void*, bool)` | IBL 토글 |
| `Renderer_IsIBLEnabled(void*)` | IBL 상태 확인 |
| `Renderer_LoadEnvironmentMap(void*, const wchar_t*)` | 환경맵 로드 |

### 조명

| 함수 | 설명 |
|------|------|
| `Renderer_SetDirectionalLight(void*, ...)` | 방향광 설정 |
| `Renderer_AddPointLight(void*, ...)` | 점광원 추가 |
| `Renderer_AddSpotLight(void*, ...)` | 스포트라이트 추가 |
| `Renderer_ClearPointLights(void*)` | 점광원 초기화 |
| `Renderer_ClearSpotLights(void*)` | 스포트라이트 초기화 |

### 디버그 렌더러

| 함수 | 설명 |
|------|------|
| `DebugRenderer_DrawGrid(void*, float, float, float, float, float, int)` | 그리드 그리기 |
| `DebugRenderer_DrawAxis(void*, float, float, float, float)` | 좌표축 그리기 |
| `DebugRenderer_SetEnabled(void*, bool)` | 디버그 렌더러 토글 |
| `DebugRenderer_RenderFrame(void*, float)` | 디버그 렌더 프레임 |

### 재질 텍스처

| 함수 | 설명 |
|------|------|
| `Material_SetTexture(void*, int, const wchar_t*)` | 재질 텍스처 슬롯 설정 |
