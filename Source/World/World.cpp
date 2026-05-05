#include "World/World.h"
#include "Animation/AnimationClip.h"

namespace Kojeom
{
void AnimatorComponent::Tick(float deltaSeconds)
{
    if (!playing || currentClipHandle == INVALID_HANDLE) return;
    playbackTime += deltaSeconds * speed;
}
}
