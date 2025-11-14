#include "TwoHalfD/time_delta.h"
#include <chrono>

bool TwoHalfD::TimeDelta::timeDeltaPassed()
{
    auto currTime = std::chrono::system_clock::now();
    if (std::chrono::duration<double, std::milli>(currTime - m_lastTimeDeltaIntervalStart).count() > m_timeDelta * 1000)
    {
        m_lastTimeDeltaIntervalStart = std::chrono::system_clock::now();
        m_lastTime = std::chrono::system_clock::now();
        return true;
    }
    m_lastTime = std::chrono::system_clock::now();
    return false;
}