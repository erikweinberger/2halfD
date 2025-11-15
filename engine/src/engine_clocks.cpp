#include "TwoHalfD/engine_clocks.h"

bool TwoHalfD::EngineClocks::graphicsTimeDeltaPassed()
{
    bool graphicsDeltaTimePassed = m_graphicsTimeDelta.timeDeltaPassed();
    if (graphicsDeltaTimePassed)
    {
        m_graphicFpsSample.push(m_graphicsTimeDelta.getLastDeltaDuration());
    }
    return graphicsDeltaTimePassed;
}

bool TwoHalfD::EngineClocks::gameTimeDeltaPassed()
{
    return m_gameTimeDelta.timeDeltaPassed();
}

double TwoHalfD::EngineClocks::getAverageGraphicsFps()
{
    double result = 0.0;
    for (const auto &fpsSample : m_graphicFpsSample)
    {
        result += fpsSample;
    }
    result = (result) / (m_graphicFpsSample.size() * 1000);
    return 1 / result;
}