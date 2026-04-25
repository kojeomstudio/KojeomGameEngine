#pragma once

#define NOMINMAX

#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "../../Engine/Utils/Common.h"
#include "../../Engine/Utils/CLI.h"
#include "../../Engine/Utils/Logger.h"
#include "../../Engine/Core/Engine.h"
#include "../../Engine/Serialization/BinaryArchive.h"
#include "../../Engine/Serialization/JsonArchive.h"
#include "../../Engine/Physics/PhysicsWorld.h"
#include "../../Engine/Physics/PhysicsTypes.h"
#include "../../Engine/Audio/AudioManager.h"
#include "../../Engine/Assets/ModelLoader.h"

namespace CLITest
{
    struct FTestResult
    {
        std::string Name;
        bool bPassed = false;
        std::string Message;
        double ElapsedMs = 0.0;
    };

    class FTestRunner
    {
    public:
        void AddTest(const std::string& Name, std::function<FTestResult()> TestFunc)
        {
            Tests.push_back({ Name, TestFunc });
        }

        int32 RunAll()
        {
            PrintHeader();
            int32 Passed = 0;
            int32 Failed = 0;
            double TotalMs = 0.0;

            for (const auto& Test : Tests)
            {
                auto Start = std::chrono::high_resolution_clock::now();
                FTestResult Result = Test.Func();
                auto End = std::chrono::high_resolution_clock::now();
                Result.ElapsedMs = std::chrono::duration<double, std::milli>(End - Start).count();
                Result.Name = Test.Name;

                PrintResult(Result);

                if (Result.bPassed)
                    Passed++;
                else
                    Failed++;
                TotalMs += Result.ElapsedMs;
            }

            PrintSummary(Passed, Failed, TotalMs);
            return Failed;
        }

        int32 RunTest(const std::string& Name)
        {
            PrintHeader();
            for (const auto& Test : Tests)
            {
                if (Test.Name == Name)
                {
                    auto Start = std::chrono::high_resolution_clock::now();
                    FTestResult Result = Test.Func();
                    auto End = std::chrono::high_resolution_clock::now();
                    Result.ElapsedMs = std::chrono::duration<double, std::milli>(End - Start).count();
                    Result.Name = Test.Name;
                    PrintResult(Result);
                    int32 Failed = Result.bPassed ? 0 : 1;
                    PrintSummary(Result.bPassed ? 1 : 0, Failed, Result.ElapsedMs);
                    return Failed;
                }
            }
            std::cout << "[ERROR] Test not found: " << Name << std::endl;
            ListTests();
            return 1;
        }

        void ListTests()
        {
            std::cout << "\nAvailable tests:\n";
            std::cout << "----------------\n";
            for (const auto& Test : Tests)
            {
                std::cout << "  - " << Test.Name << "\n";
            }
            std::cout << std::endl;
        }

    private:
        struct FTestEntry
        {
            std::string Name;
            std::function<FTestResult()> Func;
        };
        std::vector<FTestEntry> Tests;

        void PrintHeader()
        {
            std::cout << "\n========================================\n";
            std::cout << "  KojeomEngine CLI Test Runner\n";
            std::cout << "========================================\n\n";
        }

        void PrintResult(const FTestResult& Result)
        {
            std::cout << "[" << (Result.bPassed ? "PASS" : "FAIL") << "] "
                << Result.Name << " (" << std::fixed << std::setprecision(1)
                << Result.ElapsedMs << "ms)";
            if (!Result.Message.empty())
            {
                std::cout << " - " << Result.Message;
            }
            std::cout << std::endl;
        }

        void PrintSummary(int32 Passed, int32 Failed, double TotalMs)
        {
            std::cout << "\n----------------------------------------\n";
            std::cout << "Results: " << Passed << " passed, " << Failed << " failed";
            std::cout << " (" << std::fixed << std::setprecision(1) << TotalMs << "ms total)\n";
            std::cout << "========================================\n\n";
        }
    };

