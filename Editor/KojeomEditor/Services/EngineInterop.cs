using System;
using System.Runtime.InteropServices;

namespace KojeomEditor.Services;

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

    #region P/Invoke declarations

    private const string DllName = "EngineInterop.dll";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr Engine_Create();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Destroy(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int Engine_Initialize(IntPtr engine, IntPtr hwnd, int width, int height);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Tick(IntPtr engine, float deltaTime);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Render(IntPtr engine);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void Engine_Resize(IntPtr engine, int width, int height);

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
    private static extern void Renderer_SetDebugMode(IntPtr renderer, bool enabled);

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

    #endregion
}
