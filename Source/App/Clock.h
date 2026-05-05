#pragma once

#include <chrono>

namespace Kojeom
{
class Clock
{
public:
    void Reset()
    {
        m_start = std::chrono::high_resolution_clock::now();
        m_last = m_start;
    }

    float Tick()
    {
        auto now = std::chrono::high_resolution_clock::now();
        m_lastDelta = std::chrono::duration<float>(now - m_last).count();
        m_last = now;
        m_elapsed += m_lastDelta;
        return m_lastDelta;
    }

    float GetElapsedSeconds() const { return m_elapsed; }
    float GetDeltaTime() const { return m_lastDelta; }

private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_last;
    float m_elapsed = 0.0f;
    float m_lastDelta = 0.0f;
};
}
