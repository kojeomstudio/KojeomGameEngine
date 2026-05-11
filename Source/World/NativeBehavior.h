#pragma once

#include <string>
#include <functional>
#include <vector>
#include "Math/Transform.h"

namespace Kojeom
{
class Entity;
class Engine;

using BehaviorUpdateFn = std::function<void(Entity* entity, Engine* engine, float deltaSeconds)>;
using BehaviorStartFn = std::function<void(Entity* entity, Engine* engine)>;
using BehaviorStopFn = std::function<void(Entity* entity, Engine* engine)>;

struct NativeBehavior
{
    std::string behaviorName;
    BehaviorStartFn onStart = nullptr;
    BehaviorUpdateFn onUpdate = nullptr;
    BehaviorStopFn onStop = nullptr;
    bool bStarted = false;
    void* userData = nullptr;

    static NativeBehavior CreateSimple(const std::string& name, BehaviorUpdateFn updateFn)
    {
        NativeBehavior behavior;
        behavior.behaviorName = name;
        behavior.onUpdate = std::move(updateFn);
        return behavior;
    }
};

class Engine;

struct NativeBehaviorComponent
{
    std::vector<NativeBehavior> behaviors;

    void AddBehavior(const NativeBehavior& behavior)
    {
        behaviors.push_back(behavior);
    }

    void TickBehaviors(Entity* entity, Engine* engine, float deltaSeconds)
    {
        for (auto& behavior : behaviors)
        {
            if (!behavior.bStarted)
            {
                if (behavior.onStart)
                    behavior.onStart(entity, engine);
                behavior.bStarted = true;
            }
            if (behavior.onUpdate)
                behavior.onUpdate(entity, engine, deltaSeconds);
        }
    }

    void StopBehaviors(Entity* entity, Engine* engine)
    {
        for (auto& behavior : behaviors)
        {
            if (behavior.bStarted && behavior.onStop)
                behavior.onStop(entity, engine);
            behavior.bStarted = false;
        }
    }
};
}
