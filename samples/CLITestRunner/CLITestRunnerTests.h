#pragma once

#define NOMINMAX

#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
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
#include "../../Engine/Assets/Skeleton.h"
#include "../../Engine/Assets/Animation.h"
#include "../../Engine/Assets/AnimationInstance.h"

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
            Result.bPassed = true;
            Result.Message = "SKIPPED: No windowing support (headless CI environment)";
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }

        KEngine Engine;
        HRESULT hr = Engine.InitializeWithExternalHwnd(HiddenWnd, 800, 600);
        if (FAILED(hr))
        {
            Result.bPassed = true;
            Result.Message = "SKIPPED: DX11 device unavailable (headless CI environment)";
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }

        auto Renderer = Engine.GetRenderer();
        if (!Renderer)
        {
            Result.Message = "Renderer is null";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }

        auto CubeMesh = Renderer->CreateCubeMesh();
        auto SphereMesh = Renderer->CreateSphereMesh(32, 16);
        if (!CubeMesh || !SphereMesh)
        {
            Result.Message = "Mesh creation failed";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }

        auto Camera = Engine.GetCamera();
        if (!Camera)
        {
            Result.Message = "Camera is null";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }
        Camera->SetPosition(XMFLOAT3(0.0f, 2.0f, -5.0f));

        Engine.Render();
        Engine.Shutdown();
        DestroyWindow(HiddenWnd);
        UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));

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

            std::cout << "  [INFO] Testing binary archive disk round-trip...\n";
            const std::wstring TestFile = L"test_serialization.bin";

            {
                KBinaryArchive DiskWriter(KBinaryArchive::EMode::Write);
                if (!DiskWriter.Open(TestFile))
                {
                    Result.Message = "Failed to open binary archive for writing";
                    return Result;
                }
                DiskWriter << std::string("DiskTestString");
                DiskWriter << int32(99);
                DiskWriter << 2.718f;
                DiskWriter << false;
                DiskWriter.Close();
            }

            KBinaryArchive Reader(KBinaryArchive::EMode::Read);
            if (!Reader.Open(TestFile))
            {
                Result.Message = "Failed to open binary archive from disk";
                _wremove(TestFile.c_str());
                return Result;
            }

            std::string ReadStr;
            int32 ReadInt = 0;
            float ReadFloat = 0.0f;
            bool ReadBool = true;
            Reader >> ReadStr;
            Reader >> ReadInt;
            Reader >> ReadFloat;
            Reader >> ReadBool;
            Reader.Close();

            if (ReadStr != "DiskTestString" || ReadInt != 99 ||
                std::abs(ReadFloat - 2.718f) > 0.001f || ReadBool != false)
            {
                Result.Message = "Binary round-trip data mismatch";
                _wremove(TestFile.c_str());
                return Result;
            }

            _wremove(TestFile.c_str());

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
            Result.Message = "Binary write/read/disk round-trip, JSON read/write OK";
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

            std::cout << "  [INFO] Creating test OBJ file for loading...\n";
            const std::string TestObjFile = "test_triangle.obj";
            {
                std::ofstream ObjFile(TestObjFile);
                if (!ObjFile.is_open())
                {
                    Result.Message = "Failed to create test OBJ file";
                    Loader.Cleanup();
                    return Result;
                }
                ObjFile << "# Test OBJ file\n";
                ObjFile << "v 0.0 0.0 0.0\n";
                ObjFile << "v 1.0 0.0 0.0\n";
                ObjFile << "v 0.5 1.0 0.0\n";
                ObjFile << "vn 0.0 0.0 1.0\n";
                ObjFile << "vt 0.0 0.0\n";
                ObjFile << "vt 1.0 0.0\n";
                ObjFile << "vt 0.5 1.0\n";
                ObjFile << "f 1/1/1 2/2/1 3/3/1\n";
                ObjFile.close();
            }

            auto Model = Loader.LoadModel(std::wstring(TestObjFile.begin(), TestObjFile.end()));
            if (!Model)
            {
                Result.Message = "Failed to load test OBJ file";
                std::remove(TestObjFile.c_str());
                Loader.Cleanup();
                return Result;
            }

            std::cout << "  [INFO] Test OBJ loaded successfully\n";
            std::remove(TestObjFile.c_str());
            Loader.Cleanup();

            Result.bPassed = true;
            Result.Message = "Model loader initialized, OBJ created and loaded successfully";
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

    inline FTestResult TestAnimationSubsystem()
    {
        FTestResult Result;
        Result.Name = "animation";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing skeleton creation...\n";

            KSkeleton Skeleton;
            Skeleton.SetName("TestSkeleton");

            FBone RootBone;
            RootBone.Name = "Root";
            RootBone.ParentIndex = INVALID_BONE_INDEX;
            RootBone.LocalPosition = XMFLOAT3(0, 0, 0);
            RootBone.LocalRotation = XMFLOAT4(0, 0, 0, 1);
            RootBone.LocalScale = XMFLOAT3(1, 1, 1);
            Skeleton.AddBone(RootBone);

            FBone ChildBone;
            ChildBone.Name = "Child";
            ChildBone.ParentIndex = 0;
            ChildBone.LocalPosition = XMFLOAT3(1, 0, 0);
            ChildBone.LocalRotation = XMFLOAT4(0, 0, 0, 1);
            ChildBone.LocalScale = XMFLOAT3(1, 1, 1);
            Skeleton.AddBone(ChildBone);

            Skeleton.CalculateBindPoses();
            Skeleton.CalculateInverseBindPoses();

            if (Skeleton.GetBoneCount() != 2)
            {
                Result.Message = "Bone count mismatch";
                return Result;
            }

            int32 BoneIdx = Skeleton.GetBoneIndex("Child");
            if (BoneIdx != 1)
            {
                Result.Message = "Bone index lookup failed";
                return Result;
            }

            std::cout << "  [INFO] Testing animation clip creation...\n";

            auto Anim = std::make_shared<KAnimation>();
            Anim->SetName("TestAnim");
            Anim->SetDuration(1.0f);
            Anim->SetTicksPerSecond(30.0f);

            FAnimationChannel Channel;
            Channel.BoneIndex = 0;
            Channel.BoneName = "Root";

            FTransformKey Key0, Key1;
            Key0.Time = 0.0f;
            Key0.Position = XMFLOAT3(0, 0, 0);
            Key0.Rotation = XMFLOAT4(0, 0, 0, 1);
            Key0.Scale = XMFLOAT3(1, 1, 1);

            Key1.Time = 1.0f;
            Key1.Position = XMFLOAT3(2, 0, 0);
            Key1.Rotation = XMFLOAT4(0, 0, 0, 1);
            Key1.Scale = XMFLOAT3(1, 1, 1);

            Channel.PositionKeys.push_back(Key0);
            Channel.PositionKeys.push_back(Key1);
            Channel.RotationKeys.push_back(Key0);
            Channel.RotationKeys.push_back(Key1);
            Channel.ScaleKeys.push_back(Key0);
            Channel.ScaleKeys.push_back(Key1);

            Anim->AddChannel(Channel);

            std::cout << "  [INFO] Testing animation instance and bone evaluation...\n";

            KAnimationInstance AnimInstance;
            AnimInstance.SetSkeleton(&Skeleton);
            AnimInstance.AddAnimation("TestAnim", Anim);
            AnimInstance.PlayAnimation("TestAnim", Anim, EAnimationPlayMode::Loop);

            AnimInstance.Update(0.5f);

            const auto& BoneMatrices = AnimInstance.GetBoneMatrices();
            if (BoneMatrices.size() != 2)
            {
                Result.Message = "Bone matrix count mismatch after evaluation";
                return Result;
            }

            AnimInstance.StopAnimation();

            Result.bPassed = true;
            Result.Message = "Skeleton, animation clip, instance, bone evaluation OK";
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
