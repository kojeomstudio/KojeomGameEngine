#include "Tests/TestFramework.h"
#include "Math/Transform.h"
#include "Core/FileSystem.h"
#include "Core/StringId.h"
#include "Core/Log.h"
#include "Core/AppConfig.h"
#include "Renderer/RenderScene.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimationClip.h"
#include "Assets/AssetStore.h"
#include "World/World.h"
#include "World/NativeBehavior.h"
#include "Platform/IInput.h"

using namespace Kojeom;

KE_TEST(TransformIdentityMatrix)
{
    auto identity = Transform::Identity();
    Mat4 mat = identity.ToMatrix();
    KE_EXPECT_FLOAT_EQ(mat[0][0], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[1][1], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[2][2], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[3][3], 1.0f, 0.0001f);
    return {};
}

KE_TEST(TransformTranslation)
{
    Transform t;
    t.position = Vec3(1.0f, 2.0f, 3.0f);
    Mat4 mat = t.ToMatrix();
    KE_EXPECT_FLOAT_EQ(mat[3][0], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[3][1], 2.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[3][2], 3.0f, 0.0001f);
    return {};
}

KE_TEST(TransformScale)
{
    Transform t;
    t.scale = Vec3(2.0f, 3.0f, 4.0f);
    Mat4 mat = t.ToMatrix();
    KE_EXPECT_FLOAT_EQ(mat[0][0], 2.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[1][1], 3.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[2][2], 4.0f, 0.0001f);
    return {};
}

