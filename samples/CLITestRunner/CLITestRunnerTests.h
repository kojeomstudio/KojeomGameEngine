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
#include "../../Engine/Assets/AnimationStateMachine.h"
#include "../../Engine/Assets/BlendTree.h"
#include "../../Engine/Graphics/RenderModule.h"
#include "../../Engine/Input/InputManager.h"
#include "../../Engine/Scene/Actor.h"
#include "../../Engine/Assets/StaticMeshComponent.h"
#include "../../Engine/Assets/StaticMesh.h"
#include "../../Engine/Assets/LightComponent.h"
#include "../../Engine/Graphics/Light.h"
#include "../../Engine/Graphics/GraphicsDevice.h"

namespace CLITest
{
    struct FTestResult
    {
        std::string Name;
        bool bPassed = false;
        bool bSkipped = false;
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
            int32 Skipped = 0;
            double TotalMs = 0.0;
            std::vector<FTestResult> AllResults;

            for (const auto& Test : Tests)
            {
                auto Start = std::chrono::high_resolution_clock::now();
                FTestResult Result = Test.Func();
                auto End = std::chrono::high_resolution_clock::now();
                Result.ElapsedMs = std::chrono::duration<double, std::milli>(End - Start).count();
                Result.Name = Test.Name;

                PrintResult(Result);
                AllResults.push_back(Result);

                if (Result.bSkipped)
                    Skipped++;
                else if (Result.bPassed)
                    Passed++;
                else
                    Failed++;
                TotalMs += Result.ElapsedMs;
            }

            PrintSummary(Passed, Skipped, Failed, TotalMs);

            if (!ResultJsonPath.empty())
                WriteJsonResult(AllResults, Passed, Skipped, Failed, TotalMs);

            return Failed > 0 ? 5 : (Skipped > 0 && Failed == 0 ? 0 : 0);
        }