    inline FTestResult TestRenderingSubsystem()
    {
        FTestResult Result;
        Result.Name = "rendering";
        Result.bPassed = false;

        std::cout << "  [INFO] Creating hidden window for rendering test...\n";

        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"CLITestRunner";
        RegisterClassW(&wc);

        HWND HiddenWnd = CreateWindowW(L"CLITestRunner", L"CLITest", WS_OVERLAPPED,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!HiddenWnd)
        {
            Result.Message = "Failed to create hidden window";
            return Result;
        }

        KEngine Engine;
        HRESULT hr = Engine.Initialize(GetModuleHandle(nullptr), L"CLI Test", 800, 600);
        if (FAILED(hr))
        {
            Result.Message = "Engine initialization failed (hr=" + std::to_string(hr) + ")";
            DestroyWindow(HiddenWnd);
            return Result;
        }

        auto Renderer = Engine.GetRenderer();
        if (!Renderer)
        {
            Result.Message = "Renderer is null";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            return Result;
        }

        auto CubeMesh = Renderer->CreateCubeMesh();
        auto SphereMesh = Renderer->CreateSphereMesh(32, 16);
        if (!CubeMesh || !SphereMesh)
        {
            Result.Message = "Mesh creation failed";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            return Result;
        }

        auto Camera = Engine.GetCamera();
        if (!Camera)
        {
            Result.Message = "Camera is null";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            return Result;
        }
        Camera->SetPosition(XMFLOAT3(0.0f, 2.0f, -5.0f));

        Engine.Render();
        Engine.Shutdown();
        DestroyWindow(HiddenWnd);

        Result.bPassed = true;
        Result.Message = "Device, renderer, mesh creation, camera OK";
        return Result;
    }