KE_TEST(CameraDataViewProjection)
{
    CameraData cam;
    cam.position = Vec3(0.0f, 0.0f, 5.0f);
    cam.forward = Vec3(0.0f, 0.0f, -1.0f);
    cam.up = Vec3(0.0f, 1.0f, 0.0f);
    cam.fov = 60.0f;
    cam.aspectRatio = 16.0f / 9.0f;
    cam.nearPlane = 0.1f;
    cam.farPlane = 100.0f;
    cam.UpdateMatrices();

    Vec4 origin = cam.viewProjectionMatrix * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    KE_EXPECT(origin.w != 0.0f);

    KE_EXPECT_FLOAT_EQ(cam.right.x, 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(cam.right.y, 0.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(cam.right.z, 0.0f, 0.0001f);
    return {};
}

KE_TEST(FilePathValidation)
{
    KE_EXPECT_FALSE(FileSystem::ValidatePath(""));
    KE_EXPECT_FALSE(FileSystem::ValidatePath(".."));
    KE_EXPECT_FALSE(FileSystem::ValidatePath("../secret.txt"));
    KE_EXPECT_FALSE(FileSystem::ValidatePath("foo/../../bar.txt"));
    KE_EXPECT_FALSE(FileSystem::ValidatePath("%2e%2e/foo.txt"));
    KE_EXPECT_TRUE(FileSystem::ValidatePath("Textures/Brick.png"));
    KE_EXPECT_TRUE(FileSystem::ValidatePath("Meshes/Cube.obj"));
    KE_EXPECT_FALSE(FileSystem::ValidatePath(std::string(5000, 'a')));
    return {};
}

KE_TEST(FilePathExtension)
{
    KE_EXPECT(FileSystem::GetExtension("model.obj") == ".obj");
    KE_EXPECT(FileSystem::GetExtension("texture.png") == ".png");
    KE_EXPECT(FileSystem::GetExtension("scene") == "");
    KE_EXPECT(FileSystem::GetExtension("file.tar.gz") == ".gz");
    return {};
}

KE_TEST(FilePathDirectory)
{
    KE_EXPECT(FileSystem::GetDirectory("Assets/Models/cube.obj") == "Assets/Models/");
    KE_EXPECT(FileSystem::GetDirectory("cube.obj") == "./");
    return {};
}

KE_TEST(FileNameExtraction)
{
    KE_EXPECT(FileSystem::GetFileName("Assets/Models/cube.obj") == "cube.obj");
    KE_EXPECT(FileSystem::GetFileName("cube.obj") == "cube.obj");
    return {};
}

KE_TEST(PathNormalization)
{
    KE_EXPECT(FileSystem::NormalizePath("Assets\\Models\\cube.obj") == "Assets/Models/cube.obj");
    KE_EXPECT(FileSystem::NormalizePath("Assets//Models///cube.obj") == "Assets/Models/cube.obj");
    return {};
}

KE_TEST(StringIdBasic)
{
    StringId a("hello");
    StringId b("hello");
    StringId c("world");

    KE_EXPECT(a == b);
    KE_EXPECT(a != c);
    KE_EXPECT(a.GetString() == "hello");
    KE_EXPECT(c.GetString() == "world");
    return {};
}

KE_TEST(StringIdBoolConversion)
{
    StringId empty;
    StringId valid("test");
    KE_EXPECT_FALSE(static_cast<bool>(empty));
    KE_EXPECT_TRUE(static_cast<bool>(valid));
    return {};
}

KE_TEST(RenderSceneSize)
{
    RenderScene scene;
    KE_EXPECT_TRUE(scene.staticDrawCommands.empty());
    KE_EXPECT_TRUE(scene.skinnedDrawCommands.empty());
    KE_EXPECT_TRUE(scene.terrainDrawCommands.empty());
    KE_EXPECT_FLOAT_EQ(scene.TotalDrawCommands(), 0.0f, 0.0001f);

    scene.staticDrawCommands.resize(3);
    scene.terrainDrawCommands.resize(2);
    KE_EXPECT_FLOAT_EQ(scene.TotalDrawCommands(), 5.0f, 0.0001f);
    return {};
}

KE_TEST(AppConfigModeConversion)
{
    KE_EXPECT(AppConfig::ModeToString(AppConfig::Mode::Game) == "game");
    KE_EXPECT(AppConfig::ModeToString(AppConfig::Mode::Smoke) == "smoke");
    KE_EXPECT(AppConfig::ModeToString(AppConfig::Mode::ValidateAssets) == "validate-assets");
    KE_EXPECT(AppConfig::ModeToString(AppConfig::Mode::RenderTest) == "render-test");
    KE_EXPECT(AppConfig::ModeToString(AppConfig::Mode::SceneDump) == "scene-dump");

    KE_EXPECT(AppConfig::StringToMode("game") == AppConfig::Mode::Game);
    KE_EXPECT(AppConfig::StringToMode("smoke") == AppConfig::Mode::Smoke);
    KE_EXPECT(AppConfig::StringToMode("validate-assets") == AppConfig::Mode::ValidateAssets);
    KE_EXPECT(AppConfig::StringToMode("unknown") == AppConfig::Mode::Game);
    return {};
}

KE_TEST(LightDataDefaults)
{
    LightData light;
    KE_EXPECT_FLOAT_EQ(light.intensity, 1.0f, 0.0001f);
    KE_EXPECT(light.pointLightCount == 0);
    KE_EXPECT_FLOAT_EQ(light.ambientColor.x, 0.1f, 0.0001f);
    return {};
}

KE_TEST(DrawCommandsDefault)
{
    StaticDrawCommand cmd;
    KE_EXPECT(cmd.meshHandle == INVALID_HANDLE);
    KE_EXPECT(cmd.materialHandle == INVALID_HANDLE);
    KE_EXPECT_FLOAT_EQ(cmd.boundsRadius, 0.0f, 0.0001f);

    SkinnedDrawCommand skinned;
    KE_EXPECT(skinned.skeletalMeshHandle == INVALID_HANDLE);
    KE_EXPECT(skinned.boneCount == 0);

    TerrainDrawCommand terrain;
    KE_EXPECT(terrain.terrainHandle == INVALID_HANDLE);
    return {};
}

KE_TEST(SkeletonCreateAndValidate)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Bone child;
    child.name = "Child";
    child.parentIndex = 0;
    child.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(child);

    KE_EXPECT(skeleton.GetBoneCount() == 2);
    KE_EXPECT(skeleton.FindBoneIndex("Root") == 0);
    KE_EXPECT(skeleton.FindBoneIndex("Child") == 1);
    KE_EXPECT(skeleton.FindBoneIndex("Missing") == -1);
    KE_EXPECT(skeleton.Validate());
    return {};
}

KE_TEST(SkeletonInvalidParent)
{
    Skeleton skeleton;
    Bone bad;
    bad.name = "Bad";
    bad.parentIndex = 5;
    skeleton.AddBone(bad);
    KE_EXPECT_FALSE(skeleton.Validate());
    return {};
}

KE_TEST(BoneTransformIdentity)
{
    BoneTransform bt;
    Mat4 mat = bt.ToMatrix();
    KE_EXPECT_FLOAT_EQ(mat[0][0], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[1][1], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[2][2], 1.0f, 0.0001f);
    KE_EXPECT_FLOAT_EQ(mat[3][3], 1.0f, 0.0001f);
    return {};
}

KE_TEST(BoneTransformLerp)
{
    BoneTransform a;
    a.position = Vec3(0.0f);
    a.rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    a.scale = Vec3(1.0f);

    BoneTransform b;
    b.position = Vec3(10.0f, 0.0f, 0.0f);
    b.rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    b.scale = Vec3(2.0f);

    BoneTransform mid = BoneTransform::Lerp(a, b, 0.5f);
    KE_EXPECT_FLOAT_EQ(mid.position.x, 5.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mid.position.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mid.scale.x, 1.5f, 0.001f);
    return {};
}

KE_TEST(PoseComputeGlobalTransforms)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Bone child;
    child.name = "Child";
    child.parentIndex = 0;
    child.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(child);

    Pose pose(2);
    pose.GetLocalTransform(0).position = Vec3(1.0f, 0.0f, 0.0f);
    pose.GetLocalTransform(1).position = Vec3(2.0f, 0.0f, 0.0f);

    pose.ComputeGlobalTransforms(skeleton);

    KE_EXPECT_FLOAT_EQ(pose.GetGlobalTransform(0).position.x, 1.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(pose.GetGlobalTransform(1).position.x, 3.0f, 0.001f);
    return {};
}

KE_TEST(PoseComputeSkinMatrices)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Pose pose(1);
    pose.GetLocalTransform(0).position = Vec3(0.0f);
    pose.GetLocalTransform(0).scale = Vec3(1.0f);
    pose.GetLocalTransform(0).rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    pose.ComputeGlobalTransforms(skeleton);

    auto matrices = pose.ComputeSkinMatrices(skeleton);
    KE_EXPECT(matrices.size() == 1);
    KE_EXPECT_FLOAT_EQ(matrices[0][0][0], 1.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(matrices[0][1][1], 1.0f, 0.001f);
    return {};
}

KE_TEST(AnimationClipSampleAtZero)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    AnimationClip clip;
    clip.SetName("TestClip");
    clip.SetDuration(1.0f);
    clip.SetTicksPerSecond(1.0f);

    AnimationChannel channel;
    channel.boneIndex = 0;
    channel.boneName = "Root";
    VectorKey posKey;
    posKey.time = 0.0f;
    posKey.value = Vec3(1.0f, 2.0f, 3.0f);
    channel.positionKeys.push_back(posKey);
    VectorKey posKey2;
    posKey2.time = 1.0f;
    posKey2.value = Vec3(4.0f, 5.0f, 6.0f);
    channel.positionKeys.push_back(posKey2);
    clip.AddChannel(channel);

    Pose pose = clip.Sample(0.0f, skeleton);
    KE_EXPECT_FLOAT_EQ(pose.GetLocalTransform(0).position.x, 1.0f, 0.01f);
    KE_EXPECT_FLOAT_EQ(pose.GetLocalTransform(0).position.y, 2.0f, 0.01f);
    KE_EXPECT_FLOAT_EQ(pose.GetLocalTransform(0).position.z, 3.0f, 0.01f);
    return {};
}

KE_TEST(AnimationClipSampleInterpolated)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    AnimationClip clip;
    clip.SetDuration(1.0f);
    clip.SetTicksPerSecond(1.0f);

    AnimationChannel channel;
    channel.boneIndex = 0;
    channel.boneName = "Root";
    VectorKey k0;
    k0.time = 0.0f;
    k0.value = Vec3(0.0f);
    channel.positionKeys.push_back(k0);
    VectorKey k1;
    k1.time = 1.0f;
    k1.value = Vec3(10.0f, 0.0f, 0.0f);
    channel.positionKeys.push_back(k1);
    clip.AddChannel(channel);

    Pose pose = clip.Sample(0.5f, skeleton);
    KE_EXPECT_FLOAT_EQ(pose.GetLocalTransform(0).position.x, 5.0f, 0.01f);
    return {};
}

