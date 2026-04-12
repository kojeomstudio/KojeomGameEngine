using System;
using System.Runtime.InteropServices;

namespace KojeomEditor.Services;

public static class NativeComponentType
{
    public const int Base = 0;
    public const int StaticMesh = 1;
    public const int SkeletalMesh = 2;
    public const int Water = 3;
    public const int Terrain = 4;
    public const int Light = 5;
}

public class EngineInterop : IDisposable
{
    private IntPtr _enginePtr;
    private bool _isInitialized;
    private bool _disposed;

    public bool IsInitialized => _isInitialized;

    public event EventHandler<string>? LogMessage;

    public EngineInterop()
    {
        _enginePtr = IntPtr.Zero;
        _isInitialized = false;
    }

    public bool Initialize(IntPtr hwnd, int width, int height)
    {
        if (_isInitialized)
        {
            return true;
        }

        try
        {
            _enginePtr = Engine_Create();
            if (_enginePtr == IntPtr.Zero)
            {
                LogMessage?.Invoke(this, "Failed to create engine instance");
                return false;
            }

            int hr = Engine_Initialize(_enginePtr, hwnd, width, height);
            if (hr < 0)
            {
                LogMessage?.Invoke(this, $"Failed to initialize engine: 0x{hr:X8}");
                Engine_Destroy(_enginePtr);
                _enginePtr = IntPtr.Zero;
                return false;
            }

            _isInitialized = true;
            LogMessage?.Invoke(this, "Engine initialized successfully");
            return true;
        }
        catch (DllNotFoundException ex)
        {
            LogMessage?.Invoke(this, $"Engine DLL not found: {ex.Message}");
            return false;
        }
        catch (Exception ex)
        {
            LogMessage?.Invoke(this, $"Engine initialization error: {ex.Message}");
            return false;
        }
    }

    public bool InitializeEmbedded(IntPtr hwnd, int width, int height)
    {
        if (_isInitialized)
        {
            return true;
        }

        try
        {
            _enginePtr = Engine_Create();
            if (_enginePtr == IntPtr.Zero)
            {
                LogMessage?.Invoke(this, "Failed to create engine instance");
                return false;
            }

            int hr = Engine_InitializeEmbedded(_enginePtr, hwnd, width, height);
            if (hr < 0)
            {
                LogMessage?.Invoke(this, $"Failed to initialize engine: 0x{hr:X8}");
                Engine_Destroy(_enginePtr);
                _enginePtr = IntPtr.Zero;
                return false;
            }

            _isInitialized = true;
            LogMessage?.Invoke(this, "Engine initialized successfully (embedded)");
            return true;
        }
        catch (DllNotFoundException ex)
        {
            LogMessage?.Invoke(this, $"Engine DLL not found: {ex.Message}");
            return false;
        }
        catch (Exception ex)
        {
            LogMessage?.Invoke(this, $"Engine initialization error: {ex.Message}");
            return false;
        }
    }