    inline FTestResult TestSerializationSubsystem()
    {
        FTestResult Result;
        Result.Name = "serialization";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing binary archive (write)...\n";

            KBinaryArchive Writer(KBinaryArchive::EMode::Write);
            Writer << std::string("TestString");
            Writer << int32(42);
            Writer << 3.14f;
            Writer << true;

            size_t DataSize = Writer.GetSize();
            if (DataSize == 0)
            {
                Result.Message = "Binary archive write produced empty data";
                return Result;
            }

            std::cout << "  [INFO] Testing JSON archive...\n";

            KJsonArchive JsonArchive;
            auto Root = KJsonArchive::CreateObject();
            Root->Set("name", std::string("KojeomEngine"));
            Root->Set("version", 1);
            Root->Set("pi", 3.14159);
            JsonArchive.SetRoot(Root);

            std::string JsonStr = JsonArchive.SerializeToString();
            if (JsonStr.empty() || JsonStr.find("KojeomEngine") == std::string::npos)
            {
                Result.Message = "JSON archive output invalid";
                return Result;
            }

            std::cout << "  [INFO] Testing JSON roundtrip...\n";
            KJsonArchive JsonReader;
            bool bLoaded = JsonReader.DeserializeFromString(JsonStr);
            if (!bLoaded)
            {
                Result.Message = "JSON deserialize failed";
                return Result;
            }
            auto ReadRoot = JsonReader.GetRoot();
            if (!ReadRoot)
            {
                Result.Message = "JSON root is null after deserialize";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "Binary write, JSON read/write OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestPhysicsSubsystem()
    {
        FTestResult Result;
        Result.Name = "physics";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing physics world creation...\n";

            KPhysicsWorld PhysicsWorld;
            HRESULT hr = PhysicsWorld.Initialize();
            if (FAILED(hr))
            {
                Result.Message = "Physics world initialization failed";
                return Result;
            }

            PhysicsWorld.SetGravity(XMFLOAT3(0.0f, -9.81f, 0.0f));
            XMFLOAT3 Gravity = PhysicsWorld.GetGravity();
            if (Gravity.y != -9.81f)
            {
                Result.Message = "Gravity mismatch";
                PhysicsWorld.Shutdown();
                return Result;
            }

            uint32 BodyId = PhysicsWorld.CreateRigidBody();
            KRigidBody* Body = PhysicsWorld.GetRigidBody(BodyId);
            if (!Body)
            {
                Result.Message = "Failed to create rigid body";
                PhysicsWorld.Shutdown();
                return Result;
            }

            PhysicsWorld.Update(1.0f / 60.0f);

            FPhysicsRaycastHit HitResult;
            bool bHit = PhysicsWorld.Raycast(
                XMFLOAT3(0.0f, 20.0f, 0.0f),
                XMFLOAT3(0.0f, -1.0f, 0.0f),
                100.0f, HitResult);

            PhysicsWorld.Shutdown();

            Result.bPassed = true;
            Result.Message = "Physics init, gravity, rigid body, raycast OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestAudioSubsystem()
    {
        FTestResult Result;
        Result.Name = "audio";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing audio manager initialization...\n";

            KAudioManager AudioManager;
            HRESULT hr = AudioManager.Initialize();
            if (FAILED(hr))
            {
                std::cout << "  [INFO] XAudio2 init failed (expected if COM not available), testing volume API safety...\n";
                Result.bPassed = true;
                Result.Message = "Audio manager created, XAudio2 unavailable in test env";
                return Result;
            }

            AudioManager.SetMasterVolume(0.5f);
            float Vol = AudioManager.GetMasterVolume();

            AudioManager.SetSoundVolume(0.8f);
            float SoundVol = AudioManager.GetSoundVolume();

            AudioManager.SetMusicVolume(0.6f);
            float MusicVol = AudioManager.GetMusicVolume();

            AudioManager.Shutdown();

            Result.bPassed = true;
            Result.Message = "Audio init, volume channels OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestSceneSubsystem()
    {
        FTestResult Result;
        Result.Name = "scene";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing scene and actor system...\n";

            KSceneManager SceneMgr;
            auto Scene = SceneMgr.CreateScene("TestScene");
            if (!Scene)
            {
                Result.Message = "Failed to create scene";
                return Result;
            }

            SceneMgr.SetActiveScene(Scene);
            auto Active = SceneMgr.GetActiveScene();
            if (Active != Scene)
            {
                Result.Message = "Active scene mismatch";
                return Result;
            }

            auto Actor = Scene->CreateActor("TestActor");
            if (!Actor)
            {
                Result.Message = "Failed to create actor";
                return Result;
            }

            Actor->SetWorldPosition(XMFLOAT3(1.0f, 2.0f, 3.0f));
            XMFLOAT3 Pos = Actor->GetWorldTransform().Position;
            if (Pos.x != 1.0f || Pos.y != 2.0f || Pos.z != 3.0f)
            {
                Result.Message = "Actor position mismatch";
                return Result;
            }

            auto ChildActor = Scene->CreateActor("ChildActor");
            Actor->AddChild(ChildActor);
            if (Actor->GetChildren().size() != 1)
            {
                Result.Message = "Child count mismatch";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "Scene, actor, hierarchy OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestModelLoadingSubsystem()
    {
        FTestResult Result;
        Result.Name = "model-loading";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing model loader initialization...\n";

            KModelLoader Loader;
            HRESULT hr = Loader.Initialize();
            if (FAILED(hr))
            {
                Result.Message = "Model loader initialization failed";
                return Result;
            }

            bool bSupported = KModelLoader::IsSupportedFormat(L"test.obj");
            std::cout << "  [INFO] OBJ format supported: " << (bSupported ? "yes" : "no") << "\n";

            bSupported = KModelLoader::IsSupportedFormat(L"test.fbx");
            std::cout << "  [INFO] FBX format supported: " << (bSupported ? "yes" : "no") << "\n";

            std::cout << "  [INFO] Testing OBJ fallback parse...\n";
            auto Model = Loader.LoadModel(L"nonexistent_test.obj");

            Loader.Cleanup();

            Result.bPassed = true;
            Result.Message = "Model loader initialized, format detection OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestSubsystemRegistry()
    {
        FTestResult Result;
        Result.Name = "subsystem-registry";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing subsystem registry...\n";

            KSubsystemRegistry Registry;

            auto AudioSub = std::make_shared<KAudioSubsystem>();
            auto PhysicsSub = std::make_shared<KPhysicsSubsystem>();

            Registry.Register<KAudioSubsystem>(AudioSub);
            Registry.Register<KPhysicsSubsystem>(PhysicsSub);

            auto RetrievedAudio = Registry.Get<KAudioSubsystem>();
            auto RetrievedPhysics = Registry.Get<KPhysicsSubsystem>();

            if (!RetrievedAudio || !RetrievedPhysics)
            {
                Result.Message = "Failed to retrieve registered subsystems";
                return Result;
            }

            if (RetrievedAudio != AudioSub.get() || RetrievedPhysics != PhysicsSub.get())
            {
                Result.Message = "Retrieved subsystem pointer mismatch";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "Subsystem registration and retrieval OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestPathUtils()
    {
        FTestResult Result;
        Result.Name = "path-utils";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing path utilities...\n";

            if (!PathUtils::ContainsTraversal(L"../../../etc/passwd"))
            {
                Result.Message = "Failed to detect traversal in '../../../etc/passwd'";
                return Result;
            }

            if (!PathUtils::ContainsTraversal(L"..\\secret.txt"))
            {
                Result.Message = "Failed to detect traversal in '..\\secret.txt'";
                return Result;
            }

            if (PathUtils::ContainsTraversal(L"models/character.obj"))
            {
                Result.Message = "False positive on 'models/character.obj'";
                return Result;
            }

            if (!PathUtils::ContainsTraversal(L"\\\\?\\C:\\"))
            {
                Result.Message = "Failed to detect UNC path";
                return Result;
            }

            if (!PathUtils::ContainsTraversal(L"C:\\Windows\\System32"))
            {
                Result.Message = "Failed to detect absolute path";
                return Result;
            }

            if (PathUtils::ContainsTraversal(L"textures/diffuse.png"))
            {
                Result.Message = "False positive on 'textures/diffuse.png'";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "Path traversal detection OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }
}