KE_TEST(AnimationClipValidateIssues)
{
    AnimationClip clip;
    clip.SetName("BadClip");
    clip.SetDuration(-1.0f);
    clip.SetTicksPerSecond(0.0f);

    auto issues = clip.Validate(10);
    bool hasNegativeDuration = false;
    bool hasInvalidTPS = false;
    for (const auto& issue : issues)
    {
        if (issue.find("negative duration") != std::string::npos)
            hasNegativeDuration = true;
        if (issue.find("invalid ticks") != std::string::npos)
            hasInvalidTPS = true;
    }
    KE_EXPECT_TRUE(hasNegativeDuration);
    KE_EXPECT_TRUE(hasInvalidTPS);
    return {};
}

KE_TEST(AnimationClipEventsInRange)
{
    AnimationClip clip;
    clip.SetDuration(2.0f);
    clip.SetTicksPerSecond(1.0f);

    AnimationEvent evt1;
    evt1.time = 0.5f;
    evt1.name = "Step";
    clip.AddEvent(evt1);

    AnimationEvent evt2;
    evt2.time = 1.5f;
    evt2.name = "Jump";
    clip.AddEvent(evt2);

    auto triggered = clip.GetEventsInRange(0.0f, 1.0f);
    KE_EXPECT_TRUE(triggered.size() == 1);
    KE_EXPECT(triggered[0].name == "Step");

    auto triggered2 = clip.GetEventsInRange(1.0f, 2.0f);
    KE_EXPECT_TRUE(triggered2.size() == 1);
    KE_EXPECT(triggered2[0].name == "Jump");

    auto triggered3 = clip.GetEventsInRange(0.0f, 2.0f);
    KE_EXPECT_TRUE(triggered3.size() == 2);
    return {};
}

KE_TEST(AnimatorBasicPlayback)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    AnimationClip clip;
    clip.SetDuration(1.0f);
    clip.SetTicksPerSecond(1.0f);

    AnimationChannel channel;
    channel.boneIndex = 0;
    channel.boneName = "Root";
    VectorKey k0;
    k0.time = 0.0f;
    k0.value = Vec3(0.0f);
    channel.positionKeys.push_back(k0);
    VectorKey k1;
    k1.time = 1.0f;
    k1.value = Vec3(10.0f, 0.0f, 0.0f);
    channel.positionKeys.push_back(k1);
    clip.AddChannel(channel);

    Animator animator;
    animator.SetSkeleton(&skeleton);
    animator.SetClip(&clip);
    animator.SetLoop(true);
    animator.Play();

    KE_EXPECT_TRUE(animator.IsPlaying());

    animator.Tick(0.5f);
    KE_EXPECT_FLOAT_EQ(animator.GetPlaybackTime(), 0.5f, 0.001f);

    animator.Tick(0.5f);
    KE_EXPECT_TRUE(animator.IsPlaying());

    auto matrices = animator.GetSkinMatrices();
    KE_EXPECT_TRUE(matrices.size() == 1);
    return {};
}

KE_TEST(AnimatorPauseStop)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    skeleton.AddBone(root);

    AnimationClip clip;
    clip.SetDuration(2.0f);
    clip.SetTicksPerSecond(1.0f);

    Animator animator;
    animator.SetSkeleton(&skeleton);
    animator.SetClip(&clip);
    animator.Play();

    animator.Tick(0.5f);
    KE_EXPECT_FLOAT_EQ(animator.GetPlaybackTime(), 0.5f, 0.001f);

    animator.Pause();
    animator.Tick(0.5f);
    KE_EXPECT_FLOAT_EQ(animator.GetPlaybackTime(), 0.5f, 0.001f);

    animator.Stop();
    KE_EXPECT_FLOAT_EQ(animator.GetPlaybackTime(), 0.0f, 0.001f);
    KE_EXPECT_FALSE(animator.IsPlaying());
    return {};
}

KE_TEST(BlendTree1DSampling)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    AnimationClip walkClip;
    walkClip.SetDuration(1.0f);
    walkClip.SetTicksPerSecond(1.0f);
    AnimationChannel walkCh;
    walkCh.boneIndex = 0;
    walkCh.boneName = "Root";
    VectorKey wk0;
    wk0.time = 0.0f;
    wk0.value = Vec3(0.0f);
    walkCh.positionKeys.push_back(wk0);
    VectorKey wk1;
    wk1.time = 1.0f;
    wk1.value = Vec3(1.0f, 0.0f, 0.0f);
    walkCh.positionKeys.push_back(wk1);
    walkClip.AddChannel(walkCh);

    AnimationClip runClip;
    runClip.SetDuration(1.0f);
    runClip.SetTicksPerSecond(1.0f);
    AnimationChannel runCh;
    runCh.boneIndex = 0;
    runCh.boneName = "Root";
    VectorKey rk0;
    rk0.time = 0.0f;
    rk0.value = Vec3(0.0f);
    runCh.positionKeys.push_back(rk0);
    VectorKey rk1;
    rk1.time = 1.0f;
    rk1.value = Vec3(3.0f, 0.0f, 0.0f);
    runCh.positionKeys.push_back(rk1);
    runClip.AddChannel(runCh);

    BlendTree blendTree;
    blendTree.SetSkeleton(&skeleton);
    blendTree.AddNode(&walkClip, 0.0f);
    blendTree.AddNode(&runClip, 1.0f);
    blendTree.Tick(0.5f);

    Pose midPose = blendTree.Sample(0.5f);
    float posX = midPose.GetLocalTransform(0).position.x;
    KE_EXPECT_TRUE(posX >= 0.4f && posX <= 1.8f);
    return {};
}

KE_TEST(TerrainDataHeightQuery)
{
    TerrainData terrain;
    terrain.width = 4;
    terrain.height = 4;
    terrain.cellSize = 1.0f;
    terrain.heightmap = {0, 1, 2, 3,
                          1, 2, 3, 4,
                          2, 3, 4, 5,
                          3, 4, 5, 6};

    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(0, 0), 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(3, 3), 6.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(1, 1), 2.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(-1, 0), 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(10, 10), 0.0f, 0.001f);
    return {};
}

KE_TEST(TerrainDataInterpolatedHeight)
{
    TerrainData terrain;
    terrain.width = 2;
    terrain.height = 2;
    terrain.cellSize = 1.0f;
    terrain.heightmap = {0.0f, 0.0f,
                          0.0f, 4.0f};

    float hMid = terrain.GetHeightInterpolated(0.5f, 0.5f);
    KE_EXPECT_TRUE(hMid >= 0.0f && hMid <= 4.0f);
    return {};
}

