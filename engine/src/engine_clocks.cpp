#include "TwoHalfD/engine_clocks.h"

bool TwoHalfD::EngineClocks::graphicsTimeDeltaPassed()
{
    return m_graphicsTimeDelta.timeDeltaPassed();
}

bool TwoHalfD::EngineClocks::gameTimeDeltaPassed()
{
    return m_gameTimeDelta.timeDeltaPassed();
}