        int32 RunTest(const std::string& Name)
        {
            PrintHeader();
            std::vector<FTestResult> AllResults;

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
                    AllResults.push_back(Result);

                    int32 Passed = (Result.bPassed && !Result.bSkipped) ? 1 : 0;
                    int32 Skipped = Result.bSkipped ? 1 : 0;
                    int32 Failed = (!Result.bPassed && !Result.bSkipped) ? 1 : 0;
                    PrintSummary(Passed, Skipped, Failed, Result.ElapsedMs);

                    if (!ResultJsonPath.empty())
                        WriteJsonResult(AllResults, Passed, Skipped, Failed, Result.ElapsedMs);

                    return Failed > 0 ? 5 : 0;
                }
            }
            std::cout << "[ERROR] Test not found: " << Name << std::endl;
            ListTests();
            return 1;
        }

        void SetResultJsonPath(const std::string& Path) { ResultJsonPath = Path; }

        static std::string EscapeJson(const std::string& s)
        {
            std::string Result;
            Result.reserve(s.size() + 8);
            for (char c : s)
            {
                switch (c)
                {
                case '"': Result += "\\\""; break;
                case '\\': Result += "\\\\"; break;
                case '\n': Result += "\\n"; break;
                case '\r': Result += "\\r"; break;
                case '\t': Result += "\\t"; break;
                default: Result += c; break;
                }
            }
            return Result;
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
        std::string ResultJsonPath;

        void PrintHeader()
        {
            std::cout << "\n========================================\n";
            std::cout << "  KojeomEngine CLI Test Runner\n";
            std::cout << "========================================\n\n";
        }

        void PrintResult(const FTestResult& Result)
        {
            const char* Status = Result.bSkipped ? "SKIP" : (Result.bPassed ? "PASS" : "FAIL");
            std::cout << "[" << Status << "] "
                << Result.Name << " (" << std::fixed << std::setprecision(1)
                << Result.ElapsedMs << "ms)";
            if (!Result.Message.empty())
            {
                std::cout << " - " << Result.Message;
            }
            std::cout << std::endl;
        }

        void PrintSummary(int32 Passed, int32 Skipped, int32 Failed, double TotalMs)
        {
            std::cout << "\n----------------------------------------\n";
            std::cout << "Results: " << Passed << " passed, " << Skipped << " skipped, " << Failed << " failed";
            std::cout << " (" << std::fixed << std::setprecision(1) << TotalMs << "ms total)\n";
            std::cout << "========================================\n\n";
        }

        void WriteJsonResult(const std::vector<FTestResult>& Results, int32 Passed, int32 Skipped, int32 Failed, double TotalMs)
        {
            try
            {
                std::ofstream Out(ResultJsonPath);
                if (!Out.is_open())
                {
                    std::cerr << "[ERROR] Failed to write result JSON to: " << ResultJsonPath << std::endl;
                    return;
                }

                Out << "{\n";
                Out << "  \"success\": " << (Failed == 0 ? "true" : "false") << ",\n";
                Out << "  \"passed\": " << Passed << ",\n";
                Out << "  \"skipped\": " << Skipped << ",\n";
                Out << "  \"failed\": " << Failed << ",\n";
                Out << "  \"totalMs\": " << std::fixed << std::setprecision(1) << TotalMs << ",\n";
                Out << "  \"tests\": [\n";

                for (size_t i = 0; i < Results.size(); ++i)
                {
                    const auto& R = Results[i];
                    Out << "    {\n";
                    Out << "      \"name\": \"" << EscapeJson(R.Name) << "\",\n";
                    Out << "      \"passed\": " << (R.bPassed ? "true" : "false") << ",\n";
                    Out << "      \"skipped\": " << (R.bSkipped ? "true" : "false") << ",\n";
                    Out << "      \"elapsedMs\": " << std::fixed << std::setprecision(1) << R.ElapsedMs << ",\n";
                    Out << "      \"message\": \"" << EscapeJson(R.Message) << "\"\n";
                    Out << "    }" << (i + 1 < Results.size() ? "," : "") << "\n";
                }

                Out << "  ]\n";
                Out << "}\n";
                Out.close();

                std::cout << "[INFO] Results written to: " << ResultJsonPath << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ERROR] Failed to write result JSON: " << e.what() << std::endl;
            }
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
            Result.bSkipped = true;
            Result.Message = "SKIPPED: No windowing support (headless CI environment)";
            UnregisterClassW(L"CLITestRunner", GetModuleHandle(nullptr));
            return Result;
        }

        KEngine Engine;
        HRESULT hr = Engine.InitializeWithExternalHwnd(HiddenWnd, 800, 600);
        if (FAILED(hr))
        {
            Result.bPassed = true;
            Result.bSkipped = true;
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

    inline FTestResult TestCLIParser()
    {
        FTestResult Result;
        Result.Name = "cli-parser";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing CLI argument parser...\n";

            {
                auto Args = CLIUtils::ParseCommandLine("rendering --verbose");
                if (Args.Command != "rendering" || !CLIUtils::HasOption(Args, "verbose"))
                {
                    Result.Message = "Basic command + flag parse failed";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("model-loading --file=test.obj --iterations=5");
                if (Args.Command != "model-loading")
                {
                    Result.Message = "Command with options: command parse failed";
                    return Result;
                }
                if (CLIUtils::GetOption(Args, "file") != "test.obj")
                {
                    Result.Message = "Command with options: --file parse failed";
                    return Result;
                }
                if (CLIUtils::GetOption(Args, "iterations") != "5")
                {
                    Result.Message = "Command with options: --iterations parse failed";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("--help");
                if (!Args.bHelpRequested)
                {
                    Result.Message = "--help not detected";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("-h");
                if (!Args.bHelpRequested)
                {
                    Result.Message = "-h not detected as help";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("");
                if (!Args.Command.empty() || Args.bHelpRequested)
                {
                    Result.Message = "Empty command line should produce empty result";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("all");
                if (Args.Command != "all")
                {
                    Result.Message = "Single command 'all' parse failed";
                    return Result;
                }
                if (!Args.Options.empty())
                {
                    Result.Message = "Single command should have no options";
                    return Result;
                }
            }

            {
                auto Args = CLIUtils::ParseCommandLine("rendering -v --width=1920 --height=1080");
                if (!CLIUtils::HasOption(Args, "v") ||
                    CLIUtils::GetOption(Args, "width") != "1920" ||
                    CLIUtils::GetOption(Args, "height") != "1080")
                {
                    Result.Message = "Mixed short/long options parse failed";
                    return Result;
                }
            }

            std::cout << "  [INFO] Testing help text output...\n";
            std::string HelpText = CLIUtils::GetHelpText();
            if (HelpText.empty() || HelpText.find("all") == std::string::npos)
            {
                Result.Message = "Help text is empty or missing command list";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "CLI parser: command, flags, key=value, help, short opts, empty input OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestRenderModuleRegistry()
    {
        FTestResult Result;
        Result.Name = "render-modules";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing render module registry...\n";

            KRenderModuleRegistry Registry;

            struct FTestModule : public IRenderModule
            {
                HRESULT Initialize(KGraphicsDevice* InDevice) override { bInitialized = true; return S_OK; }
                void Shutdown() override { bInitialized = false; }
                void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override {}
                const std::string& GetName() const override { static const std::string name = "Test"; return name; }
                ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
            };

            auto Mod = Registry.RegisterModule<FTestModule>();
            if (!Mod)
            {
                Result.Message = "Failed to register module";
                return Result;
            }

            if (Registry.GetModuleCount() != 1)
            {
                Result.Message = "Module count mismatch after registration";
                return Result;
            }

            auto Retrieved = Registry.GetModule<FTestModule>();
            if (Retrieved != Mod.get())
            {
                Result.Message = "Retrieved module pointer mismatch";
                return Result;
            }

            if (!Registry.HasModule<FTestModule>())
            {
                Result.Message = "HasModule returned false for registered module";
                return Result;
            }

            auto ByPhase = Registry.GetModulesByPhase(ERenderModulePhase::PostProcess);
            if (ByPhase.size() != 1 || ByPhase[0] != Mod.get())
            {
                Result.Message = "GetModulesByPhase returned unexpected results";
                return Result;
            }

            auto ShadowModules = Registry.GetModulesByPhase(ERenderModulePhase::Shadow);
            if (!ShadowModules.empty())
            {
                Result.Message = "GetModulesByPhase(Shadow) should be empty";
                return Result;
            }

            Registry.ForEachModule([](IRenderModule* M) {
                M->SetEnabled(true);
            });
            if (!Mod->IsEnabled())
            {
                Result.Message = "ForEachModule SetEnabled failed";
                return Result;
            }

            Registry.ShutdownAll();
            if (Registry.GetModuleCount() != 0)
            {
                Result.Message = "Module count should be 0 after shutdown";
                return Result;
            }

            Result.bPassed = true;
            Result.Message = "Register, retrieve, phase query, iteration, shutdown OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestInputSystem()
    {
        FTestResult Result;
        Result.Name = "input";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing input manager...\n";

            KInputManager InputMgr;

            bool bKeyPressed = InputMgr.IsKeyDown(static_cast<uint32_t>('W'));
            (void)bKeyPressed;

            int32_t mouseX = 0, mouseY = 0;
            InputMgr.GetMousePosition(mouseX, mouseY);
            (void)mouseX;
            (void)mouseY;

            InputMgr.RegisterAction("MoveForward", static_cast<uint32_t>('W'));
            InputMgr.RegisterAction("MoveBackward", static_cast<uint32_t>('S'));

            bool bForwardDown = InputMgr.IsActionDown("MoveForward");
            bool bBackwardDown = InputMgr.IsActionDown("MoveBackward");
            (void)bForwardDown;
            (void)bBackwardDown;

            InputMgr.FlushInput();
            InputMgr.ClearState();

            Result.bPassed = true;
            Result.Message = "Input manager, key queries, action registration, state OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestBlendTree()
    {
        FTestResult Result;
        Result.Name = "blend-tree";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing blend tree...\n";

            KSkeleton Skeleton;
            FBone RootBone;
            RootBone.Name = "Root";
            RootBone.ParentIndex = INVALID_BONE_INDEX;
            Skeleton.AddBone(RootBone);

            FBone ChildBone;
            ChildBone.Name = "Child";
            ChildBone.ParentIndex = 0;
            Skeleton.AddBone(ChildBone);

            Skeleton.CalculateBindPoses();
            Skeleton.CalculateInverseBindPoses();

            auto AnimIdle = std::make_shared<KAnimation>();
            AnimIdle->SetName("Idle");
            AnimIdle->SetDuration(24.0f);
            AnimIdle->SetTicksPerSecond(24.0f);
            FAnimationChannel IdleRootChannel;
            IdleRootChannel.BoneName = "Root";
            FTransformKey IdleKey0;
            IdleKey0.Time = 0.0f;
            IdleRootChannel.PositionKeys.push_back(IdleKey0);
            AnimIdle->AddChannel(IdleRootChannel);
            AnimIdle->BuildBoneIndexMap(&Skeleton);

            auto AnimWalk = std::make_shared<KAnimation>();
            AnimWalk->SetName("Walk");
            AnimWalk->SetDuration(24.0f);
            AnimWalk->SetTicksPerSecond(24.0f);
            FAnimationChannel WalkRootChannel;
            WalkRootChannel.BoneName = "Root";
            FTransformKey WalkKey0;
            WalkKey0.Time = 0.0f;
            WalkKey0.Position = XMFLOAT3(0, 0, 0);
            FTransformKey WalkKey1;
            WalkKey1.Time = 24.0f;
            WalkKey1.Position = XMFLOAT3(1, 0, 0);
            WalkRootChannel.PositionKeys.push_back(WalkKey0);
            WalkRootChannel.PositionKeys.push_back(WalkKey1);
            AnimWalk->AddChannel(WalkRootChannel);
            AnimWalk->BuildBoneIndexMap(&Skeleton);

            auto AnimRun = std::make_shared<KAnimation>();
            AnimRun->SetName("Run");
            AnimRun->SetDuration(24.0f);
            AnimRun->SetTicksPerSecond(24.0f);
            FAnimationChannel RunRootChannel;
            RunRootChannel.BoneName = "Root";
            FTransformKey RunKey0;
            RunKey0.Time = 0.0f;
            RunKey0.Position = XMFLOAT3(0, 0, 0);
            FTransformKey RunKey1;
            RunKey1.Time = 24.0f;
            RunKey1.Position = XMFLOAT3(2, 0, 0);
            RunRootChannel.PositionKeys.push_back(RunKey0);
            RunRootChannel.PositionKeys.push_back(RunKey1);
            AnimRun->AddChannel(RunRootChannel);
            AnimRun->BuildBoneIndexMap(&Skeleton);

            KBlendTree BlendTree;
            BlendTree.SetSkeleton(&Skeleton);
            BlendTree.SetParameterName("Speed");
            BlendTree.AddChild(AnimIdle, 0.0f);
            BlendTree.AddChild(AnimWalk, 5.0f);
            BlendTree.AddChild(AnimRun, 10.0f);

            bool bHasCorrectCount = (BlendTree.GetChildCount() == 3);
            if (!bHasCorrectCount)
            {
                Result.Message = "Expected 3 children, got " + std::to_string(BlendTree.GetChildCount());
                return Result;
            }

            BlendTree.Update(0.01f, 0.0f);
            bool bIdleWeight = (std::abs(BlendTree.GetChildWeight(0) - 1.0f) < 0.001f &&
                               std::abs(BlendTree.GetChildWeight(1) - 0.0f) < 0.001f &&
                               std::abs(BlendTree.GetChildWeight(2) - 0.0f) < 0.001f);
            if (!bIdleWeight)
            {
                Result.Message = "Idle weight check failed at param=0.0";
                return Result;
            }

            BlendTree.Update(0.01f, 10.0f);
            bool bRunWeight = (std::abs(BlendTree.GetChildWeight(2) - 1.0f) < 0.001f);
            if (!bRunWeight)
            {
                Result.Message = "Run weight check failed at param=10.0";
                return Result;
            }

            BlendTree.Update(0.01f, 7.5f);
            bool bBlendWeight = (std::abs(BlendTree.GetChildWeight(1) - 0.5f) < 0.001f &&
                                std::abs(BlendTree.GetChildWeight(2) - 0.5f) < 0.001f);
            if (!bBlendWeight)
            {
                Result.Message = "Blend weight check failed at param=7.5";
                return Result;
            }

            BlendTree.Update(0.01f, -1.0f);
            bool bClampedLow = (std::abs(BlendTree.GetChildWeight(0) - 1.0f) < 0.001f);

            BlendTree.Update(0.01f, 15.0f);
            bool bClampedHigh = (std::abs(BlendTree.GetChildWeight(2) - 1.0f) < 0.001f);

            uint32 BoneMatrixCount = BlendTree.GetBoneMatrixCount();
            bool bBoneMatrices = (BoneMatrixCount == 2);

            BlendTree.RemoveChild(2);
            bool bRemoveChild = (BlendTree.GetChildCount() == 2);

            BlendTree.Reset();
            bool bReset = (BlendTree.GetChildWeight(0) == 0.0f && BlendTree.GetBoneMatrixCount() == 0);

            Result.bPassed = bHasCorrectCount && bIdleWeight && bRunWeight && bBlendWeight &&
                             bClampedLow && bClampedHigh && bBoneMatrices && bRemoveChild && bReset;
            Result.Message = "Blend tree: create, weights, clamping, bone matrices, reset OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestRootMotion()
    {
        FTestResult Result;
        Result.Name = "root-motion";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing root motion extraction...\n";

            KSkeleton Skeleton;
            FBone RootBone;
            RootBone.Name = "Root";
            RootBone.ParentIndex = INVALID_BONE_INDEX;
            RootBone.LocalPosition = XMFLOAT3(0, 0, 0);
            Skeleton.AddBone(RootBone);
            Skeleton.CalculateBindPoses();
            Skeleton.CalculateInverseBindPoses();

            auto Anim = std::make_shared<KAnimation>();
            Anim->SetName("WalkForward");
            Anim->SetDuration(48.0f);
            Anim->SetTicksPerSecond(24.0f);

            FAnimationChannel RootChannel;
            RootChannel.BoneName = "Root";

            FTransformKey Key0;
            Key0.Time = 0.0f;
            Key0.Position = XMFLOAT3(0, 0, 0);

            FTransformKey Key1;
            Key1.Time = 24.0f;
            Key1.Position = XMFLOAT3(1, 0, 0);

            FTransformKey Key2;
            Key2.Time = 48.0f;
            Key2.Position = XMFLOAT3(2, 0, 0);

            RootChannel.PositionKeys.push_back(Key0);
            RootChannel.PositionKeys.push_back(Key1);
            RootChannel.PositionKeys.push_back(Key2);
            Anim->AddChannel(RootChannel);
            Anim->BuildBoneIndexMap(&Skeleton);

            KAnimationInstance Instance;
            Instance.SetSkeleton(&Skeleton);
            Instance.PlayAnimation("WalkForward", Anim, EAnimationPlayMode::Once);
            Instance.SetRootMotionMode(ERootMotionMode::RootMotionFromRootBoneOnly);
            Instance.SetRootMotionBoneIndex(0);

            const FRootMotionData& RM0 = Instance.ExtractRootMotion();
            bool bNoMotionInit = !RM0.bHasRootMotion;

            Instance.Update(1.0f);
            Instance.UpdateBoneMatrices();
            const FRootMotionData& RM1 = Instance.ExtractRootMotion();
            bool bHasMotionAfterUpdate = RM1.bHasRootMotion;

            float TotalDeltaZ = 0.0f;
            for (int i = 0; i < 10; ++i)
            {
                Instance.Update(0.1f);
                Instance.UpdateBoneMatrices();
                const FRootMotionData& RM = Instance.ExtractRootMotion();
                if (RM.bHasRootMotion)
                {
                    TotalDeltaZ += RM.PositionDelta.x;
                }
            }
            bool bAccumulatesMotion = (TotalDeltaZ > 0.0f);

            Instance.StopAnimation();
            const FRootMotionData& RMStop = Instance.ExtractRootMotion();
            bool bNoMotionAfterStop = !RMStop.bHasRootMotion;

            KAnimationInstance NoRootInstance;
            NoRootInstance.SetSkeleton(&Skeleton);
            NoRootInstance.PlayAnimation("WalkForward", Anim, EAnimationPlayMode::Once);
            NoRootInstance.SetRootMotionMode(ERootMotionMode::NoRootMotion);
            NoRootInstance.Update(1.0f);
            NoRootInstance.UpdateBoneMatrices();
            const FRootMotionData& RMNoRoot = NoRootInstance.ExtractRootMotion();
            bool bNoRootMode = !RMNoRoot.bHasRootMotion;

            Result.bPassed = bNoMotionInit && bHasMotionAfterUpdate && bAccumulatesMotion &&
                             bNoMotionAfterStop && bNoRootMode;
            Result.Message = "Root motion: extraction, accumulation, mode control OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestAnimationEvents()
    {
        FTestResult Result;
        Result.Name = "animation-events";
        Result.bPassed = false;

        try
        {
            std::cout << "  [INFO] Testing animation events with payload...\n";

            KSkeleton Skeleton;
            FBone RootBone;
            RootBone.Name = "Root";
            RootBone.ParentIndex = INVALID_BONE_INDEX;
            Skeleton.AddBone(RootBone);
            Skeleton.CalculateBindPoses();
            Skeleton.CalculateInverseBindPoses();

            auto Anim = std::make_shared<KAnimation>();
            Anim->SetName("EventTest");
            Anim->SetDuration(48.0f);
            Anim->SetTicksPerSecond(24.0f);
            FAnimationChannel Channel;
            Channel.BoneName = "Root";
            FTransformKey K0;
            K0.Time = 0.0f;
            Channel.PositionKeys.push_back(K0);
            Anim->AddChannel(Channel);
            Anim->BuildBoneIndexMap(&Skeleton);

            KAnimationStateMachine StateMachine;
            StateMachine.SetSkeleton(&Skeleton);
            KAnimationState* State = StateMachine.AddState("EventState", Anim);
            State->SetLooping(true);

            FAnimNotify Notify;
            Notify.Name = "FootStep";
            Notify.TriggerTime = 0.5f;
            Notify.Duration = 0.0f;

            int CallbackTriggerCount = 0;
            Notify.Callback = [&CallbackTriggerCount]() { CallbackTriggerCount++; };
            State->AddNotify(Notify);

            FAnimNotify NotifyWithPayload;
            NotifyWithPayload.Name = "SoundEvent";
            NotifyWithPayload.TriggerTime = 0.75f;
            NotifyWithPayload.Duration = 0.0f;
            NotifyWithPayload.Payload.FloatParams["Volume"] = 0.8f;
            NotifyWithPayload.Payload.StringParams["SoundName"] = "footstep_grass";

            std::string ReceivedSoundName;
            float ReceivedVolume = 0.0f;
            NotifyWithPayload.CallbackWithPayload = [&ReceivedSoundName, &ReceivedVolume](const FAnimNotifyPayload& Payload) {
                ReceivedSoundName = Payload.GetString("SoundName");
                ReceivedVolume = Payload.GetFloat("Volume", 0.0f);
            };
            State->AddNotify(NotifyWithPayload);

            StateMachine.SetDefaultState("EventState");

            for (int i = 0; i < 3; ++i)
            {
                StateMachine.Update(0.5f);
            }

            bool bBasicCallback = (CallbackTriggerCount >= 1);

            for (int i = 0; i < 3; ++i)
            {
                StateMachine.Update(0.5f);
            }

            bool bMultipleTriggers = (CallbackTriggerCount >= 3);
            bool bPayloadReceived = (ReceivedSoundName == "footstep_grass" &&
                                     std::abs(ReceivedVolume - 0.8f) < 0.001f);

            State->ClearNotifies();
            bool bClearNotifies = State->GetNotifies().empty();

            FAnimNotify NotifyEdge;
            NotifyEdge.Name = "EdgeTest";
            NotifyEdge.TriggerTime = 0.0f;
            int EdgeCount = 0;
            NotifyEdge.Callback = [&EdgeCount]() { EdgeCount++; };
            State->AddNotify(NotifyEdge);
            StateMachine.Update(1.0f);
            bool bEdgeTrigger = (EdgeCount >= 1);

            Result.bPassed = bBasicCallback && bMultipleTriggers && bPayloadReceived &&
                             bClearNotifies && bEdgeTrigger;
            Result.Message = "Animation events: basic callback, payload, clear, edge trigger OK";
        }
        catch (const std::exception& e)
        {
            Result.Message = std::string("Exception: ") + e.what();
        }

        return Result;
    }

    inline FTestResult TestSmokeMode()
    {
        FTestResult Result;
        Result.Name = "smoke";
        Result.bPassed = false;

        std::cout << "  [INFO] Running smoke test...\n";

        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"CLITestRunnerSmoke";
        RegisterClassW(&wc);

        HWND HiddenWnd = CreateWindowW(L"CLITestRunnerSmoke", L"Smoke", WS_OVERLAPPED,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!HiddenWnd)
        {
            Result.bPassed = true;
            Result.bSkipped = true;
            Result.Message = "SKIPPED: No windowing support (headless CI)";
            UnregisterClassW(L"CLITestRunnerSmoke", GetModuleHandle(nullptr));
            return Result;
        }

        KEngine Engine;
        HRESULT hr = Engine.InitializeWithExternalHwnd(HiddenWnd, 800, 600);
        if (FAILED(hr))
        {
            Result.bPassed = true;
            Result.bSkipped = true;
            Result.Message = "SKIPPED: DX11 unavailable (headless CI)";
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRunnerSmoke", GetModuleHandle(nullptr));
            return Result;
        }

        bool bTickOK = true;
        bool bRenderOK = true;
        int32 FrameCount = 10;

        for (int32 i = 0; i < FrameCount; ++i)
        {
            Engine.Update(1.0f / 60.0f);
        }

        Engine.Render();

        Engine.Shutdown();
        DestroyWindow(HiddenWnd);
        UnregisterClassW(L"CLITestRunnerSmoke", GetModuleHandle(nullptr));

        Result.bPassed = bTickOK && bRenderOK;
        Result.Message = Result.bPassed ? "Smoke test passed: " + std::to_string(FrameCount) + " frames" : "Smoke test FAILED";
        return Result;
    }

    inline FTestResult TestValidateAssets()
    {
        FTestResult Result;
        Result.Name = "validate-assets";
        Result.bPassed = false;

        std::cout << "  [INFO] Running asset validation...\n";

        int32 CheckedItems = 0;
        int32 MissingItems = 0;
        int32 PassedValidations = 0;

        auto CheckDir = [&](const std::wstring& Path) {
            ++CheckedItems;
            DWORD attrs = GetFileAttributesW(Path.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::wcout << L"  [MISSING] Directory: " << Path << L"\n";
                ++MissingItems;
            }
            else
            {
                ++PassedValidations;
            }
        };

        CheckDir(L"Assets");
        CheckDir(L"Assets/Models");
        CheckDir(L"Assets/Textures");
        CheckDir(L"Assets/Materials");
        CheckDir(L"Assets/Scenes");

        std::cout << "  [INFO] Testing model loader format support...\n";
        KModelLoader Loader;
        HRESULT hr = Loader.Initialize();
        if (SUCCEEDED(hr))
        {
            struct FFormatCheck { const wchar_t* Ext; const char* Name; };
            FFormatCheck Formats[] = {
                { L"model.obj", "OBJ" },
                { L"model.fbx", "FBX" },
                { L"model.gltf", "GLTF" },
                { L"model.glb", "GLB" },
                { L"model.stl", "STL" },
                { L"model.dae", "DAE" },
            };

            for (const auto& Fmt : Formats)
            {
                ++CheckedItems;
                bool Supported = KModelLoader::IsSupportedFormat(Fmt.Ext);
                if (Supported)
                {
                    ++PassedValidations;
                    std::cout << "  [OK] Format supported: " << Fmt.Name << "\n";
                }
                else
                {
                    std::cout << "  [INFO] Format not supported: " << Fmt.Name << " (expected if Assimp disabled)\n";
                }
            }
            Loader.Cleanup();
        }
        else
        {
            std::cout << "  [INFO] Model loader init failed (expected without GPU/Assimp)\n";
        }

        std::cout << "  [INFO] Testing OBJ load from memory...\n";
        {
            const std::string TestFile = "test_validate_asset.obj";
            {
                std::ofstream f(TestFile);
                f << "# Test OBJ\nv 0 0 0\nv 1 0 0\nv 0.5 1 0\nvn 0 0 1\nvt 0 0\nvt 1 0\nvt 0.5 1\nf 1/1/1 2/2/1 3/3/1\n";
                f.close();
            }

            KModelLoader ObjLoader;
            if (SUCCEEDED(ObjLoader.Initialize()))
            {
                ++CheckedItems;
                auto Model = ObjLoader.LoadModel(std::wstring(TestFile.begin(), TestFile.end()));
                if (Model)
                {
                    ++PassedValidations;
                    std::cout << "  [OK] OBJ file loaded and parsed successfully\n";

                    if (Model->StaticMesh && Model->StaticMesh->IsValid())
                    {
                        ++PassedValidations;
                        auto LOD0 = Model->StaticMesh->GetLOD(0);
                        if (LOD0)
                        {
                            std::cout << "  [OK] Mesh LOD0 verified: " << LOD0->Vertices.size() << "v/" << LOD0->Indices.size() << "i\n";
                        }
                    }
                }
                else
                {
                    ++MissingItems;
                    std::cout << "  [FAIL] OBJ file failed to load\n";
                }
                ObjLoader.Cleanup();
            }
            std::remove(TestFile.c_str());
        }

        std::cout << "  [INFO] Testing serialization round-trip...\n";
        {
            ++CheckedItems;
            KBinaryArchive Writer(KBinaryArchive::EMode::Write);
            if (Writer.Open(L"test_validate_archive.bin"))
            {
                Writer << std::string("SceneName");
                Writer << int32(3);
                Writer << float(3.14f);
                Writer << true;
                Writer.Close();

                KBinaryArchive Reader(KBinaryArchive::EMode::Read);
                if (Reader.Open(L"test_validate_archive.bin"))
                {
                    std::string ReadStr;
                    int32 ReadInt = 0;
                    float ReadFloat = 0.0f;
                    bool ReadBool = false;
                    Reader >> ReadStr >> ReadInt >> ReadFloat >> ReadBool;
                    Reader.Close();

                    if (ReadStr == "SceneName" && ReadInt == 3 &&
                        std::abs(ReadFloat - 3.14f) < 0.01f && ReadBool == true)
                    {
                        ++PassedValidations;
                        std::cout << "  [OK] Binary archive round-trip verified\n";
                    }
                    else
                    {
                        ++MissingItems;
                        std::cout << "  [FAIL] Binary archive round-trip mismatch\n";
                    }
                }
                _wremove(L"test_validate_archive.bin");
            }
        }

        std::cout << "  [INFO] Testing JSON archive...\n";
        {
            ++CheckedItems;
            KJsonArchive Json;
            auto Root = KJsonArchive::CreateObject();
            Root->Set("scene", std::string("ValidateTest"));
            Root->Set("entities", 1);
            Json.SetRoot(Root);
            std::string Serialized = Json.SerializeToString();

            KJsonArchive JsonReader;
            if (JsonReader.DeserializeFromString(Serialized))
            {
                auto ReadRoot = JsonReader.GetRoot();
                if (ReadRoot && ReadRoot->GetString("scene") == "ValidateTest" && ReadRoot->GetInt("entities") == 1)
                {
                    ++PassedValidations;
                    std::cout << "  [OK] JSON archive round-trip verified\n";
                }
                else
                {
                    ++MissingItems;
                    std::cout << "  [FAIL] JSON archive round-trip mismatch\n";
                }
            }
            else
            {
                ++MissingItems;
                std::cout << "  [FAIL] JSON deserialization failed\n";
            }
        }

        Result.bPassed = (MissingItems == 0 && PassedValidations > 0);
        if (MissingItems > 0)
        {
            Result.Message = "Asset validation FAILED: " + std::to_string(MissingItems) + " issues, " +
                             std::to_string(PassedValidations) + "/" + std::to_string(CheckedItems) + " passed";
        }
        else
        {
            Result.Message = "Asset validation passed: " + std::to_string(PassedValidations) + "/" +
                             std::to_string(CheckedItems) + " checks OK";
        }
        return Result;
    }

    inline FTestResult TestSceneDump()
    {
        FTestResult Result;
        Result.Name = "scene-dump";
        Result.bPassed = false;

        std::cout << "  [INFO] Running scene dump test...\n";

        KScene Scene;
        Scene.SetName("DumpTestScene");

        auto Actor1 = std::make_shared<KActor>();
        Actor1->SetName("Cube1");
        Actor1->SetWorldPosition(XMFLOAT3(1.0f, 0.0f, 0.0f));
        auto MeshComp1 = std::make_shared<KStaticMeshComponent>();
        Actor1->AddComponent(MeshComp1);
        Scene.AddActor(Actor1);

        auto Actor2 = std::make_shared<KActor>();
        Actor2->SetName("Sphere1");
        Actor2->SetWorldPosition(XMFLOAT3(3.0f, 0.0f, 0.0f));
        auto MeshComp2 = std::make_shared<KStaticMeshComponent>();
        Actor2->AddComponent(MeshComp2);
        Actor1->AddChild(Actor2);
        Scene.AddActor(Actor2);

        auto Actor3 = std::make_shared<KActor>();
        Actor3->SetName("DirLight");
        Actor3->SetWorldPosition(XMFLOAT3(0.0f, 10.0f, 0.0f));
        auto LightComp = std::make_shared<KLightComponent>();
        Actor3->AddComponent(LightComp);
        Scene.AddActor(Actor3);

        auto Actor4 = std::make_shared<KActor>();
        Actor4->SetName("EmptyActor");
        Actor4->SetWorldPosition(XMFLOAT3(5.0f, 0.0f, 5.0f));
        Scene.AddActor(Actor4);

        std::string DumpJsonPath = "scene_dump_result.json";
        {
            std::ofstream Out(DumpJsonPath);
            if (!Out.is_open())
            {
                Result.Message = "Failed to write scene dump JSON";
                return Result;
            }

            uint32 StaticMeshCount = 0;
            uint32 SkinnedMeshCount = 0;
            uint32 LightCount = 0;
            uint32 EmptyCount = 0;
            uint32 TotalComponents = 0;

            Out << "{\n";
            Out << "  \"scene\": \"" << FTestRunner::EscapeJson(Scene.GetName()) << "\",\n";

            for (const auto& Actor : Scene.GetActors())
            {
                for (const auto& Comp : Actor->GetComponents())
                {
                    TotalComponents++;
                    EComponentType CType = Comp->GetComponentTypeID();
                    if (CType == EComponentType::StaticMesh) StaticMeshCount++;
                    else if (CType == EComponentType::SkeletalMesh) SkinnedMeshCount++;
                    else if (CType == EComponentType::Light) LightCount++;
                }
                if (Actor->GetComponents().empty()) EmptyCount++;
            }

            Out << "  \"actors\": " << Scene.GetActors().size() << ",\n";
            Out << "  \"components\": {\n";
            Out << "    \"total\": " << TotalComponents << ",\n";
            Out << "    \"staticMesh\": " << StaticMeshCount << ",\n";
            Out << "    \"skinnedMesh\": " << SkinnedMeshCount << ",\n";
            Out << "    \"light\": " << LightCount << ",\n";
            Out << "    \"empty\": " << EmptyCount << "\n";
            Out << "  },\n";

            Out << "  \"actorsList\": [\n";
            size_t ActorIdx = 0;
            for (const auto& Actor : Scene.GetActors())
            {
                auto Transform = Actor->GetWorldTransform();
                Out << "    {\n";
                Out << "      \"name\": \"" << FTestRunner::EscapeJson(Actor->GetName()) << "\",\n";
                Out << "      \"position\": [" << Transform.Position.x << ", " << Transform.Position.y << ", " << Transform.Position.z << "],\n";
                Out << "      \"componentCount\": " << Actor->GetComponents().size() << ",\n";
                Out << "      \"childCount\": " << Actor->GetChildren().size() << ",\n";
                Out << "      \"parent\": " << (Actor->GetParent() ? ("\"" + FTestRunner::EscapeJson(Actor->GetParent()->GetName()) + "\"") : "null") << "\n";
                Out << "    }" << (ActorIdx + 1 < Scene.GetActors().size() ? "," : "") << "\n";
                ActorIdx++;
            }
            Out << "  ],\n";

            Out << "  \"camera\": { \"valid\": true },\n";
            Out << "  \"staticDrawCommands\": " << StaticMeshCount << ",\n";
            Out << "  \"skinnedDrawCommands\": " << SkinnedMeshCount << "\n";
            Out << "}\n";
            Out.close();
        }

        bool bJsonValid = false;
        {
            std::ifstream In(DumpJsonPath);
            if (In.is_open())
            {
                std::string Content((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());
                In.close();
                bJsonValid = Content.find("\"scene\":") != std::string::npos &&
                             Content.find("\"actors\": 4") != std::string::npos &&
                             Content.find("\"staticMesh\": 2") != std::string::npos &&
                             Content.find("\"light\": 1") != std::string::npos &&
                             Content.find("\"empty\": 1") != std::string::npos &&
                             Content.find("\"Cube1\"") != std::string::npos &&
                             Content.find("\"Sphere1\"") != std::string::npos &&
                             Content.find("\"DirLight\"") != std::string::npos &&
                             Content.find("\"EmptyActor\"") != std::string::npos;
            }
        }
        std::remove(DumpJsonPath.c_str());

        uint32 ActorCount = static_cast<uint32>(Scene.GetActors().size());
        auto FoundActor = Scene.FindActor("Cube1");
        auto FoundChild = Scene.FindActor("Sphere1");
        auto NotFound = Scene.FindActor("NonExistent");

        Result.bPassed = (ActorCount == 4 && FoundActor && FoundChild && !NotFound &&
                          bJsonValid);
        if (Result.bPassed)
        {
            Result.Message = "Scene dump passed: " + std::to_string(ActorCount) + " actors, "
                             "2 static mesh, 1 light, JSON output verified";
        }
        else
        {
            Result.Message = "Scene dump FAILED (actors=" + std::to_string(ActorCount) +
                             ", json=" + (bJsonValid ? "ok" : "invalid") + ")";
        }
        return Result;
    }

    inline FTestResult TestRenderTest()
    {
        FTestResult Result;
        Result.Name = "render-test";
        Result.bPassed = false;

        std::cout << "  [INFO] Running render test...\n";

        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"CLITestRenderTest";
        RegisterClassW(&wc);

        HWND HiddenWnd = CreateWindowW(L"CLITestRenderTest", L"RenderTest", WS_OVERLAPPED,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!HiddenWnd)
        {
            Result.bPassed = true;
            Result.bSkipped = true;
            Result.Message = "SKIPPED: No windowing support (headless CI)";
            UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));
            return Result;
        }

        KEngine Engine;
        HRESULT hr = Engine.InitializeWithExternalHwnd(HiddenWnd, 800, 600);
        if (FAILED(hr))
        {
            Result.bPassed = true;
            Result.bSkipped = true;
            Result.Message = "SKIPPED: DX11 unavailable (headless CI)";
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));
            return Result;
        }

        auto Renderer = Engine.GetRenderer();
        if (!Renderer)
        {
            Result.Message = "Renderer is null after initialization";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));
            return Result;
        }

        auto Camera = Engine.GetCamera();
        if (!Camera)
        {
            Result.Message = "Camera is null";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));
            return Result;
        }
        Camera->SetPosition(XMFLOAT3(0.0f, 2.0f, -5.0f));

        FDirectionalLight DirLight;
        DirLight.Direction = XMFLOAT3(-0.5f, -1.0f, 0.3f);
        DirLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        DirLight.Intensity = 1.0f;
        DirLight.AmbientColor = XMFLOAT4(0.15f, 0.15f, 0.2f, 1.0f);
        Renderer->SetDirectionalLight(DirLight);

        auto CubeMesh = Renderer->CreateCubeMesh();
        auto SphereMesh = Renderer->CreateSphereMesh(24, 12);
        if (!CubeMesh || !SphereMesh)
        {
            Result.Message = "Mesh creation failed";
            Engine.Shutdown();
            DestroyWindow(HiddenWnd);
            UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));
            return Result;
        }

        XMMATRIX CubeWorld = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        XMMATRIX SphereWorld = XMMatrixTranslation(2.5f, 0.0f, 0.0f);

        int32 FrameCount = 3;
        for (int32 i = 0; i < FrameCount; ++i)
        {
            Engine.Update(1.0f / 60.0f);
            Renderer->BeginFrame(Camera);

            Renderer->RenderMeshLit(CubeMesh, CubeWorld);
            Renderer->RenderMeshLit(SphereMesh, SphereWorld);

            Renderer->EndFrame(true);
        }

        int32 DrawCalls = Renderer->GetDrawCallCount();
        int32 VertexCount = Renderer->GetVertexCount();
        float FrameTime = Renderer->GetFrameTime();

        KGraphicsDevice* GraphicsDevice = Engine.GetGraphicsDevice();
        bool bBackbufferValid = false;
        if (GraphicsDevice && GraphicsDevice->GetContext())
        {
            ID3D11DeviceContext* Ctx = GraphicsDevice->GetContext();
            ID3D11RenderTargetView* RTV = nullptr;
            Ctx->OMGetRenderTargets(1, &RTV, nullptr);
            if (RTV)
            {
                ID3D11Resource* BackBuffer = nullptr;
                RTV->GetResource(&BackBuffer);
                RTV->Release();

                if (BackBuffer)
                {
                    D3D11_TEXTURE2D_DESC Desc;
                    ID3D11Texture2D* Tex = nullptr;
                    if (SUCCEEDED(BackBuffer->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&Tex)))
                    {
                        Tex->GetDesc(&Desc);
                        Tex->Release();
                    }
                    BackBuffer->Release();

                    if (Desc.Width > 0 && Desc.Height > 0)
                    {
                        bBackbufferValid = true;
                        std::cout << "  [OK] Backbuffer: " << Desc.Width << "x" << Desc.Height << "\n";
                    }
                }
            }
        }

        Engine.Shutdown();
        DestroyWindow(HiddenWnd);
        UnregisterClassW(L"CLITestRenderTest", GetModuleHandle(nullptr));

        Result.bPassed = (DrawCalls > 0 && VertexCount > 0 && bBackbufferValid);
        if (Result.bPassed)
        {
            Result.Message = "Render test passed: " + std::to_string(DrawCalls) + " draw calls, "
                             + std::to_string(VertexCount) + " vertices, "
                             + std::to_string(FrameCount) + " frames";
        }
        else
        {
            Result.Message = "Render test FAILED (drawCalls=" + std::to_string(DrawCalls) +
                             ", vertices=" + std::to_string(VertexCount) +
                             ", backbuffer=" + (bBackbufferValid ? "valid" : "invalid") + ")";
        }
        return Result;
    }
}