KE_TEST(AssetStoreCreateTerrain)
{
    AssetStore store;
    std::vector<float> heights(4, 1.0f);
    AssetHandle handle = store.CreateTerrain("test", 2, 2, 1.0f, 5.0f, heights);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);

    const TerrainData* terrain = store.GetTerrain(handle);
    KE_EXPECT_TRUE(terrain != nullptr);
    KE_EXPECT(terrain->width == 2);
    KE_EXPECT(terrain->height == 2);
    KE_EXPECT_FLOAT_EQ(terrain->GetHeight(0, 0), 1.0f, 0.001f);
    return {};
}

KE_TEST(AssetStoreRegisterMaterial)
{
    AssetStore store;
    MaterialData mat;
    mat.albedo = Vec3(0.8f, 0.3f, 0.2f);
    mat.metallic = 0.5f;
    mat.roughness = 0.4f;
    AssetHandle handle = store.RegisterMaterial(mat);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);

    const MaterialData* retrieved = store.GetMaterial(handle);
    KE_EXPECT_TRUE(retrieved != nullptr);
    KE_EXPECT_FLOAT_EQ(retrieved->albedo.x, 0.8f, 0.001f);
    KE_EXPECT_FLOAT_EQ(retrieved->metallic, 0.5f, 0.001f);
    KE_EXPECT_FLOAT_EQ(retrieved->roughness, 0.4f, 0.001f);
    return {};
}

KE_TEST(AssetStoreStats)
{
    AssetStore store;
    MaterialData mat;
    store.RegisterMaterial(mat);
    store.RegisterMaterial(mat);

    auto stats = store.GetStats();
    KE_EXPECT_TRUE(stats.materialCount == 2);
    return {};
}

KE_TEST(AssetStoreUnloadMesh)
{
    AssetStore store;
    MeshData mesh;
    mesh.name = "test";
    mesh.vertices.resize(3);
    mesh.indices = {0, 1, 2};
    mesh.boundsMin = Vec3(-1.0f);
    mesh.boundsMax = Vec3(1.0f);
    AssetHandle handle = store.RegisterInternalMesh("test_mesh", mesh);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);

    KE_EXPECT_TRUE(store.GetMesh(handle) != nullptr);
    KE_EXPECT_TRUE(store.UnloadMesh(handle));
    KE_EXPECT_TRUE(store.GetMesh(handle) == nullptr);
    return {};
}

KE_TEST(AssetStoreValidateAllAssets)
{
    AssetStore store;
    MeshData goodMesh;
    goodMesh.vertices.resize(3);
    goodMesh.indices = {0, 1, 2};
    store.RegisterInternalMesh("good", goodMesh);

    auto issues = store.ValidateAllAssets();
    KE_EXPECT_TRUE(issues.empty());
    return {};
}

KE_TEST(AnimationStateMachineTransition)
{
    Skeleton skeleton;
    Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    AnimationClip idleClip;
    idleClip.SetDuration(1.0f);
    idleClip.SetTicksPerSecond(1.0f);
    AnimationChannel idleCh;
    idleCh.boneIndex = 0;
    idleCh.boneName = "Root";
    VectorKey ik;
    ik.time = 0.0f;
    ik.value = Vec3(0.0f);
    idleCh.positionKeys.push_back(ik);
    idleClip.AddChannel(idleCh);

    AnimationClip walkClip;
    walkClip.SetDuration(1.0f);
    walkClip.SetTicksPerSecond(1.0f);
    AnimationChannel walkCh;
    walkCh.boneIndex = 0;
    walkCh.boneName = "Root";
    VectorKey wk;
    wk.time = 0.0f;
    wk.value = Vec3(1.0f, 0.0f, 0.0f);
    walkCh.positionKeys.push_back(wk);
    walkClip.AddChannel(walkCh);

    AnimationStateMachine sm;
    sm.SetSkeleton(&skeleton);
    sm.AddState("Idle", &idleClip, true, 1.0f);
    sm.AddState("Walk", &walkClip, true, 1.0f);
    sm.AddTransition("Idle", "Walk", "start_walking", 0.25f);

    KE_EXPECT_TRUE(sm.IsInState("Idle"));

    sm.SetTrigger("start_walking");
    sm.Tick(0.1f);
    KE_EXPECT_TRUE(sm.IsInState("Walk"));
    return {};
}

