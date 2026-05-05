#include "World/World.h"
#include "Animation/AnimationClip.h"
#include "Assets/AssetStore.h"

namespace Kojeom
{
AssetStore* World::s_assetStore = nullptr;

void AnimatorComponent::Tick(float deltaSeconds)
{
    if (!playing) return;
    if (skeletonHandle == INVALID_HANDLE || currentClipHandle == INVALID_HANDLE) return;

    if (needsInit)
    {
        needsInit = false;
        AssetStore* store = World::GetAssetStore();
        if (store)
        {
            auto* skelData = store->GetSkeleton(skeletonHandle);
            auto* clipData = store->GetAnimationClip(currentClipHandle);
            if (skelData && clipData)
            {
                internalAnimator.SetSkeleton(&skelData->skeleton);
                internalAnimator.SetClip(&clipData->clip);
                internalAnimator.SetLoop(loop);
                internalAnimator.SetSpeed(speed);

                stateMachine.SetSkeleton(&skelData->skeleton);
            }
        }
    }

    if (useStateMachine)
    {
        stateMachine.Tick(deltaSeconds);
        cachedSkinMatrices = stateMachine.GetSkinMatrices();
        playbackTime = 0.0f;
        return;
    }

    internalAnimator.SetLoop(loop);
    internalAnimator.SetSpeed(speed);
    if (!internalAnimator.IsPlaying() && playing)
        internalAnimator.Play();

    internalAnimator.Tick(deltaSeconds);
    playbackTime = internalAnimator.GetPlaybackTime();

    cachedSkinMatrices = internalAnimator.GetSkinMatrices();
}
}
