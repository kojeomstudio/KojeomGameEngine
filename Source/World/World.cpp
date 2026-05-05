#include "World/World.h"
#include "Animation/AnimationClip.h"
#include "Assets/AssetStore.h"

namespace Kojeom
{
void AnimatorComponent::Tick(float deltaSeconds)
{
    if (!playing) return;
    if (skeletonHandle == INVALID_HANDLE || currentClipHandle == INVALID_HANDLE) return;

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