KE_TEST(RootMotionDataDefaults)
{
    RootMotionData rm;
    KE_EXPECT_FLOAT_EQ(rm.deltaPosition.x, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(rm.deltaPosition.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(rm.deltaPosition.z, 0.0f, 0.001f);
    KE_EXPECT_FALSE(rm.hasRootMotion);
    return {};
}

KE_TEST(AnimationEventDefaults)
{
    AnimationEvent evt;
    KE_EXPECT_FLOAT_EQ(evt.time, 0.0f, 0.001f);
    KE_EXPECT_TRUE(evt.name.empty());
    KE_EXPECT_TRUE(evt.stringValue.empty());
    KE_EXPECT(evt.intValue == 0);
    KE_EXPECT_FLOAT_EQ(evt.floatValue, 0.0f, 0.001f);
    return {};
}

KE_TEST(EntityCreationAndName)
{
    Kojeom::Entity entity(1, "TestEntity");
    KE_EXPECT(entity.GetId() == 1);
    KE_EXPECT(entity.GetName() == "TestEntity");
    entity.SetName("RenamedEntity");
    KE_EXPECT(entity.GetName() == "RenamedEntity");
    return {};
}

KE_TEST(EntityTransformDefault)
{
    Kojeom::Entity entity(1);
    auto* transform = entity.GetTransform();
    KE_EXPECT(transform != nullptr);
    KE_EXPECT_FLOAT_EQ(transform->transform.position.x, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(transform->transform.position.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(transform->transform.position.z, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(transform->transform.scale.x, 1.0f, 0.001f);
    return {};
}

KE_TEST(EntityAddComponent)
{
    Kojeom::Entity entity(1);
    auto* cam = entity.AddComponent<Kojeom::CameraComponent>();
    KE_EXPECT(cam != nullptr);
    KE_EXPECT(entity.GetCameraComponent() == cam);

    auto* mr = entity.AddComponent<Kojeom::MeshRendererComponent>(42, 10);
    KE_EXPECT(mr != nullptr);
    KE_EXPECT(mr->meshHandle == 42);
    KE_EXPECT(mr->materialHandle == 10);
    KE_EXPECT(entity.GetMeshRendererComponent() == mr);
    return {};
}

KE_TEST(EntityComponentNotFound)
{
    Kojeom::Entity entity(1);
    KE_EXPECT(entity.GetCameraComponent() == nullptr);
    KE_EXPECT(entity.GetMeshRendererComponent() == nullptr);
    KE_EXPECT(entity.GetTerrainComponent() == nullptr);
    KE_EXPECT(entity.GetSkeletalMeshComponent() == nullptr);
    KE_EXPECT(entity.GetAnimatorComponent() == nullptr);
    return {};
}

KE_TEST(EntityHierarchy)
{
    Kojeom::Entity parent(1, "Parent");
    Kojeom::Entity child(2, "Child");

    child.SetParent(1);
    parent.AddChild(2);

    KE_EXPECT(child.HasParent());
    KE_EXPECT(child.GetParentId() == 1);
    KE_EXPECT(parent.HasChildren());
    KE_EXPECT(parent.GetChildren().size() == 1);
    KE_EXPECT(parent.GetChildren()[0] == 2);

    parent.RemoveChild(2);
    KE_EXPECT(!parent.HasChildren());
    return {};
}

KE_TEST(EntityWorldMatrix)
{
    Kojeom::Entity entity(1);
    entity.GetTransform()->transform.position = Vec3(5.0f, 3.0f, 1.0f);

    std::unordered_map<Kojeom::EntityId, Kojeom::Entity*> entityMap;
    entityMap[1] = &entity;

    Mat4 worldMat = entity.GetWorldMatrix(entityMap);
    KE_EXPECT_FLOAT_EQ(worldMat[3][0], 5.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(worldMat[3][1], 3.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(worldMat[3][2], 1.0f, 0.001f);
    return {};
}

KE_TEST(WorldCreateEntity)
{
    Kojeom::World world;
    auto* e1 = world.CreateEntity("Entity1");
    auto* e2 = world.CreateEntity("Entity2");
    KE_EXPECT(e1 != nullptr);
    KE_EXPECT(e2 != nullptr);
    KE_EXPECT(e1->GetId() != e2->GetId());
    KE_EXPECT(world.GetEntityCount() == 2);
    KE_EXPECT(world.GetEntity(e1->GetId()) == e1);
    KE_EXPECT(world.GetEntity(e2->GetId()) == e2);
    return {};
}

KE_TEST(WorldRemoveEntity)
{
    Kojeom::World world;
    auto* e1 = world.CreateEntity("Entity1");
    auto* e2 = world.CreateEntity("Entity2");
    world.RemoveEntity(e1->GetId());
    KE_EXPECT(world.GetEntityCount() == 1);
    KE_EXPECT(world.GetEntity(e1->GetId()) == nullptr);
    KE_EXPECT(world.GetEntity(e2->GetId()) == e2);
    return {};
}

KE_TEST(WorldRemoveAllEntities)
{
    Kojeom::World world;
    world.CreateEntity("A");
    world.CreateEntity("B");
    world.CreateEntity("C");
    KE_EXPECT(world.GetEntityCount() == 3);
    world.RemoveAllEntities();
    KE_EXPECT(world.GetEntityCount() == 0);
    return {};
}

KE_TEST(WorldAttachChild)
{
    Kojeom::World world;
    auto* parent = world.CreateEntity("Parent");
    auto* child = world.CreateEntity("Child");
    bool result = world.AttachChild(parent->GetId(), child->GetId());
    KE_EXPECT_TRUE(result);
    KE_EXPECT(child->HasParent());
    KE_EXPECT(child->GetParentId() == parent->GetId());
    KE_EXPECT(parent->HasChildren());
    return {};
}

KE_TEST(WorldAttachChildSelfFails)
{
    Kojeom::World world;
    auto* entity = world.CreateEntity("Self");
    bool result = world.AttachChild(entity->GetId(), entity->GetId());
    KE_EXPECT_FALSE(result);
    return {};
}

KE_TEST(WorldDetachFromParent)
{
    Kojeom::World world;
    auto* parent = world.CreateEntity("Parent");
    auto* child = world.CreateEntity("Child");
    world.AttachChild(parent->GetId(), child->GetId());
    KE_EXPECT_TRUE(child->HasParent());
    world.DetachFromParent(child->GetId());
    KE_EXPECT_FALSE(child->HasParent());
    KE_EXPECT_FALSE(parent->HasChildren());
    return {};
}

KE_TEST(WorldBuildRenderSceneEmpty)
{
    Kojeom::World world;
    Kojeom::RenderScene scene = world.BuildRenderScene();
    KE_EXPECT_TRUE(scene.staticDrawCommands.empty());
    KE_EXPECT_TRUE(scene.skinnedDrawCommands.empty());
    KE_EXPECT_TRUE(scene.terrainDrawCommands.empty());
    return {};
}

KE_TEST(WorldBuildRenderSceneWithMesh)
{
    Kojeom::World world;
    auto* entity = world.CreateEntity("MeshEntity");
    auto* mr = entity->AddComponent<Kojeom::MeshRendererComponent>(100, 200);
    KE_EXPECT(mr != nullptr);

    Kojeom::RenderScene scene = world.BuildRenderScene();
    KE_EXPECT_TRUE(scene.staticDrawCommands.size() == 1);
    KE_EXPECT(scene.staticDrawCommands[0].meshHandle == 100);
    KE_EXPECT(scene.staticDrawCommands[0].materialHandle == 200);
    return {};
}

KE_TEST(WorldBuildRenderSceneWithCamera)
{
    Kojeom::World world;
    auto* camEntity = world.CreateEntity("Camera");
    auto* cam = camEntity->AddComponent<Kojeom::CameraComponent>();
    cam->isActive = true;
    cam->cameraData.position = Vec3(0.0f, 5.0f, 10.0f);
    cam->cameraData.forward = Vec3(0.0f, 0.0f, -1.0f);
    cam->cameraData.up = Vec3(0.0f, 1.0f, 0.0f);
    cam->cameraData.fov = 60.0f;
    cam->cameraData.aspectRatio = 16.0f / 9.0f;
    cam->cameraData.nearPlane = 0.1f;
    cam->cameraData.farPlane = 100.0f;
    cam->cameraData.UpdateMatrices();

    Kojeom::RenderScene scene = world.BuildRenderScene();
    KE_EXPECT_FLOAT_EQ(scene.camera.position.x, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(scene.camera.position.y, 5.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(scene.camera.position.z, 10.0f, 0.001f);
    return {};
}

KE_TEST(WorldTick)
{
    Kojeom::World world;
    auto* entity = world.CreateEntity("Entity");
    auto* cam = entity->AddComponent<Kojeom::CameraComponent>();
    cam->isActive = true;
    cam->cameraData.fov = 60.0f;
    cam->cameraData.aspectRatio = 1.0f;
    cam->cameraData.nearPlane = 0.1f;
    cam->cameraData.farPlane = 100.0f;
    cam->cameraData.position = Vec3(0.0f);
    cam->cameraData.forward = Vec3(0.0f, 0.0f, -1.0f);
    cam->cameraData.up = Vec3(0.0f, 1.0f, 0.0f);

    world.Tick(0.016f);
    KE_EXPECT(cam->cameraData.viewMatrix[0][0] != 0.0f || cam->cameraData.viewMatrix[0][1] != 0.0f);
    return {};
}

KE_TEST(NativeBehaviorLifecycle)
{
    Kojeom::Entity entity(1);
    bool started = false;
    bool updated = false;
    bool stopped = false;

    Kojeom::NativeBehavior behavior;
    behavior.behaviorName = "TestBehavior";
    behavior.onStart = [&](Kojeom::Entity*, Kojeom::Engine*) { started = true; };
    behavior.onUpdate = [&](Kojeom::Entity*, Kojeom::Engine*, float) { updated = true; };
    behavior.onStop = [&](Kojeom::Entity*, Kojeom::Engine*) { stopped = true; };

    auto& behaviors = entity.GetBehaviorComponent();
    behaviors.AddBehavior(behavior);

    KE_EXPECT_FALSE(started);
    KE_EXPECT_FALSE(updated);

    behaviors.TickBehaviors(&entity, nullptr, 0.016f);
    KE_EXPECT_TRUE(started);
    KE_EXPECT_TRUE(updated);
    KE_EXPECT_FALSE(stopped);

    behaviors.StopBehaviors(&entity, nullptr);
    KE_EXPECT_TRUE(stopped);
    return {};
}

KE_TEST(NativeBehaviorCreateSimple)
{
    int callCount = 0;
    auto behavior = Kojeom::NativeBehavior::CreateSimple("Simple",
        [&](Kojeom::Entity*, Kojeom::Engine*, float) { callCount++; });
    KE_EXPECT(behavior.behaviorName == "Simple");
    KE_EXPECT_TRUE(behavior.onUpdate != nullptr);
    KE_EXPECT_TRUE(behavior.onStart == nullptr);
    KE_EXPECT_TRUE(behavior.onStop == nullptr);
    return {};
}

KE_TEST(MeshDataBounds)
{
    Kojeom::MeshData mesh;
    mesh.boundsMin = Vec3(-1.0f, -2.0f, -3.0f);
    mesh.boundsMax = Vec3(1.0f, 2.0f, 3.0f);
    Vec3 center = (mesh.boundsMin + mesh.boundsMax) * 0.5f;
    KE_EXPECT_FLOAT_EQ(center.x, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(center.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(center.z, 0.0f, 0.001f);
    float radius = glm::length(mesh.boundsMax - mesh.boundsMin) * 0.5f;
    KE_EXPECT(radius > 0.0f);
    return {};
}

KE_TEST(SkinnedVertexDefaults)
{
    Kojeom::SkinnedVertex sv;
    KE_EXPECT_FLOAT_EQ(sv.boneWeights.x, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(sv.boneWeights.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(sv.boneWeights.z, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(sv.boneWeights.w, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(sv.boneIndices.x, 0.0f, 0.001f);
    return {};
}

KE_TEST(MaterialDataDefaults)
{
    Kojeom::MaterialData mat;
    KE_EXPECT_FLOAT_EQ(mat.albedo.x, 0.8f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mat.metallic, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mat.roughness, 0.5f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mat.ao, 1.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(mat.emissiveStrength, 1.0f, 0.001f);
    KE_EXPECT_TRUE(mat.albedoTexturePath.empty());
    KE_EXPECT_TRUE(mat.normalTexturePath.empty());
    return {};
}

KE_TEST(AssetStoreRegisterInternalMesh)
{
    Kojeom::AssetStore store;
    Kojeom::MeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices = {0, 1, 2};
    mesh.boundsMin = Vec3(-1.0f);
    mesh.boundsMax = Vec3(1.0f);
    mesh.name = "test_cube";
    AssetHandle handle = store.RegisterInternalMesh("test_cube", mesh);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);
    KE_EXPECT_TRUE(store.GetMesh(handle) != nullptr);
    KE_EXPECT(store.GetMesh(handle)->name == "test_cube");
    return {};
}

KE_TEST(AssetStoreRegisterSkinnedMesh)
{
    Kojeom::AssetStore store;
    Kojeom::SkinnedMeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices = {0, 1, 2};
    mesh.boundsMin = Vec3(-1.0f);
    mesh.boundsMax = Vec3(1.0f);
    mesh.name = "skinned_test";
    AssetHandle handle = store.RegisterSkinnedMesh("skinned_test", mesh);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);
    KE_EXPECT_TRUE(store.GetSkinnedMesh(handle) != nullptr);
    return {};
}

KE_TEST(AssetStoreRegisterSkeleton)
{
    Kojeom::AssetStore store;
    Kojeom::SkeletonData skelData;
    Kojeom::Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skelData.skeleton.AddBone(root);
    skelData.name = "TestSkel";
    AssetHandle handle = store.RegisterSkeleton("test_skel", skelData);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);
    KE_EXPECT_TRUE(store.GetSkeleton(handle) != nullptr);
    KE_EXPECT(store.GetSkeleton(handle)->skeleton.GetBoneCount() == 1);
    return {};
}

KE_TEST(AssetStoreRegisterAnimationClip)
{
    Kojeom::AssetStore store;
    Kojeom::AnimationClipData clipData;
    clipData.clip.SetName("Walk");
    clipData.clip.SetDuration(2.0f);
    clipData.clip.SetTicksPerSecond(30.0f);
    clipData.name = "WalkClip";
    AssetHandle handle = store.RegisterAnimationClip("walk_clip", clipData);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);
    KE_EXPECT_TRUE(store.GetAnimationClip(handle) != nullptr);
    KE_EXPECT(store.GetAnimationClip(handle)->clip.GetName() == "Walk");
    return {};
}

KE_TEST(AssetStoreTextureRegistration)
{
    Kojeom::AssetStore store;
    Kojeom::TextureData texData;
    texData.width = 4;
    texData.height = 4;
    texData.channels = 4;
    texData.pixels.resize(4 * 4 * 4, 255);
    texData.path = "test_texture";
    AssetHandle handle = store.RegisterTexture("test_texture", texData);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);
    KE_EXPECT_TRUE(store.GetTexture(handle) != nullptr);
    KE_EXPECT(store.GetTexture(handle)->width == 4);
    KE_EXPECT(store.GetTexture(handle)->height == 4);
    return {};
}

KE_TEST(AssetStoreUnloadNonExistent)
{
    Kojeom::AssetStore store;
    KE_EXPECT_FALSE(store.UnloadMesh(99999));
    return {};
}

KE_TEST(AssetStoreGetStatsComprehensive)
{
    Kojeom::AssetStore store;
    Kojeom::MaterialData mat;
    store.RegisterMaterial(mat);
    store.RegisterMaterial(mat);

    Kojeom::MeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices = {0, 1, 2};
    store.RegisterInternalMesh("m1", mesh);

    auto stats = store.GetStats();
    KE_EXPECT_TRUE(stats.materialCount == 2);
    KE_EXPECT_TRUE(stats.meshCount == 1);
    return {};
}

KE_TEST(AnimationClipLoop)
{
    Kojeom::Skeleton skeleton;
    Kojeom::Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Kojeom::AnimationClip clip;
    clip.SetDuration(1.0f);
    clip.SetTicksPerSecond(1.0f);
    Kojeom::AnimationChannel ch;
    ch.boneIndex = 0;
    ch.boneName = "Root";
    Kojeom::VectorKey k0;
    k0.time = 0.0f;
    k0.value = Vec3(0.0f);
    ch.positionKeys.push_back(k0);
    Kojeom::VectorKey k1;
    k1.time = 1.0f;
    k1.value = Vec3(10.0f, 0.0f, 0.0f);
    ch.positionKeys.push_back(k1);
    clip.AddChannel(ch);

    Kojeom::Animator animator;
    animator.SetSkeleton(&skeleton);
    animator.SetClip(&clip);
    animator.SetLoop(true);
    animator.Play();

    animator.Tick(1.5f);
    float t = animator.GetPlaybackTime();
    KE_EXPECT_TRUE(t >= 0.0f && t <= 2.0f);
    return {};
}

KE_TEST(AnimationCrossfadeBasic)
{
    Kojeom::Skeleton skeleton;
    Kojeom::Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Kojeom::AnimationClip clipA;
    clipA.SetDuration(1.0f);
    clipA.SetTicksPerSecond(1.0f);
    Kojeom::AnimationChannel chA;
    chA.boneIndex = 0;
    chA.boneName = "Root";
    Kojeom::VectorKey kA0;
    kA0.time = 0.0f;
    kA0.value = Vec3(0.0f);
    chA.positionKeys.push_back(kA0);
    Kojeom::VectorKey kA1;
    kA1.time = 1.0f;
    kA1.value = Vec3(5.0f, 0.0f, 0.0f);
    chA.positionKeys.push_back(kA1);
    clipA.AddChannel(chA);

    Kojeom::AnimationClip clipB;
    clipB.SetDuration(1.0f);
    clipB.SetTicksPerSecond(1.0f);
    Kojeom::AnimationChannel chB;
    chB.boneIndex = 0;
    chB.boneName = "Root";
    Kojeom::VectorKey kB0;
    kB0.time = 0.0f;
    kB0.value = Vec3(0.0f);
    chB.positionKeys.push_back(kB0);
    Kojeom::VectorKey kB1;
    kB1.time = 1.0f;
    kB1.value = Vec3(10.0f, 0.0f, 0.0f);
    chB.positionKeys.push_back(kB1);
    clipB.AddChannel(chB);

    Kojeom::AnimationCrossfade crossfade;
    crossfade.Start(&clipA, 0.0f, &clipB, 0.5f, &skeleton);
    crossfade.Tick(0.25f);
    Pose pose = crossfade.Sample();
    KE_EXPECT_TRUE(pose.GetLocalTransform(0).position.x >= 0.0f);
    KE_EXPECT_TRUE(crossfade.IsActive());
    KE_EXPECT_TRUE(crossfade.GetProgress() > 0.0f);
    return {};
}

KE_TEST(TerrainDataBoundsCheck)
{
    Kojeom::TerrainData terrain;
    terrain.width = 0;
    terrain.height = 0;
    terrain.heightmap.clear();
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(0, 0), 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(terrain.GetHeight(-5, -5), 0.0f, 0.001f);
    return {};
}

KE_TEST(TerrainDataInterpolatedEdge)
{
    Kojeom::TerrainData terrain;
    terrain.width = 3;
    terrain.height = 3;
    terrain.cellSize = 1.0f;
    terrain.heightmap = {0.0f, 1.0f, 2.0f,
                          1.0f, 2.0f, 3.0f,
                          2.0f, 3.0f, 4.0f};
    float h = terrain.GetHeightInterpolated(0.0f, 0.0f);
    KE_EXPECT_FLOAT_EQ(h, 0.0f, 0.001f);
    return {};
}

KE_TEST(RenderSceneMultipleEntities)
{
    Kojeom::World world;
    for (int i = 0; i < 5; ++i)
    {
        auto* e = world.CreateEntity("Mesh" + std::to_string(i));
        e->AddComponent<Kojeom::MeshRendererComponent>(i + 1, i + 10);
    }
    Kojeom::RenderScene scene = world.BuildRenderScene();
    KE_EXPECT_TRUE(scene.staticDrawCommands.size() == 5);
    return {};
}

KE_TEST(RenderSceneMixedComponents)
{
    Kojeom::World world;
    auto* meshEntity = world.CreateEntity("Mesh");
    meshEntity->AddComponent<Kojeom::MeshRendererComponent>(1, 10);

    auto* terrainEntity = world.CreateEntity("Terrain");
    auto* tc = terrainEntity->AddComponent<Kojeom::TerrainComponent>();
    tc->terrainHandle = 2;
    tc->materialHandle = 20;

    auto* camEntity = world.CreateEntity("Camera");
    auto* cam = camEntity->AddComponent<Kojeom::CameraComponent>();
    cam->isActive = true;

    Kojeom::RenderScene scene = world.BuildRenderScene();
    KE_EXPECT_TRUE(scene.staticDrawCommands.size() == 1);
    KE_EXPECT_TRUE(scene.terrainDrawCommands.size() == 1);
    return {};
}

KE_TEST(InputStateBeginFrame)
{
    Kojeom::InputState state;
    state.keysDown[static_cast<int>(Kojeom::KeyCode::W)] = true;
    state.keysPressed[static_cast<int>(Kojeom::KeyCode::W)] = true;
    state.BeginFrame();
    KE_EXPECT_TRUE(state.keysDown[static_cast<int>(Kojeom::KeyCode::W)]);
    KE_EXPECT_FALSE(state.keysPressed[static_cast<int>(Kojeom::KeyCode::W)]);
    return {};
}

KE_TEST(LightDataMaxPointLights)
{
    KE_EXPECT_TRUE(Kojeom::LightData::MAX_POINT_LIGHTS == 4);
    Kojeom::LightData light;
    light.pointLightCount = 2;
    light.pointLights[0].position = Vec3(1.0f, 2.0f, 3.0f);
    light.pointLights[0].color = Vec3(1.0f, 0.5f, 0.2f);
    light.pointLights[0].range = 10.0f;
    light.pointLights[0].intensity = 2.0f;
    KE_EXPECT_FLOAT_EQ(light.pointLights[0].position.x, 1.0f, 0.001f);
    return {};
}

KE_TEST(VertexDefaultTangent)
{
    Kojeom::Vertex v;
    KE_EXPECT_FLOAT_EQ(v.tangent.x, 1.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(v.tangent.y, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(v.tangent.z, 0.0f, 0.001f);
    KE_EXPECT_FLOAT_EQ(v.tangent.w, 1.0f, 0.001f);
    return {};
}

KE_TEST(AppConfigAllModes)
{
    KE_EXPECT(Kojeom::AppConfig::StringToMode("game") == Kojeom::AppConfig::Mode::Game);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("smoke") == Kojeom::AppConfig::Mode::Smoke);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("validate-assets") == Kojeom::AppConfig::Mode::ValidateAssets);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("render-test") == Kojeom::AppConfig::Mode::RenderTest);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("scene-dump") == Kojeom::AppConfig::Mode::SceneDump);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("screenshot-compare") == Kojeom::AppConfig::Mode::ScreenshotCompare);
    KE_EXPECT(Kojeom::AppConfig::StringToMode("benchmark") == Kojeom::AppConfig::Mode::Benchmark);
    KE_EXPECT(Kojeom::AppConfig::ModeToString(Kojeom::AppConfig::Mode::ScreenshotCompare) == "screenshot-compare");
    KE_EXPECT(Kojeom::AppConfig::ModeToString(Kojeom::AppConfig::Mode::Benchmark) == "benchmark");
    return {};
}

KE_TEST(AssetStatsMemoryTracking)
{
    Kojeom::AssetStore store;

    Kojeom::MeshData mesh;
    mesh.vertices.resize(100);
    mesh.indices.resize(300);
    mesh.boundsMin = Vec3(-1.0f);
    mesh.boundsMax = Vec3(1.0f);
    mesh.name = "memory_test";
    store.RegisterInternalMesh("memory_test", mesh);

    Kojeom::TextureData tex;
    tex.width = 4;
    tex.height = 4;
    tex.channels = 4;
    tex.pixels.resize(64, 128);
    tex.path = "mem_test_tex";
    store.RegisterTexture("mem_test_tex", tex);

    auto stats = store.GetStats();
    KE_EXPECT_TRUE(stats.totalMeshBytes > 0);
    KE_EXPECT_TRUE(stats.totalTextureBytes == 64);
    KE_EXPECT_TRUE(stats.totalMemoryBytes == stats.totalMeshBytes + stats.totalTextureBytes);
    KE_EXPECT_TRUE(stats.totalVertexCount == 100);
    KE_EXPECT_TRUE(stats.totalIndexCount == 300);
    return {};
}

KE_TEST(AssetStoreValidateTerrainBounds)
{
    Kojeom::AssetStore store;
    std::vector<float> heights = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    AssetHandle handle = store.CreateTerrain("terrain_bounds", 3, 3, 1.0f, 10.0f, heights);
    KE_EXPECT_TRUE(handle != INVALID_HANDLE);

    const Kojeom::TerrainData* terrain = store.GetTerrain(handle);
    KE_EXPECT_TRUE(terrain != nullptr);

    auto issues = store.ValidateAllAssets();
    KE_EXPECT_TRUE(issues.empty());

    float h = terrain->GetHeightInterpolated(1.5f, 1.5f);
    KE_EXPECT_TRUE(h >= 0.0f && h <= 10.0f);
    return {};
}

KE_TEST(AssetStoreEmptyMeshValidation)
{
    Kojeom::AssetStore store;
    Kojeom::MeshData emptyMesh;
    emptyMesh.name = "empty";
    AssetHandle handle = store.RegisterInternalMesh("empty_mesh", emptyMesh);
    auto issues = store.ValidateAllAssets();
    bool foundEmptyIssue = false;
    for (const auto& issue : issues)
    {
        if (issue.find("no vertices") != std::string::npos)
            foundEmptyIssue = true;
    }
    KE_EXPECT_TRUE(foundEmptyIssue);
    return {};
}

KE_TEST(WorldNestedHierarchy)
{
    Kojeom::World world;
    auto* root = world.CreateEntity("Root");
    auto* child1 = world.CreateEntity("Child1");
    auto* child2 = world.CreateEntity("Child2");

    world.AttachChild(root->GetId(), child1->GetId());
    world.AttachChild(child1->GetId(), child2->GetId());

    KE_EXPECT_TRUE(child2->HasParent());
    KE_EXPECT_TRUE(child2->GetParentId() == child1->GetId());
    KE_EXPECT_TRUE(child1->HasChildren());
    KE_EXPECT_TRUE(child1->GetChildren().size() == 1);
    KE_EXPECT_TRUE(root->HasChildren());
    return {};
}

KE_TEST(AnimationClipMissingBone)
{
    Kojeom::Skeleton skeleton;
    Kojeom::Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = Mat4(1.0f);
    skeleton.AddBone(root);

    Kojeom::AnimationClip clip;
    clip.SetDuration(1.0f);
    clip.SetTicksPerSecond(1.0f);

    Kojeom::AnimationChannel ch;
    ch.boneIndex = 5;
    ch.boneName = "NonExistent";
    Kojeom::VectorKey k;
    k.time = 0.0f;
    k.value = Vec3(1.0f);
    ch.positionKeys.push_back(k);
    clip.AddChannel(ch);

    Kojeom::Pose pose = clip.Sample(0.0f, skeleton);
    KE_EXPECT_TRUE(pose.GetBoneCount() == 1);
    return {};
}

int main()
{
    Kojeom::Log::Init("test_results.log");
    int failures = Kojeom::TestRunner::Get().RunAll();
    return failures;
}