    public void Tick(float deltaTime)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return;
        }

        Engine_Tick(_enginePtr, deltaTime);
    }

    public void Render()
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return;
        }

        Engine_Render(_enginePtr);
    }

    public void Resize(int width, int height)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return;
        }

        Engine_Resize(_enginePtr, width, height);
    }

    public IntPtr GetSceneManager()
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Engine_GetSceneManager(_enginePtr);
    }

    public IntPtr GetRenderer()
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Engine_GetRenderer(_enginePtr);
    }

    public IntPtr CreateScene(string name)
    {
        var sceneMgr = GetSceneManager();
        if (sceneMgr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Scene_Create(sceneMgr, name);
    }

    public IntPtr LoadScene(string path)
    {
        var sceneMgr = GetSceneManager();
        if (sceneMgr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Scene_Load(sceneMgr, path);
    }

    public bool SaveScene(string path, IntPtr scene)
    {
        var sceneMgr = GetSceneManager();
        if (sceneMgr == IntPtr.Zero || scene == IntPtr.Zero)
        {
            return false;
        }

        return Scene_Save(sceneMgr, path, scene) >= 0;
    }

    public void SetActiveScene(IntPtr scene)
    {
        var sceneMgr = GetSceneManager();
        if (sceneMgr != IntPtr.Zero && scene != IntPtr.Zero)
        {
            Scene_SetActive(sceneMgr, scene);
        }
    }

    public IntPtr GetActiveScene()
    {
        var sceneMgr = GetSceneManager();
        if (sceneMgr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Scene_GetActive(sceneMgr);
    }

    public IntPtr CreateActor(IntPtr scene, string name)
    {
        if (scene == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Actor_Create(scene, name);
    }

    public void SetActorPosition(IntPtr actor, float x, float y, float z)
    {
        if (actor != IntPtr.Zero)
        {
            Actor_SetPosition(actor, x, y, z);
        }
    }

    public void SetActorRotation(IntPtr actor, float x, float y, float z, float w)
    {
        if (actor != IntPtr.Zero)
        {
            Actor_SetRotation(actor, x, y, z, w);
        }
    }

    public void SetActorScale(IntPtr actor, float x, float y, float z)
    {
        if (actor != IntPtr.Zero)
        {
            Actor_SetScale(actor, x, y, z);
        }
    }

    public (float x, float y, float z) GetActorPosition(IntPtr actor)
    {
        if (actor == IntPtr.Zero)
        {
            return (0, 0, 0);
        }

        float x = 0, y = 0, z = 0;
        Actor_GetPosition(actor, ref x, ref y, ref z);
        return (x, y, z);
    }

    public string? GetActorName(IntPtr actor)
    {
        if (actor == IntPtr.Zero)
        {
            return null;
        }

        return Marshal.PtrToStringAnsi(Actor_GetName(actor));
    }

    public void SetRenderPath(int path)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero)
        {
            Renderer_SetRenderPath(renderer, path);
        }
    }

    public void SetDebugMode(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero)
        {
            Renderer_SetDebugMode(renderer, enabled);
        }
    }

    public (int drawCalls, int vertexCount, float frameTime) GetRenderStats()
    {
        var renderer = GetRenderer();
        if (renderer == IntPtr.Zero)
        {
            return (0, 0, 0);
        }

        int drawCalls = 0, vertexCount = 0;
        float frameTime = 0;
        Renderer_GetStats(renderer, ref drawCalls, ref vertexCount, ref frameTime);
        return (drawCalls, vertexCount, frameTime);
    }

    public IntPtr GetMainCamera()
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }
        return Camera_GetMain(_enginePtr);
    }

    public void SetCameraPosition(float x, float y, float z)
    {
        var camera = GetMainCamera();
        if (camera != IntPtr.Zero)
        {
            Camera_SetPosition(camera, x, y, z);
        }
    }

    public void SetCameraRotation(float pitch, float yaw, float roll)
    {
        var camera = GetMainCamera();
        if (camera != IntPtr.Zero)
        {
            Camera_SetRotation(camera, pitch, yaw, roll);
        }
    }

    public (float x, float y, float z) GetCameraPosition()
    {
        var camera = GetMainCamera();
        if (camera == IntPtr.Zero)
        {
            return (0, 0, 0);
        }
        float x = 0, y = 0, z = 0;
        Camera_GetPosition(camera, ref x, ref y, ref z);
        return (x, y, z);
    }

    public (float[] matrix, float[] projMatrix) GetCameraMatrices()
    {
        var camera = GetMainCamera();
        var viewMatrix = new float[16];
        var projMatrix = new float[16];
        if (camera != IntPtr.Zero)
        {
            Camera_GetViewMatrix(camera, viewMatrix);
            Camera_GetProjectionMatrix(camera, projMatrix);
        }
        return (viewMatrix, projMatrix);
    }

    public IntPtr Raycast(float originX, float originY, float originZ,
                          float dirX, float dirY, float dirZ,
                          out float hitX, out float hitY, out float hitZ)
    {
        hitX = hitY = hitZ = 0;
        var scene = GetActiveScene();
        if (scene == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        return Scene_Raycast(scene, originX, originY, originZ, dirX, dirY, dirZ,
                            out hitX, out hitY, out hitZ);
    }

    public int GetActorCount()
    {
        var scene = GetActiveScene();
        if (scene == IntPtr.Zero) return 0;
        return Scene_GetActorCount(scene);
    }

    public IntPtr GetActorAt(int index)
    {
        var scene = GetActiveScene();
        if (scene == IntPtr.Zero) return IntPtr.Zero;
        return Scene_GetActorAt(scene, index);
    }

    public void GetActorRotation(IntPtr actor, out float x, out float y, out float z, out float w)
    {
        x = y = z = 0; w = 1;
        if (actor != IntPtr.Zero)
        {
            Actor_GetRotation(actor, ref x, ref y, ref z, ref w);
        }
    }

    public void GetActorScale(IntPtr actor, out float x, out float y, out float z)
    {
        x = y = z = 1;
        if (actor != IntPtr.Zero)
        {
            Actor_GetScale(actor, ref x, ref y, ref z);
        }
    }

    public int GetComponentCount(IntPtr actor)
    {
        int count = 0;
        if (actor != IntPtr.Zero)
        {
            Actor_GetComponentCount(actor, ref count);
        }
        return count;
    }

    public string? GetComponentName(IntPtr actor, int index)
    {
        if (actor == IntPtr.Zero) return null;
        return Marshal.PtrToStringAnsi(Actor_GetComponentName(actor, index));
    }

    public int GetComponentType(IntPtr actor, int index)
    {
        if (actor == IntPtr.Zero) return 0;
        return Actor_GetComponentType(actor, index);
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        if (_enginePtr != IntPtr.Zero)
        {
            Engine_Destroy(_enginePtr);
            _enginePtr = IntPtr.Zero;
        }

        _isInitialized = false;
        _disposed = true;

        GC.SuppressFinalize(this);
    }

    ~EngineInterop()
    {
        Dispose();
    }

    public IntPtr AddComponent(IntPtr actor, int componentType)
    {
        if (actor != IntPtr.Zero)
        {
            return Actor_AddComponent(actor, componentType);
        }
        return IntPtr.Zero;
    }

    public void DestroyActor(IntPtr scene, IntPtr actor)
    {
        if (scene != IntPtr.Zero && actor != IntPtr.Zero)
        {
            Actor_Destroy(scene, actor);
        }
    }

    public void SetActorVisibility(IntPtr actor, bool visible)
    {
        if (actor != IntPtr.Zero)
        {
            Actor_SetVisibility(actor, visible);
        }
    }

    public bool IsActorVisible(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return false;
        return Actor_IsVisible(actor);
    }

    public void SetSSAOEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetSSAOEnabled(renderer, enabled);
    }

    public void SetPostProcessEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetPostProcessEnabled(renderer, enabled);
    }

    public void SetShadowEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetShadowEnabled(renderer, enabled);
    }

    public void SetSkyEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetSkyEnabled(renderer, enabled);
    }

    public void SetTAAEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetTAAEnabled(renderer, enabled);
    }

    public void SetDebugUIEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetDebugUIEnabled(renderer, enabled);
    }

    public void SetSSREnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetSSREnabled(renderer, enabled);
    }

    public void SetVolumetricFogEnabled(bool enabled)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetVolumetricFogEnabled(renderer, enabled);
    }

    public void SetCameraFOV(float fovY)
    {
        var camera = GetMainCamera();
        if (camera != IntPtr.Zero) Camera_SetFOV(camera, fovY);
    }

    public float GetCameraFOV()
    {
        var camera = GetMainCamera();
        return camera == IntPtr.Zero ? 0 : Camera_GetFOV(camera);
    }

    public void SetCameraNearFar(float nearZ, float farZ)
    {
        var camera = GetMainCamera();
        if (camera != IntPtr.Zero) Camera_SetNearFar(camera, nearZ, farZ);
    }

    public (float near, float far) GetCameraNearFar()
    {
        var camera = GetMainCamera();
        if (camera == IntPtr.Zero) return (0.1f, 1000);
        return (Camera_GetNearZ(camera), Camera_GetFarZ(camera));
    }

    public void SetDirectionalLight(float dirX, float dirY, float dirZ,
                                     float colorR, float colorG, float colorB, float colorA,
                                     float ambR, float ambG, float ambB, float ambA)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero)
            Renderer_SetDirectionalLight(renderer, dirX, dirY, dirZ, colorR, colorG, colorB, colorA, ambR, ambG, ambB, ambA);
    }

    public void AddPointLight(float posX, float posY, float posZ,
                               float colorR, float colorG, float colorB, float intensity,
                               float radius, float falloff)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero)
            Renderer_AddPointLight(renderer, posX, posY, posZ, colorR, colorG, colorB, intensity, radius, falloff);
    }

    public void ClearPointLights()
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_ClearPointLights(renderer);
    }

    public void AddSpotLight(float posX, float posY, float posZ,
                              float dirX, float dirY, float dirZ,
                              float colorR, float colorG, float colorB, float intensity,
                              float innerCone, float outerCone, float radius, float falloff)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero)
            Renderer_AddSpotLight(renderer, posX, posY, posZ, dirX, dirY, dirZ, colorR, colorG, colorB, intensity, innerCone, outerCone, radius, falloff);
    }

    public void ClearSpotLights()
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_ClearSpotLights(renderer);
    }

    public void SetShadowSceneBounds(float centerX, float centerY, float centerZ, float radius)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetShadowSceneBounds(renderer, centerX, centerY, centerZ, radius);
    }

    public IntPtr LoadTexture(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return IntPtr.Zero;
        return Texture_Load(_enginePtr, path);
    }

    public void UnloadTexture(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        Texture_Unload(_enginePtr, path);
    }

    public void PauseAnimation(IntPtr component)
    {
        if (component != IntPtr.Zero) SkeletalMeshComponent_PauseAnimation(component);
    }

    public void ResumeAnimation(IntPtr component)
    {
        if (component != IntPtr.Zero) SkeletalMeshComponent_ResumeAnimation(component);
    }

    public string? GetAnimationName(IntPtr component, int index)
    {
        if (component == IntPtr.Zero) return null;
        return Marshal.PtrToStringAnsi(SkeletalMeshComponent_GetAnimationName(component, index));
    }

    public int ModelHasSkeleton(IntPtr model)
    {
        if (model == IntPtr.Zero) return 0;
        return Model_HasSkeleton(model);
    }

    public int GetModelAnimationCount(IntPtr model)
    {
        if (model == IntPtr.Zero) return 0;
        return Model_GetAnimationCount(model);
    }

    public string? GetModelAnimationName(IntPtr model, int index)
    {
        if (model == IntPtr.Zero) return null;
        return Marshal.PtrToStringAnsi(Model_GetAnimationName(model, index));
    }

    public void AddChild(IntPtr parent, IntPtr child)
    {
        if (parent != IntPtr.Zero && child != IntPtr.Zero)
            Actor_AddChild(parent, child);
    }

    public int GetChildCount(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return 0;
        return Actor_GetChildCount(actor);
    }

    public IntPtr GetChild(IntPtr actor, int index)
    {
        if (actor == IntPtr.Zero) return IntPtr.Zero;
        return Actor_GetChild(actor, index);
    }

    public IntPtr GetParent(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return IntPtr.Zero;
        return Actor_GetParent(actor);
    }

    public IntPtr LoadModel(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return IntPtr.Zero;
        return Model_Load(_enginePtr, path);
    }

    public IntPtr SetMesh(IntPtr component, string meshPath)
    {
        if (component == IntPtr.Zero) return IntPtr.Zero;
        return StaticMeshComponent_SetMesh(component, meshPath);
    }

    public void SetDirectionalLightDirection(float x, float y, float z)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetDirectionalLightDirection(renderer, x, y, z);
    }

    public void SetDirectionalLightColor(float r, float g, float b, float a)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetDirectionalLightColor(renderer, r, g, b, a);
    }

    public void SetDirectionalLightAmbient(float r, float g, float b, float a)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetDirectionalLightAmbient(renderer, r, g, b, a);
    }

    public (float posX, float posY, float posZ, float colorR, float colorG, float colorB, float intensity, float radius, float falloff) GetPointLight(int index)
    {
        var renderer = GetRenderer();
        if (renderer == IntPtr.Zero) return (0, 0, 0, 0, 0, 0, 0, 0, 0);
        Renderer_GetPointLight(renderer, index,
            out float posX, out float posY, out float posZ,
            out float colorR, out float colorG, out float colorB, out float intensity,
            out float radius, out float falloff);
        return (posX, posY, posZ, colorR, colorG, colorB, intensity, radius, falloff);
    }

    public IntPtr LoadAndGetSkeletalMesh(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return IntPtr.Zero;
        return Model_LoadAndGetSkeletalMesh(_enginePtr, path);
    }

    #region P/Invoke declarations

    private const string DllName = "EngineInterop.dll";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Engine_Create();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Destroy(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Engine_Initialize(IntPtr engine, IntPtr hwnd, int width, int height);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Engine_InitializeEmbedded(IntPtr engine, IntPtr hwnd, int width, int height);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Tick(IntPtr engine, float deltaTime);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Render(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Resize(IntPtr engine, int width, int height);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Model_Load(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr StaticMeshComponent_SetMesh(IntPtr component, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Engine_GetSceneManager(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Engine_GetRenderer(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Scene_Create(IntPtr sceneMgr, [MarshalAs(UnmanagedType.LPStr)] string name);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Scene_Load(IntPtr sceneMgr, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Scene_Save(IntPtr sceneMgr, [MarshalAs(UnmanagedType.LPWStr)] string path, IntPtr scene);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Scene_SetActive(IntPtr sceneMgr, IntPtr scene);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Scene_GetActive(IntPtr sceneMgr);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_Create(IntPtr scene, [MarshalAs(UnmanagedType.LPStr)] string name);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_Destroy(IntPtr scene, IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_SetPosition(IntPtr actor, float x, float y, float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_SetRotation(IntPtr actor, float x, float y, float z, float w);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_SetScale(IntPtr actor, float x, float y, float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_GetPosition(IntPtr actor, ref float x, ref float y, ref float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetName(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetRenderPath(IntPtr renderer, int path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDebugMode(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_GetStats(IntPtr renderer, ref int drawCalls, ref int vertexCount, ref float frameTime);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Camera_GetMain(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_SetPosition(IntPtr camera, float x, float y, float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_SetRotation(IntPtr camera, float pitch, float yaw, float roll);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_GetPosition(IntPtr camera, ref float x, ref float y, ref float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_GetViewMatrix(IntPtr camera, [Out] float[] matrix);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_GetProjectionMatrix(IntPtr camera, [Out] float[] matrix);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Scene_Raycast(IntPtr scene, float originX, float originY, float originZ,
                                               float dirX, float dirY, float dirZ,
                                               out float outHitX, out float outHitY, out float outHitZ);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Scene_GetActorCount(IntPtr scene);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Scene_GetActorAt(IntPtr scene, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_GetRotation(IntPtr actor, ref float x, ref float y, ref float z, ref float w);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_GetScale(IntPtr actor, ref float x, ref float y, ref float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_GetComponentCount(IntPtr actor, ref int outCount);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetComponentName(IntPtr actor, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Actor_GetComponentType(IntPtr actor, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Material_SetAlbedo(IntPtr material, float r, float g, float b, float a);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Material_SetMetallic(IntPtr material, float value);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Material_SetRoughness(IntPtr material, float value);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_AddComponent(IntPtr actor, int componentType);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_SetVisibility(IntPtr actor, [MarshalAs(UnmanagedType.I1)] bool visible);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    private static extern bool Actor_IsVisible(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetSSAOEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetPostProcessEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetShadowEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetSkyEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetTAAEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDebugUIEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetSSREnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetVolumetricFogEnabled(IntPtr renderer, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_SetFOV(IntPtr camera, float fovY);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Camera_GetFOV(IntPtr camera);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Camera_SetNearFar(IntPtr camera, float nearZ, float farZ);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Camera_GetNearZ(IntPtr camera);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Camera_GetFarZ(IntPtr camera);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDirectionalLight(IntPtr renderer, float dirX, float dirY, float dirZ,
                                                              float colorR, float colorG, float colorB, float colorA,
                                                              float ambR, float ambG, float ambB, float ambA);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_AddPointLight(IntPtr renderer, float posX, float posY, float posZ,
                                                       float colorR, float colorG, float colorB, float intensity,
                                                       float radius, float falloff);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_ClearPointLights(IntPtr renderer);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_AddSpotLight(IntPtr renderer, float posX, float posY, float posZ,
                                                      float dirX, float dirY, float dirZ,
                                                      float colorR, float colorG, float colorB, float intensity,
                                                      float innerCone, float outerCone, float radius, float falloff);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_ClearSpotLights(IntPtr renderer);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetShadowSceneBounds(IntPtr renderer, float centerX, float centerY, float centerZ, float radius);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Texture_Load(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Texture_Unload(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void SkeletalMeshComponent_PauseAnimation(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void SkeletalMeshComponent_ResumeAnimation(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr SkeletalMeshComponent_GetAnimationName(IntPtr component, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Model_HasSkeleton(IntPtr model);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Model_GetAnimationCount(IntPtr model);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Model_GetAnimationName(IntPtr model, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_AddChild(IntPtr parent, IntPtr child);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Actor_GetChildCount(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetChild(IntPtr actor, int index);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetParent(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDirectionalLightDirection(IntPtr renderer, float x, float y, float z);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDirectionalLightColor(IntPtr renderer, float r, float g, float b, float a);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDirectionalLightAmbient(IntPtr renderer, float r, float g, float b, float a);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Actor_SetName(IntPtr actor, [MarshalAs(UnmanagedType.LPStr)] string name);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr StaticMeshComponent_GetMaterial(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr StaticMeshComponent_CreateDefaultMesh(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_GetDirectionalLight(IntPtr renderer,
        out float dirX, out float dirY, out float dirZ,
        out float colorR, out float colorG, out float colorB, out float colorA,
        out float ambR, out float ambG, out float ambB, out float ambA);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Renderer_GetPointLightCount(IntPtr renderer);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_GetPointLight(IntPtr renderer, int index,
        out float posX, out float posY, out float posZ,
        out float colorR, out float colorG, out float colorB, out float intensity,
        out float radius, out float falloff);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Renderer_SetDirectionalLightIntensity(IntPtr renderer, float intensity);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Renderer_GetDirectionalLightIntensity(IntPtr renderer);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Model_LoadAndGetSkeletalMesh(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void SkeletalMeshComponent_SetSkeletalMeshFromModel(IntPtr component, IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Material_SetAO(IntPtr material, float value);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Material_GetAlbedo(IntPtr material, out float r, out float g, out float b, out float a);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Material_GetMetallic(IntPtr material);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Material_GetRoughness(IntPtr material);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float Material_GetAO(IntPtr material);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetStaticMeshComponent(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DebugRenderer_DrawGrid(IntPtr engine, float centerX, float centerY, float centerZ, float size, float cellSize, int subdivisions);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DebugRenderer_DrawAxis(IntPtr engine, float originX, float originY, float originZ, float length);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DebugRenderer_SetEnabled(IntPtr engine, [MarshalAs(UnmanagedType.I1)] bool enabled);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void DebugRenderer_RenderFrame(IntPtr engine, float deltaTime);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Model_Unload(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetSkeletalMeshComponent(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Actor_GetLightComponent(IntPtr actor);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr SkeletalMeshComponent_PlayAnimation(IntPtr component, [MarshalAs(UnmanagedType.LPStr)] string animName);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void SkeletalMeshComponent_StopAnimation(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int SkeletalMeshComponent_GetAnimationCount(IntPtr component);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Model_LoadAndGetStaticMesh(IntPtr engine, [MarshalAs(UnmanagedType.LPWStr)] string path);

    #endregion

    public void SetActorName(IntPtr actor, string name)
    {
        if (actor != IntPtr.Zero) Actor_SetName(actor, name);
    }

    public IntPtr GetStaticMeshComponentMaterial(IntPtr component)
    {
        if (component == IntPtr.Zero) return IntPtr.Zero;
        return StaticMeshComponent_GetMaterial(component);
    }

    public IntPtr CreateDefaultMesh(IntPtr component)
    {
        if (component == IntPtr.Zero) return IntPtr.Zero;
        return StaticMeshComponent_CreateDefaultMesh(component);
    }

    public (float dirX, float dirY, float dirZ, float colorR, float colorG, float colorB, float colorA, float ambR, float ambG, float ambB, float ambA) GetDirectionalLight()
    {
        var renderer = GetRenderer();
        if (renderer == IntPtr.Zero) return (0,0,0, 1,1,1,1, 0,0,0,1);
        Renderer_GetDirectionalLight(renderer,
            out float dirX, out float dirY, out float dirZ,
            out float colorR, out float colorG, out float colorB, out float colorA,
            out float ambR, out float ambG, out float ambB, out float ambA);
        return (dirX, dirY, dirZ, colorR, colorG, colorB, colorA, ambR, ambG, ambB, ambA);
    }

    public int GetPointLightCount()
    {
        var renderer = GetRenderer();
        return renderer == IntPtr.Zero ? 0 : Renderer_GetPointLightCount(renderer);
    }

    public void SetDirectionalLightIntensity(float intensity)
    {
        var renderer = GetRenderer();
        if (renderer != IntPtr.Zero) Renderer_SetDirectionalLightIntensity(renderer, intensity);
    }

    public float GetDirectionalLightIntensity()
    {
        var renderer = GetRenderer();
        return renderer == IntPtr.Zero ? 0 : Renderer_GetDirectionalLightIntensity(renderer);
    }

    public void SetMaterialAO(IntPtr material, float ao)
    {
        if (material != IntPtr.Zero) Material_SetAO(material, ao);
    }

    public (float r, float g, float b, float a) GetMaterialAlbedo(IntPtr material)
    {
        if (material == IntPtr.Zero) return (1, 1, 1, 1);
        Material_GetAlbedo(material, out float r, out float g, out float b, out float a);
        return (r, g, b, a);
    }

    public float GetMaterialMetallic(IntPtr material)
    {
        return material == IntPtr.Zero ? 0 : Material_GetMetallic(material);
    }

    public float GetMaterialRoughness(IntPtr material)
    {
        return material == IntPtr.Zero ? 0 : Material_GetRoughness(material);
    }

    public float GetMaterialAO(IntPtr material)
    {
        return material == IntPtr.Zero ? 0 : Material_GetAO(material);
    }

    public void SetSkeletalMeshFromModel(IntPtr component, string path)
    {
        if (component == IntPtr.Zero) return;
        SkeletalMeshComponent_SetSkeletalMeshFromModel(component, _enginePtr, path);
    }

    public IntPtr GetActorStaticMeshComponent(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return IntPtr.Zero;
        return Actor_GetStaticMeshComponent(actor);
    }

    public void SetMaterialAlbedo(IntPtr material, float r, float g, float b, float a)
    {
        if (material != IntPtr.Zero) Material_SetAlbedo(material, r, g, b, a);
    }

    public void SetMaterialMetallic(IntPtr material, float value)
    {
        if (material != IntPtr.Zero) Material_SetMetallic(material, value);
    }

    public void SetMaterialRoughness(IntPtr material, float value)
    {
        if (material != IntPtr.Zero) Material_SetRoughness(material, value);
    }

    public void DebugRendererDrawGrid(float centerX, float centerY, float centerZ, float size, float cellSize, int subdivisions)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        DebugRenderer_DrawGrid(_enginePtr, centerX, centerY, centerZ, size, cellSize, subdivisions);
    }

    public void DebugRendererDrawAxis(float originX, float originY, float originZ, float length)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        DebugRenderer_DrawAxis(_enginePtr, originX, originY, originZ, length);
    }

    public void DebugRendererSetEnabled(bool enabled)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        DebugRenderer_SetEnabled(_enginePtr, enabled);
    }

    public void DebugRendererRenderFrame(float deltaTime)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        DebugRenderer_RenderFrame(_enginePtr, deltaTime);
    }

    public void UnloadModel(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return;
        Model_Unload(_enginePtr, path);
    }

    public IntPtr GetActorSkeletalMeshComponent(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return IntPtr.Zero;
        return Actor_GetSkeletalMeshComponent(actor);
    }

    public IntPtr GetActorLightComponent(IntPtr actor)
    {
        if (actor == IntPtr.Zero) return IntPtr.Zero;
        return Actor_GetLightComponent(actor);
    }

    public IntPtr PlayAnimation(IntPtr component, string animName)
    {
        if (component == IntPtr.Zero) return IntPtr.Zero;
        return SkeletalMeshComponent_PlayAnimation(component, animName);
    }

    public void StopAnimation(IntPtr component)
    {
        if (component != IntPtr.Zero) SkeletalMeshComponent_StopAnimation(component);
    }

    public int GetSkeletalMeshAnimationCount(IntPtr component)
    {
        if (component == IntPtr.Zero) return 0;
        return SkeletalMeshComponent_GetAnimationCount(component);
    }

    public IntPtr LoadAndGetStaticMesh(string path)
    {
        if (!_isInitialized || _enginePtr == IntPtr.Zero) return IntPtr.Zero;
        return Model_LoadAndGetStaticMesh(_enginePtr, path);
    }
}
