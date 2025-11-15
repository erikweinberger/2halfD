#include "TwoHalfD/time_delta.h"
#include <chrono>

bool TwoHalfD::TimeDelta::timeDeltaPassed()
{
    auto currTime = std::chrono::system_clock::now();
    double currInterval = std::chrono::duration<double, std::milli>(currTime - m_lastTimeDeltaIntervalStart).count();

    if (currInterval > m_timeDelta * 1000 || m_isUncapped)
    {
        m_lastDeltaDurationMilli = currInterval;
        m_lastTimeDeltaIntervalStart = std::chrono::system_clock::now();
        m_lastTime = std::chrono::system_clock::now();
        return true;
    }

    m_lastTime = std::chrono::system_clock::now();
    return false;
}

double TwoHalfD::TimeDelta::getLastDeltaDuration()
{
    return m_lastDeltaDurationMilli;
}